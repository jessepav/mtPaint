/*	help.c
	Copyright (C) 2004-2006 Mark Tyler

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

int help_page_count = 4
;
char *help_page_titles[] = {
_("General"),
_("Keyboard shortcuts"),
_("Mouse shortcuts"),
_("Credits"),
NULL };

char *help_page_contents[] = {
_("\n"
"mtPaint 3.00 - Copyright (C) 2004-2006 The Authors\n"
"\n"
"See 'Credits' section for a list of the authors.\n"
"\n"
"mtPaint is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\n"
"\n"
"mtPaint is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n"
"\n"
"mtPaint is a simple GTK+1/2 painting program designed for creating icons and pixel based artwork. It can edit indexed palette or 24 bit RGB images and offers basic painting and palette manipulation tools. It also has several other more powerful features such as channels, layers and animation. Due to its simplicity and lack of dependencies it runs well on GNU/Linux, Windows and older PC hardware.\n"
"\n"
"There is full documentation of mtPaint's features contained in a handbook.  If you don't already have this, you can download it from the mtPaint website.\n"
"\n"
"If you are a developer and you have improved any part of mtPaint, you are very welcome to contact the maintainer to have these improvements merged into mtPaint for the benefit of all users.\n"
"\n"
),
_("\n"
"  Ctrl-N            Create new image\n"
"  Ctrl-O            Open Image\n"
"  Ctrl-S            Save Image\n"
"  Ctrl-Q            Quit program\n"
"\n"
"  Ctrl-A            Select whole image\n"
"  Escape            Select nothing, cancel paste box\n"
"  Ctrl-C            Copy selection to clipboard\n"
"  Ctrl-X            Copy selection to clipboard, and then paint current pattern to selection area\n"
"  Ctrl-V            Paste clipboard to centre of current view\n"
"  Ctrl-K            Paste clipboard to location it was copied from\n"
"  Enter/Return      Commit paste to canvas\n"
"\n"
"  Arrow keys        Paint Mode - Change colour A or B\n"
"  Arrow keys        Selection Mode - Nudge selection box or paste box by one pixel\n"
"  Shift+Arrow keys  Nudge selection box or paste box by x pixels - x is defined by the Preferences window\n"
"\n"
"  Delete            Crop image to selection\n"
"  Insert            Transform colours - i.e. Brightness, Contrast, Saturation, Posterize, Gamma\n"
"  Ctrl-G            Greyscale the image\n"
"\n"
"  Ctrl-T            Draw a rectangle around the selection area with the current fill\n"
"  Ctrl-Shift-T      Fill in the selection area with the current fill\n"
"  Ctrl-L            Draw an ellipse spanning the selection area\n"
"  Ctrl-Shift-L      Draw a filled ellipse spanning the selection area\n"
"\n"
"  Ctrl-E            Edit the RGB values for colours A & B\n"
"  Ctrl-W            Edit all palette colours\n"
"\n"
"  Ctrl-P            Preferences\n"
"  Ctrl-I            Information\n"
"\n"
"  Ctrl-Z            Undo last action\n"
"  Ctrl-R            Redo an undone action\n"
"\n"
"  C                 Command Line Window\n"
"  V                 View Window\n"
"  L                 Layers Window\n"
"  Z                 Set zoom\n"
"\n"
"  +,=               Zoom in\n"
"  -                 Zoom out\n"
"\n"
"  1                 10% zoom\n"
"  2                 25% zoom\n"
"  3                 50% zoom\n"
"  4                 100% zoom\n"
"  5                 400% zoom\n"
"  6                 800% zoom\n"
"  7                 1200% zoom\n"
"  8                 1600% zoom\n"
"  9                 2000% zoom\n"
"\n"
"  F1                Help\n"
"  F2                Choose Pattern\n"
"  F3                Choose Brush\n"
"  F4                Paint Tool\n"
"  F5                Toggle Main Toolbar\n"
"  F6                Toggle Tools Toolbar\n"
"  F7                Toggle Settings Toolbar\n"
"  F8                Toggle Palette\n"
"  F9                Selection Tool\n"
"\n"
"  Ctrl + F1 - F12   Save current clipboard to file 1-12\n"
"  Shift + F1 - F12  Load clipboard from file 1-12\n"
"\n"
"  Ctrl + 1, 2, ... , 0  Set opacity to 10%, 20%, ... , 100% (main or keypad numbers)\n"
"  Ctrl + + or =     Increase opacity by 1%\n"
"  Ctrl + -          Decrease opacity by 1%\n"
"\n"
"  Home              Show or hide main window menu/toolbar/status bar/palette\n"
"  Page Up           Scale Image\n"
"  Page Down         Resize Image canvas\n"
"  End               Pan Window\n"
),
_("\n"
"  Left button          Paint to canvas using the current tool\n"
"  Middle button        Set the centre for the next zoom\n"
"  Right button         Commit paste to canvas / Stop drawing current line / Cancel selection\n"
"\n"
"  Scroll Wheel         In GTK+2 the user can have the scroll wheel zoom in or out via the Preferences window\n"
"\n"
"  Ctrl+Left button     Choose colour A from under mouse pointer\n"
"  Ctrl+Right button    Choose colour B from under mouse pointer\n"
"\n"
"  Shift+Right button   Set the centre for the next zoom\n"
"\n"
"\n"
"You can fixate the X/Y co-ordinates while moving the mouse:\n"
"\n"
"  Shift                Constrain mouse movements to vertical line\n"
"  Shift+Ctrl           Constrain mouse movements to horizontal line\n"
),
_("\n"
"mtPaint is maintained by Mark Tyler.\n"
"\n"
"marktyler_5@hotmail.com\n"
"http://mtpaint.sourceforge.net/\n"
"\n"
"\n"
"The following people (in alphabetical order) have contributed directly to the project, and are therefore worthy of gracious thanks for their generosity and hard work:\n"
"\n"
"\n"
"Authors\n"
"\n"
"Dennis Lee - Wrote the two quantizing methods DL1 & 3 - see quantizer.c for more information.\n"
"Dmitry Groshev - Wrote the code for tint mode and image scaling. Rewrote much of the code for version 3 in order to support channels, and also applied many other improvements to the code.\n"
"Magnus Hjorth - Wrote inifile.c/h, from mhWaveEdit 1.3.0.\n"
"Mark Tyler - Original author.\n"
"Xiaolin Wu - Wrote the Wu quantizing method - see wu.c for more information.\n"
"\n"
"\n"
"General Contributions\n"
"\n"
"Abdulla Al Muhairi - Website redesign April 2005\n"
"Alan Horkan - Feedback/ideas\n"
"Alexandre Prokoudine - Feedback/ideas\n"
"Antonio Andrea Bianco - Feedback/ideas\n"
"Ed Jason - Feedback/ideas\n"
"Eddie Kohler - Created Gifsicle which is needed for the creation and viewing of animated GIF files http://www.lcdf.org/gifsicle/\n"
"Guadalinex Team (Junta de Andalucia) - Feedback/ideas, man page, Launchpad/Rosetta registration\n"
"Lou Afonso - Feedback/ideas\n"
"Magnus Hjorth - Feedback/ideas\n"
"Martin Zelaia - Feedback/ideas\n"
"Pavel Ruzicka - Feedback/ideas\n"
"Puppy Linux (Barry Kauler) - Feedback/ideas\n"
"Vlastimil Krejcir - Feedback/ideas\n"
"William Kern - Feedback/ideas\n"
"\n"
"\n"
"Translations\n"
"\n"
"Guadalinex Team (Junta de Andalucia) - Spanish\n"
"Israel G. Lugo - Portuguese\n"
"Nicolas Velin, Pascal Billard - French\n"
"Paulo Trevizan - Brazilian Portuguese\n"
"Pavel Ruzicka - Czech\n"
),
NULL };
