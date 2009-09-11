/*	inifile.c
	Copyright (C) 2007 Dmitry Groshev

	This file is part of mtPaint.

	mtPaint is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	mtPaint is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with mtPaint in the file COPYING.
*/

/* *** PREFACE ***
 *  This is deliberately simple implementation - no comment preservation and
 * no sections, because we've no need of them.
 *  Allocations are done in slabs, because no slot is ever deallocated or
 * reordered; only modified string values are allocated singly, since it is
 * probable they will keep being modified.
 *  Implementation allows no more than 16K ini entries, because of using one
 * 32-bit hash function as two 16-bit ones, but this is more than enough for
 * our purposes. - WJ */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "global.h"
#include "inifile.h"

/* Make code not compile where it cannot run */
typedef char Integers_Do_Not_Fit_Into_Pointers[2 * (sizeof(int) <= sizeof(char *)) - 1];

#define SLAB_INCREMENT 16384
#define SLAB_RESERVED  64 /* Reserved for allocator overhead */

#define INI_LIMIT 16384 /* Limit imposed by hashing scheme */
#define HASHFILL(X) ((X) * 3) /* Hash occupancy no more than 1/3 */
#define HASHSEED 0x811C9DC5
#define HASH_RND(X) ((X) * 0x10450405 + 1)
#define HASH_MIN 256 /* Minimum hash capacity, so that it won't be too tiny */

/* Slot types */
#define INI_NONE  0
#define INI_UNDEF 1
#define INI_STR   2
#define INI_INT   3
#define INI_BOOL  4

/* Slot flags */
#define INI_STRING  0x0001 /* Index of strings block - must be bit 0 */
#define INI_MALLOC  0x0002 /* Value is allocated (not in strings block 0) */
#define INI_DEFAULT 0x0004 /* Default value is defined */

#define SLOT_NAME(I,S) ((I)->sblock[(S)->flags & INI_STRING] + (S)->key)

/* "One at a time" hash function */
static guint32 hashf(guint32 seed, char *key)
{
	for (; *key; key++)
	{
		seed += *key;
		seed += seed << 10;
		seed ^= seed >> 6;
	}
	seed += seed << 3;
	seed ^= seed >> 11;
	seed += seed << 15;
	return (seed);
} 

static int cuckoo_insert(inifile *inip, inislot *slotp)
{
	char *name;
	short idx, tmp;
	guint32 key;
	int i, j;

	/* Normal cuckoo process */
	idx = (slotp - inip->slots) + 1;
	for (i = 0; i < inip->maxloop; i++)
	{
		name = SLOT_NAME(inip, slotp);
		key = hashf(inip->seed, name);
		key >>= (i & 1) << 4;
		j = (key & inip->hmask) * 2 + (i & 1);
		tmp = inip->hash[j];
		inip->hash[j] = idx;
		idx = tmp;
		if (!idx) return (TRUE);
		slotp = inip->slots + (idx - 1);
	}
	return (FALSE);
}

static int resize_hash(inifile *inip, int cnt)
{
	int len;

	if (!cnt) return (0); /* Degenerate case */
	len = HASHFILL(cnt) - 1;
	len |= len >> 1;
	len |= len >> 2;
	len |= len >> 4;
	len |= len >> 8;
	len |= len >> 16;
	len++;
	if (len <= inip->hmask * 2 + 2) return (0); /* Large enough */
	free(inip->hash);
	inip->hash = calloc(1, len * sizeof(short));
	if (!inip->hash) return (-1); /* Failure */
	inip->hmask = (len >> 1) - 1;
	inip->maxloop = ceil(3.0 * log(len / 2) /
		log((double)(len / 2) / cnt));
	return (1); /* Resized */
}

static int rehash(inifile *inip)
{
	int i, flag;

	flag = resize_hash(inip, inip->count);
	if (flag < 0) return (FALSE); /* Failed */

	while (TRUE) /* Until done */
	{
		if (!flag) /* No size change */
		{
			inip->seed = HASH_RND(inip->seed);
			memset(inip->hash, 0, (inip->hmask + 1) * 2 * sizeof(short));
		}
		/* Re-insert items */
		for (i = 0; (i < inip->count) &&
			(flag = cuckoo_insert(inip, inip->slots + i)); i++);
		if (flag) return (TRUE);
	}
}

static inislot *cuckoo_find(inifile *inip, char *name)
{
	char *sname;
	inislot *slotp;
	guint32 key;
	int i, j = 0;

	key = hashf(inip->seed, name);
	while (TRUE)
	{
		i = inip->hash[(key & inip->hmask) * 2 + j];
		if (i)
		{
			slotp = inip->slots + i - 1;
			sname = SLOT_NAME(inip, slotp);
			if (!strcmp(name, sname)) return (slotp);
		}
		if (j++) return (NULL);
		key >>= 16;
	}
}

static inislot *add_slot(inifile *inip)
{
	inislot *ra;
	int i, j;

	if (inip->count >= INI_LIMIT) return (FALSE); /* Too many entries */
	/* Extend the slab if needed */
	i = inip->count * sizeof(inislot) + SLAB_RESERVED + SLAB_INCREMENT - 1;
	j = (i + sizeof(inislot)) / SLAB_INCREMENT;
	if (i / SLAB_INCREMENT < j)
	{
		ra = realloc(inip->slots, j * SLAB_INCREMENT - SLAB_RESERVED);
		if (!ra) return (NULL);
		inip->slots = ra;
	}
	ra = inip->slots + inip->count++;
	memset(ra, 0, sizeof(inislot));
	return (ra);
}

static char *store_string(inifile *inip, char *str)
{
	int i, j, l;
	char *ra;

	/* Zero offset for zero length */
	if (!str || !*str) return (inip->sblock[1]);

	/* Extend the slab if needed */
	l = strlen(str) + 1;
	i = inip->slen + SLAB_RESERVED + SLAB_INCREMENT - 1;
	j = (i + l) / SLAB_INCREMENT;
	if (i / SLAB_INCREMENT < j)
	{
		ra = realloc(inip->sblock[1], j * SLAB_INCREMENT - SLAB_RESERVED);
		if (!ra) return (NULL);
		inip->sblock[1] = ra;
	}

	ra = inip->sblock[1] + inip->slen;
	memcpy(ra, str, l);
	inip->slen += l;
	return (ra);
}

static inislot *key_slot(inifile *inip, char *key, int type)
{
	inislot *slot;

	slot = cuckoo_find(inip, key);
	if (slot)
	{
		if (type == INI_NONE) return (slot);
		if (slot->flags & INI_MALLOC)
		{
			free(slot->value);
			slot->flags ^= INI_MALLOC;
		}
#if VALIDATE_TYPE
		if ((slot->type != type) && (slot->type > INI_UNDEF))
			g_warning("INI key '%s' changed type\n", key);
#endif
	}
	else
	{
		key = store_string(inip, key);
		if (!key) return (NULL);
		slot = add_slot(inip);
		if (!slot) return (NULL);
		slot->key = key - inip->sblock[1];
		slot->flags |= INI_STRING;
		if (!cuckoo_insert(inip, slot) && !rehash(inip))
			return (NULL);
	}
	slot->type = type;
	return (slot);
}

int new_ini(inifile *inip)
{
	memset(inip, 0, sizeof(inifile));
	inip->sblock[1] = malloc(SLAB_INCREMENT - SLAB_RESERVED);
	if (!inip->sblock[1]) return (FALSE);
	inip->sblock[1][0] = 0;
	inip->slen = 1;
	inip->slots = malloc(SLAB_INCREMENT - SLAB_RESERVED);
	inip->seed = HASHSEED;
	return (inip->slots && (resize_hash(inip, HASH_MIN) > 0));
}

void forget_ini(inifile *inip)
{
	int i;

	for (i = 0; i < inip->count; i++)
	{
		if (inip->slots[i].flags & INI_MALLOC)
			free(inip->slots[i].value);
	}
	free(inip->sblock[0]);
	free(inip->sblock[1]);
	free(inip->slots);
	free(inip->hash);
	memset(inip, 0, sizeof(inifile));
}

int read_ini(inifile *inip, char *fname)
{
	FILE *fp;
	inifile ini;
	inislot *slot;
	char *tmp, *wrk, *w2, *str;
	int i, l;


	/* Init the structure */
	if (!new_ini(&ini)) return (FALSE);

	/* Read the file */
	if ((fp = fopen(fname, "rb")) == NULL) goto fail;
	fseek(fp, 0, SEEK_END);
	l = ftell(fp);
	ini.sblock[0] = calloc(1, l + 2);
	if (!ini.sblock[0]) goto ffail;
	fseek(fp, 0, SEEK_SET);
	i = fread(ini.sblock[0] + 1, 1, l, fp);
	if (i != l) goto ffail;
	fclose(fp);

	/* Parse the contents */
	for (tmp = ini.sblock[0] + 1; ; tmp = str)
	{
		tmp += strspn(tmp, "\r\n\t ");
		if (!*tmp) break;
		str = tmp + strcspn(tmp, "\r\n");
		if (*str) *str++ = '\0';
		if ((*tmp == ';') || (*tmp == '#')) continue; /* Comment */
		wrk = strpbrk(tmp, "=\t ");
		if (!wrk || (*(w2 = wrk + strspn(wrk, "\t ")) != '='))
		{
			g_printerr("Wrong INI line: '%s'\n", tmp);
			continue;
		}
		w2 += 1 + strspn(w2 + 1, "\t ");
		*wrk = '\0';
//		for (wrk = str - 1; *wrk && ((*wrk == '\t') || (*wrk == ' '); *wrk-- = '\0');
		/* Hash this pair */
		slot = cuckoo_find(&ini, tmp);
		if (!slot) /* New key */
		{
			slot = add_slot(&ini);
			if (!slot) goto fail;
			slot->type = INI_UNDEF;
			slot->key = tmp - ini.sblock[0];
			if (!cuckoo_insert(&ini, slot) && !rehash(&ini))
				goto fail;
		}
		slot->value = w2;
	}

	/* Return the result */
	*inip = ini;
	return (TRUE);

ffail:	fclose(fp);
fail:	forget_ini(&ini);
	return (FALSE);
}

int write_ini(inifile *inip, char *fname, char *header)
{
	FILE *fp;
	inislot *slotp;
	char *name, *sv;
	int i;

	if (!(fp = fopen(fname, "w"))) return (FALSE);
	if (header) fprintf(fp, "%s\n", header);

	for (i = 0; i < inip->count; i++)
	{
		slotp = inip->slots + i;
		name = SLOT_NAME(inip, slotp);
		sv = slotp->value ? slotp->value : "";
		switch (slotp->type)
		{
		case INI_STR:
			if ((slotp->flags & INI_DEFAULT) &&
				!strcmp(inip->sblock[1] + slotp->defv, sv))
				break;
		case INI_UNDEF:
		default:
			fprintf(fp, "%s = %s\n", name, sv);
			break;
		case INI_INT:
			if ((slotp->flags & INI_DEFAULT) &&
				((int)slotp->value == slotp->defv)) break;
			fprintf(fp, "%s = %d\n", name, (int)slotp->value);
			break;
		case INI_BOOL:
			if ((slotp->flags & INI_DEFAULT) &&
				(!!slotp->value == !!slotp->defv)) break;
			fprintf(fp, "%s = %s\n", name, slotp->value ?
				"true" : "false");
			break;
		}
	}
	fclose(fp);
	return (TRUE);
}

int ini_setstr(inifile *inip, char *key, char *value)
{
	inislot *slot;

	if (!(slot = key_slot(inip, key, INI_STR))) return (FALSE);
/* NULLs are stored as empty strings, for less hazardous handling */
	slot->value = "";
/* Uncomment this instead if want NULLs to remain NULLs */
//	slot->value = NULL;
	if (value)
	{
		value = strdup(value);
		if (!value) return (FALSE);
		slot->value = value;
		slot->flags |= INI_MALLOC;
	}
	return (TRUE);
}

int ini_setint(inifile *inip, char *key, int value)
{
	inislot *slot;

	if (!(slot = key_slot(inip, key, INI_INT))) return (FALSE);
	slot->value = (char *)value;
	return (TRUE);
}

int ini_setbool(inifile *inip, char *key, int value)
{
	inislot *slot;

	if (!(slot = key_slot(inip, key, INI_BOOL))) return (FALSE);
	slot->value = (char *)!!value;
	return (TRUE);
}

char *ini_getstr(inifile *inip, char *key, char *defv)
{
	inislot *slot;
	char *tail;

	/* Read existing */
	slot = key_slot(inip, key, INI_NONE);
	if (!slot) return (defv);
	if (slot->type == INI_STR)
	{
#if VALIDATE_DEF
		if ((slot->flags & INI_DEFAULT) &&
			strcmp(defv ? defv : "", inip->sblock[1] + slot->defv))
			g_warning("INI key '%s' new default\n", key);
#endif
		if (slot->flags & INI_DEFAULT) return (slot->value);
		slot->type = INI_UNDEF; /* Fall through to storing default */
	}

	/* Store default */
	tail = store_string(inip, defv);
	if (tail)
	{
		slot->defv = tail - inip->sblock[1];
		slot->flags |= INI_DEFAULT;
	}
	else /* Cannot store the default */
	{
		slot->defv = 0;
		slot->flags &= ~INI_DEFAULT;
	}

	if (slot->type != INI_UNDEF)
	{
#if VALIDATE_TYPE
		if (slot->type != INI_NONE)
			g_printerr("INI key '%s' wrong type\n", key);
#endif
		slot->value = defv;
		if (defv) slot->value = strdup(defv);
		if (slot->value) slot->flags |= INI_MALLOC;
	}
	slot->type = INI_STR;
	return (slot->value);
}

int ini_getint(inifile *inip, char *key, int defv)
{
	inislot *slot;
	char *tail;
	long l;

	/* Read existing */
	slot = key_slot(inip, key, INI_NONE);
	if (!slot) return (defv);
	if (slot->type == INI_INT)
	{
#if VALIDATE_DEF
		if ((slot->flags & INI_DEFAULT) && (defv != slot->defv))
			g_warning("INI key '%s' new default\n", key);
#endif
		if (slot->flags & INI_DEFAULT) return ((int)(slot->value));
		/* Fall through to storing default */
	}

	/* Store default */
	slot->defv = defv;
	slot->flags |= INI_DEFAULT;

	while (slot->type != INI_INT)
	{
		if (slot->type == INI_UNDEF)
		{
			l = strtol(slot->value, &tail, 10);
			slot->value = (void *)l;
			if (!*tail) break;
		}
		else if (slot->flags & INI_MALLOC) free(slot->value);
#if VALIDATE_TYPE
		if (slot->type != INI_NONE)
			g_printerr("INI key '%s' wrong type\n", key);
#endif
		slot->value = (char *)defv;
		break;
	}
	slot->type = INI_INT;
	return ((int)(slot->value));
}

int ini_getbool(inifile *inip, char *key, int defv)
{
	static const char *YN[] = { "n", "y", "0", "1", "no", "yes",
		"off", "on", "false", "true", "disabled", "enabled", NULL };
	inislot *slot;
	int i;

	defv = !!defv;

	/* Read existing */
	slot = key_slot(inip, key, INI_NONE);
	if (!slot) return (defv);
	if (slot->type == INI_BOOL)
	{
#if VALIDATE_DEF
		if ((slot->flags & INI_DEFAULT) && (defv != slot->defv))
			g_warning("INI key '%s' new default\n", key);
#endif
		if (slot->flags & INI_DEFAULT) return ((int)(slot->value));
		/* Fall through to storing default */
	}

	/* Store default */
	slot->defv = defv;
	slot->flags |= INI_DEFAULT;

	while (slot->type != INI_BOOL)
	{
		if (slot->type == INI_UNDEF)
		{
			for (i = 0; YN[i] && strcasecmp(YN[i], slot->value); i++);
			slot->value = (char *)(i & 1);
			if (YN[i]) break;
		}
		else if (slot->flags & INI_MALLOC) free(slot->value);
#if VALIDATE_TYPE
		if (slot->type != INI_NONE)
			g_printerr("INI key '%s' wrong type\n", key);
#endif
		slot->value = (char *)defv;
		break;
	}
	slot->type = INI_BOOL;
	return ((int)(slot->value));
}

#ifdef WIN32

char *get_home_directory(void)
{
	static char *homedir = NULL;

	if (homedir) return homedir;
	homedir = getenv("USERPROFILE");	// Gets the current users home directory in WinXP
	if (!homedir) homedir = "";		// And this, in Win9x :-)
	return homedir;
}

#else

#include <unistd.h>
#include <pwd.h>

/*
 * This function came from mhWaveEdit
 * by Magnus Hjorth, 2003.
 */
gchar *get_home_directory(void)
{
	static char *homedir = NULL;
	struct passwd *p;

	if (homedir) return homedir;
	homedir = getenv("HOME");
	if (!homedir)
	{
		p = getpwuid(getuid());
		if (p) homedir = p->pw_dir;
	}
	if (!homedir)
	{
		g_warning(_("Could not find home directory. Using current directory as "
			"home directory."));
		homedir = ".";
	}
	return homedir;
}

#endif

/* Compatibility functions */

static inifile main_ini;
static char main_ininame[530];

void inifile_init(char *ini_filename)
{
	snprintf(main_ininame, 512, "%s%s", get_home_directory(), ini_filename);
	if (!read_ini(&main_ini, main_ininame)) new_ini(&main_ini);
}

void inifile_quit(void)
{
	write_ini(&main_ini, main_ininame,
	  "# Remove this file to restore default settings.\n");
	forget_ini(&main_ini);
}

gchar *inifile_get(gchar *setting, gchar *defaultValue)
{
	return (ini_getstr(&main_ini, setting, defaultValue));
}

gint32 inifile_get_gint32(gchar *setting, gint32 defaultValue)
{
	return (ini_getint(&main_ini, setting, defaultValue));
}

gboolean inifile_get_gboolean(gchar *setting, gboolean defaultValue)
{
	return (ini_getbool(&main_ini, setting, defaultValue));
}

gboolean inifile_set(gchar *setting, gchar *value)
{
	return (ini_setstr(&main_ini, setting, value));
}

gboolean inifile_set_gint32(gchar *setting, gint32 value)
{
	return (ini_setint(&main_ini, setting, value));
}

gboolean inifile_set_gboolean(gchar *setting, gboolean value)
{
	return (ini_setbool(&main_ini, setting, value));
}
