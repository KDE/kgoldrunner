/***************************************************************************
                          kgoldrunner.cpp  -  description
                             -------------------
    begin                : Wed Jan 23 15:19:17 EST 2002
    copyright            : (C) 2002 by Marco Krüger and Ian Wadham
    email                : See menu "Help, About KGoldrunner"
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/******************************************************************************/
/*****************************    INCLUDEs     ********************************/
/******************************************************************************/

#include "kgoldrunner.h"
#include "kgoldrunner.moc"
#include "kgoldrunnerwidget.h"

#include <iostream.h>
#include <stdlib.h>

// ICON FILES
#include "hero.xpm"
#include "enemy1.xpm"
#include "enemy2.xpm"

#include "brick.xpm"
#include "hgbrick.xpm"
#include "nugget.xpm"
#include "pole.xpm"
#include "beton.xpm"
#include "ladder.xpm"
#include "hladder.xpm"

#include "edithero.xpm"
#include "editenemy.xpm"

#include "filenew.xpm"
#include "fileopen.xpm"
#include "filesave.xpm"

/******************************************************************************/
/***********************    COLLECTION DATA CLASS    **************************/
/******************************************************************************/

KGrCollection::KGrCollection (Owner o, const QString & n, const QString & p,
			      const char s, int nl, const QString & a)
{
    // Load the information about a collection of KGoldrunner levels.
    owner = o; name = n; prefix = p; settings = s; nLevels = nl; about = a;
}

/******************************************************************************/
/**********************    KGOLDRUNNER (MAIN) CLASS    ************************/
/******************************************************************************/

#ifdef QT1
KGoldrunner::KGoldrunner(QWidget *parent, const char *name, WFlags f)
	: KTopLevelWidget(name)
#else

#include <kstddirs.h>		// Used in KDE2-dependent "getDirectories" code.

KGoldrunner::KGoldrunner(QWidget *parent, const char *name, WFlags f)
 	: QMainWindow (parent, name, f)
#endif
{
	// Avoid "saveOK()" check if an error-exit occurs during file checks.
	editMode = FALSE;
	exitWarning = TRUE;

	// Get directory paths for the system levels, user levels and manual.
	if (! getDirectories())
	    myQuit();				// If not found, abort.
	printf ("manual: %s\nsystem: %s\nuser:   %s\n", systemHTMLDir.myStr(),
			systemDataDir.myStr(), userDataDir.myStr());

	// Initialise the collections of levels.
	collections.setAutoDelete(TRUE);
	owner = SYSTEM;				// Use system levels initially.
	if (! loadCollections (SYSTEM))		// Load system collections list.
	    myQuit();				// If no collections, abort.
	loadCollections (USER);			// Load user collections list.
						// If none, don't worry.
	mapCollections();			// Check ".grl" file integrity.

	// Set default collection (first in the SYSTEM "games.dat" file).
	collnOffset = 0;
	collnIndex = 0;
	collection = collections.at(collnOffset + collnIndex);
	level = 1;

/******************************************************************************/
/*********************  SET MENUS AND APPLICATION CONTROL  ********************/
/******************************************************************************/

	setCaption ("KGoldrunner");

	file_menu = new QPopupMenu();
#ifdef QT1
//	file_menu->insertItem(klocale->translate("&Neues Spiel"),ID_NEW);
	file_menu->insertItem(klocale->translate("&Start Game"),ID_NEW);
	file_menu->insertItem(klocale->translate("&Play Any Level"),ID_OPEN);
	file_menu->insertItem(klocale->translate("Play &Next"),ID_NEXT);
	file_menu->insertItem(klocale->translate("&Kill Hero"),ID_KILL);
	file_menu->setAccel (Key_K, ID_KILL);

	file_menu->insertSeparator();
	file_menu->insertItem (klocale->translate("Start &Tutorial"), ID_TUTE);
	file_menu->insertItem (klocale->translate("Get a &Hint"), ID_HINT);

	file_menu->insertSeparator();
	file_menu->insertItem (klocale->translate("Load Game"), ID_LOADGAME);
	file_menu->insertItem (klocale->translate("Save Game"), ID_SAVEGAME);
	file_menu->insertItem (klocale->translate("Show High Scores"), ID_HIGH);
	file_menu->setAccel (Key_S, ID_SAVEGAME);

	file_menu->insertSeparator();
	file_menu->insertItem (klocale->translate("Save Edits"), ID_SAVEFILE);

	file_menu->insertSeparator();
//	file_menu->insertItem(klocale->translate("&Schließen"),ID_EXIT);
	file_menu->insertItem(klocale->translate("&Quit"),ID_EXIT);
#else
	setUsesBigPixmaps (FALSE);		// Set small toolbar buttons.

	file_menu->insertItem("&Start Game",ID_NEW);
	file_menu->insertItem("&Play Any Level",ID_OPEN);
	file_menu->insertItem("Play &Next",ID_NEXT);
	file_menu->insertItem("&Kill Hero",ID_KILL);
	file_menu->setAccel (Key_K, ID_KILL);

	file_menu->insertSeparator();
	file_menu->insertItem ("Start &Tutorial", ID_TUTE);
	file_menu->insertItem ("Get a &Hint", ID_HINT);

	file_menu->insertSeparator();
	file_menu->insertItem ("Load Game", ID_LOADGAME);
	file_menu->insertItem ("Save Game", ID_SAVEGAME);
	file_menu->insertItem ("Show High Scores", ID_HIGH);
	file_menu->setAccel (Key_S, ID_SAVEGAME);

	file_menu->insertSeparator();
	file_menu->insertItem ("Save Edits", ID_SAVEFILE);

	file_menu->insertSeparator();
	file_menu->insertItem("&Quit",ID_EXIT);
#endif

#ifdef QT1
	menuBar = new KMenuBar(this, "menubar");
//	menuBar->insertItem(klocale->translate("&Datei"), file_menu);
	menuBar->insertItem(klocale->translate("&Game"), file_menu);
#else
//	menuBar()->insertItem("&Datei", file_menu);
	menuBar()->insertItem("&Game", file_menu);
#endif

	opt_menu = new QPopupMenu();
	opt_menu->setCheckable (TRUE);
#ifdef QT1
	opt_menu->insertItem(klocale->translate("&KGoldrunner Defaults"), ID_KGR);
	opt_menu->insertItem(klocale->translate("&Traditional Defaults"), ID_TRAD);
	opt_menu->insertSeparator();
	opt_menu->insertItem(klocale->translate("More Enemies Slower"), ID_MESLOW);
	opt_menu->insertItem(klocale->translate("Always Collect Gold"), ID_ACGOLD);
	opt_menu->insertItem(klocale->translate("Run through Hole"), ID_RTHOLE);
	opt_menu->insertItem(klocale->translate("Reappear at Top"), ID_RATOP);
	opt_menu->insertItem(klocale->translate("Search Strategy - Low"), ID_SRCH1);
	opt_menu->insertItem(klocale->translate("Search Strategy - Med"), ID_SRCH2);
	opt_menu->insertSeparator();
	opt_menu->insertItem(klocale->translate("Normal Speed"), ID_NSPEED);
	opt_menu->insertItem(klocale->translate("Beginner Speed"), ID_BSPEED);
	opt_menu->insertItem(klocale->translate("Novice Speed"), ID_VSPEED);
	opt_menu->insertItem(klocale->translate("Champion Speed"), ID_CSPEED);
	opt_menu->insertItem(klocale->translate("Increase Speed"), ID_ISPEED);
	opt_menu->insertItem(klocale->translate("Decrease Speed"), ID_DSPEED);
	opt_menu->setAccel(Key_Plus, ID_ISPEED);
	opt_menu->setAccel(Key_Minus, ID_DSPEED);

	menuBar->insertSeparator();
	menuBar->insertItem(klocale->translate("&Settings"), opt_menu, ID_OPT);
#else
	opt_menu->insertItem("&KGoldrunner Defaults", ID_KGR);
	opt_menu->insertItem("&Traditional Defaults", ID_TRAD);
	opt_menu->insertSeparator();
	opt_menu->insertItem("More Enemies Slower", ID_MESLOW);
	opt_menu->insertItem("Always Collect Gold", ID_ACGOLD);
	opt_menu->insertItem("Run through Hole", ID_RTHOLE);
	opt_menu->insertItem("Reappear at Top", ID_RATOP);
	opt_menu->insertItem("Search Strategy - Low", ID_SRCH1);
	opt_menu->insertItem("Search Strategy - Med", ID_SRCH2);
	opt_menu->insertSeparator();
	opt_menu->insertItem("Normal Speed", ID_NSPEED);
	opt_menu->insertItem("Beginner Speed", ID_BSPEED);
	opt_menu->insertItem("Novice Speed", ID_VSPEED);
	opt_menu->insertItem("Champion Speed", ID_CSPEED);
	opt_menu->insertItem("Increase Speed", ID_ISPEED);
	opt_menu->insertItem("Decrease Speed", ID_DSPEED);
	opt_menu->setAccel(Key_Plus, ID_ISPEED);
	opt_menu->setAccel(Key_Minus, ID_DSPEED);

	menuBar()->insertSeparator();
	menuBar()->insertItem("&Settings", opt_menu, ID_OPT);
#endif
	// Tick the rules for the default game (or collection of levels).
	rCheck ((collection->settings == 'K') ? ID_KGR : ID_TRAD);

	// Tick the default speed (which is set when the hero is created).
	sCheck (ID_NSPEED);

	edit_menu = new QPopupMenu();
#ifdef QT1
	edit_menu->insertItem(klocale->translate("&Create Level"),ID_CREATE);
	edit_menu->insertItem(klocale->translate("&Edit Any Level"),ID_UPDATE);
	edit_menu->insertItem(klocale->translate("Edit &Next"),ID_EDITNEXT);
	edit_menu->insertSeparator();
	edit_menu->insertItem(klocale->translate("&Save Level"),ID_SAVEFILE);
	edit_menu->insertItem(klocale->translate("&Move Level"),ID_MOVEFILE);
	edit_menu->insertItem(klocale->translate("&Delete Level"),ID_DELEFILE);
	edit_menu->insertSeparator();
	edit_menu->insertItem(klocale->translate("Create a Game"),ID_NCOLL);
	edit_menu->insertItem(klocale->translate("Edit Game Info"),ID_ECOLL);
	menuBar->insertSeparator();
	menuBar->insertItem(klocale->translate("&Editor"),edit_menu,ID_EDITMENU);
#else
	edit_menu->insertItem("&Create Level",ID_CREATE);
	edit_menu->insertItem("&Edit Any Level",ID_UPDATE);
	edit_menu->insertItem("Edit &Next",ID_EDITNEXT);
	edit_menu->insertSeparator();
	edit_menu->insertItem("&Save Level",ID_SAVEFILE);
	edit_menu->insertItem("&Move Level",ID_MOVEFILE);
	edit_menu->insertItem("&Delete Level",ID_DELEFILE);
	edit_menu->insertSeparator();
	edit_menu->insertItem("Create a Game",ID_NCOLL);
	edit_menu->insertItem("Edit Game Info",ID_ECOLL);
	menuBar()->insertSeparator();
	menuBar()->insertItem("&Editor", edit_menu, ID_EDITMENU);
#endif

	about = "KGoldrunner v1.0 (01/2002)\n\n(C) ";
#ifdef QT1
	about += (QString) klocale->translate("von") +
	  " Marco Krüger, grisuji@gmx.de\n\n";
	about += (QString) klocale->translate("Extended to v1.0\nby");
	about += (QString) " Ian Wadham, ianw@netspace.net.au";
	menuBar->insertSeparator();
	menuBar->insertItem( klocale->translate("&Help"),
			     KApplication::getKApplication()->getHelpMenu(TRUE, about ) );
#else
	about += (QString) "von Marco Krüger, grisuji@gmx.de\n\n";
	about += (QString) "Extended to v1.0\nby";
	about += (QString) " Ian Wadham, ianw@netspace.net.au";
	menuBar()->insertSeparator();
	help_menu = new QPopupMenu ();
	help_menu->insertItem ("&Tutorial", ID_TUTE);
	help_menu->insertItem ("&Hint", ID_HINT);
	help_menu->insertItem ("&User's Guide", ID_MANUAL);
	help_menu->insertSeparator();
	help_menu->insertItem ("&About KGoldrunner", ID_ABOUTKGR);
	help_menu->insertItem ("About &Qt Toolkit", ID_ABOUTQT);
	menuBar()->insertSeparator();
	menuBar()->insertItem ("&Help", help_menu, ID_HELPMENU);
#endif

	// Set up the Pause/Resume button and connect the menu signals.
	pauseBtn = new QPushButton ("PAUSE (Esc)", this);
	pauseBtn->setFixedSize (90, 25);
	pauseBtn->setAccel (Key_Escape);
#ifdef QT1
	pauseBtn->move (417, 0);
	connect(menuBar, SIGNAL(activated(int)), SLOT(commandCallback(int)));
	connect(pauseBtn, SIGNAL(clicked()), SLOT(pauseResume()));
#else
	menuBar()->insertSeparator();
	menuBar()->insertItem (pauseBtn, ID_PAUSE);
	connect(menuBar(), SIGNAL(activated(int)), SLOT(commandCallback(int)));
	connect(pauseBtn, SIGNAL(clicked()), SLOT(pauseResume()));
#endif

	// Allow a clean exit when the user clicks "X" at top right of window.
	exitWarning = FALSE;
#ifdef QT1
	connect (kapp, SIGNAL(lastWindowClosed()), this, SLOT(myQuit()));
#else
	connect (qApp, SIGNAL(lastWindowClosed()), this, SLOT(myQuit()));
#endif

	// Set the game editor mode as OFF, but available.
	editMode = FALSE;
	paintEditObj = FALSE;
	editObj  = BRICK;

	edit_menu->setItemEnabled (ID_SAVEFILE, FALSE);
	file_menu->setItemEnabled (ID_SAVEFILE, FALSE);

#ifdef QT1
	loader = kapp->getIconLoader();
#endif

#ifdef QT1
	statusBar = new KStatusBar(this);
	initStatusBar();
	setStatusBar(statusBar);
	statusBar->show();
#else
	initStatusBar();
#endif

	makeEditToolbar();

/******************************************************************************/
/************************  SET PLAYFIELD AND GAME DATA  ***********************/
/******************************************************************************/

	file_menu->setItemEnabled (ID_SAVEGAME, FALSE);

	enemies.setAutoDelete(TRUE);

	setBackgroundMode (NoBackground);
	view = new KGoldrunnerWidget(this);
	int w = (FIELDWIDTH*4+16)*STEP;
	int h = (FIELDHEIGHT*4+16)*STEP;
	view->setFixedSize (w, h);
#ifdef QT1
	setView (view);
#else
	setCentralWidget (view);
#endif

	setBlankLevel (TRUE);	// Fill the playfield with blank walls.
	view->show();
	view->repaint();	// Paint the border of the playfield.

#ifdef QT1
	setMenu (menuBar);
	editToolbar->hide();
#else
	removeToolBar (editToolbar);
	setDockEnabled (QMainWindow::Bottom, FALSE);
	setDockEnabled (QMainWindow::Left, FALSE);
	setDockEnabled (QMainWindow::Right, FALSE);

	// Force QMainWindow to recalculate its widget sizes immediately.
	qApp->processEvents();

	// Force QMainWindow to release the toolbar space and keep the layout
	// clean.  Not nice, but I've tried everything else in the Qt book.
	// We also stop the user resizing the main window (it is not supported
	// by KGoldrunner and only creates ugly grey areas).
	this->setFixedSize (this->minimumSize());
#endif

	hero = new KGrHero (QPixmap (hero_xpm), 0, 0);
	connect (hero, SIGNAL (gotNugget(int)),   SLOT (incScore(int)));
	connect (hero, SIGNAL (haveAllNuggets()), SLOT (showHidden()));
	connect (hero, SIGNAL (caughtHero()),     SLOT (herosDead()));
      	connect (hero, SIGNAL (leaveLevel()),     SLOT (nextLevel()));
	hero->setPlayfield (&playfield);

	// Get the mouse position every 40 msec.  It is used to steer the hero.
	mouseSampler = new QTimer (this);
	connect (mouseSampler, SIGNAL(timeout()), SLOT (readMousePos ()));
	mouseSampler->start (40, FALSE);

    	enemy = NULL;
	newLevel = TRUE;			// Next level will be a new one.
	loading  = TRUE;			// Stop input until it's loaded.
	
	srand(1); // initialisiere Random-Generator

	// Paint the main widget (title, menu, status bar, blank playfield).
	this->show();

	// Force the main widget to appear before the "Start Game" dialog does.
#ifdef QT1
	kapp->processEvents();
#else
	qApp->processEvents();
#endif

	// Call the "Start Game" menu item and pop up the "Start Game" dialog.
	modalFreeze = FALSE;
	messageFreeze = FALSE;
	commandCallback (ID_NEW);
}

KGoldrunner::~KGoldrunner()
{
	delete editToolbar;
	delete file_menu;
	delete edit_menu;
#ifdef QT1
	delete menuBar;
	delete statusBar;
#endif
}

bool KGoldrunner::getDirectories ()
{
    // This procedure is HIGHLY DEPENDENT on the operating system and
    // desktop environment and will require changes when porting.  Note that
    // the Qt library has compile-time symbols defined for various window
    // systems (e.g. _WS_X11_ and _WS_WIN_).  See "Windowsystem-Specific Notes"
    // in the Qt 2 On-line Reference Documentation, but you also have to think
    // about installation packages and where they put things (configuration).

    bool result = TRUE;

#ifdef QT1
    // This piece of code depends on KDE 1 class libraries and Qt 1.  Note
    // that, in the KDE1/QT1 version of KGoldrunner, the manual is obtained
    // from a KDE Help class built into the KDE Application.

    systemDataDir = KApplication::kde_datadir() + "/kgoldrun/";
    userDataDir   = KApplication::localkdedir() + "/share/apps/kgoldrun/";

    // Create the user's "kgoldrun" and "levels" directories, if not present.
    QDir d (userDataDir.myStr());
    if (! d.exists()) {
	if (! d.mkdir (userDataDir.myStr(), TRUE)) {
	    QMessageBox::information (this, "KGoldrunner - Get Directories",
	    "Cannot create directory \"" + userDataDir + "\"\n"
	    "in your HOME work-area.  You will need it if you want to create\n"
	    "KGoldrunner games.  Please contact your System Administrator.");
	}
    }
    QString s = userDataDir + "levels";
    d.setPath (s.myStr());
    if (! d.exists()) {
	if (! d.mkdir (s.myStr(), TRUE)) {
	    QMessageBox::information (this, "KGoldrunner - Get Directories",
	    "Cannot create directory \"" + s + "\"\n"
	    "in your HOME work-area.  You will need it if you want to create\n"
	    "KGoldrunner games.  Please contact your System Administrator.");
	}
    }

#else
    // This piece of code depends (minimally) on KDE 2 class libraries and Qt 2.

    // WHERE THINGS ARE: In the KDE 2 environment (Release 2.1.2), application
    // documentation and data files are in a directory structure given by
    // $KDEDIRS (e.g. "/opt/kde2/").  Application user data files are in
    // a directory structure given by $KDEHOME ("$HOME/.kde2").  Within
    // those two structures, the three sub-directories will typically be
    // "share/doc/HTML/en/kgoldrun/", "share/apps/kgoldrun/system/" and
    // "share/apps/kgoldrun/user/".  Note that it is necessary to have
    // an extra path level ("system" or "user") after "kgoldrun", otherwise
    // all the KGoldrunner files have similar path names (after "apps") and
    // KDE always locates directories in $KDEHOME and never the released games.

    // The directory strings are set by KDE 2 at run time and might change in
    // later releases, so use them with caution and only if something gets lost.

    KStandardDirs * dirs = new KStandardDirs();

    // Find the KGoldrunner Users' Guide, English version (en).
    systemHTMLDir = dirs->findResourceDir ("html", "en/kgoldrun/");
    if (systemHTMLDir.length() <= 0) {
	QMessageBox::information (this, "KGoldrunner - Get Directories",
	"Cannot find documentation sub-directory \"en/kgoldrun/\"\n"
	"in area \"" + dirs->kde_default ("html") +
	"\" of the KDE directories ($KDEDIRS).\n"
	"Please contact your System Administrator.");
	result = FALSE;
    }
    else
	systemHTMLDir.append ("en/kgoldrun/");

    // Find the system collections in a directory of the required KDE type.
    systemDataDir = dirs->findResourceDir ("data", "kgoldrun/system/");
    if (systemDataDir.length() <= 0) {
	QMessageBox::information (this, "KGoldrunner - Get Directories",
	"Cannot find System Levels sub-directory \"kgoldrun/system/\"\n"
	"in area \"" + dirs->kde_default ("data") +
	"\" of the KDE directories ($KDEDIRS).\n"
	"Please contact your System Administrator.");
	result = FALSE;
    }
    else
	systemDataDir.append ("kgoldrun/system/");

    // Locate and optionally create directories for user collections and levels.
    bool create = TRUE;
    userDataDir   = dirs->saveLocation ("data", "kgoldrun/user/", create);
    if (userDataDir.length() <= 0) {
	QMessageBox::information (this, "KGoldrunner - Get Directories",
	"Cannot find or create User Levels sub-directory \"kgoldrun/user/\"\n"
	"in area \"" + dirs->kde_default ("data") +
	"\" of the KDE user area ($KDEHOME).\n"
	"Please contact your System Administrator.");
	result = FALSE;
    }
    else {
	create = dirs->makeDir (userDataDir + "levels/");
	if (! create) {
	    QMessageBox::information (this, "KGoldrunner - Get Directories",
	    "Cannot find or create \"levels/\" directory in sub-directory\n"
	    "\"kgoldrun/user/\" in the KDE user area ($KDEHOME).\n"
	    "Please contact your System Administrator.");
	    result = FALSE;
	}
    }
#endif

    return (result);
}

// This slot is activated when top-level window is closed, whether by selecting
// "Quit" from the menu or by clicking the "X" at the top right of the window.

void KGoldrunner::myQuit()
{
    if (exitWarning) {
	// Exit via menu item "Quit": need to save has already been checked.
	exit(0);
    }
    else {
	// Last chance to save: user may have clicked "X" widget accidentally.
	exitWarning = TRUE;
	if (saveOK()) {
	    exit(0);
	}
    }
}

QString KGoldrunner::getTitle()
{
    // Generate title string "Collection-name - Level NNN, Level-name".
    QString levelTitle;
    if (level == 0) {
	if (! editMode)
	    levelTitle = "E N D --- F I N --- E N D E";
	else
	    levelTitle = "New Level";
    }
    else if (levelName.length() > 0)
	levelTitle.sprintf ("%s - Level %03d - %s",
			    collection->name.myStr(), level, levelName.myStr());
    else
	levelTitle.sprintf ("%s - Level %03d",
			    collection->name.myStr(), level);
    return (levelTitle);
}

/******************************************************************************/
/**************************  GAME CONTROL PROCEDURES  *************************/
/******************************************************************************/

void KGoldrunner::keyPressEvent(QKeyEvent *k)
{
    int	i;

    // If loading a level for play or editing, ignore keyboard input.
    if (loading) return;

    if (editMode) {
      // Set the type of editing object to be painted by clicking the mouse.
      switch(k->key()) {
      case Key_Space:		// Empty space.
	    editObj = FREE; i = ID_FREE; break;
      case Key_E:		// Enemy.
	    editObj = ENEMY; i = ID_ENEMY; break;
      case Key_R:		// Hero.
	    editObj = HERO; i = ID_HERO; break;
      case Key_X:		// Concrete (not diggable).
	    editObj = BETON; i = ID_BETON; break;
      case Key_M:		// Brick (diggable).
	    editObj = BRICK; i = ID_BRICK; break;
      case Key_F:		// Fall-through brick.
	    editObj = FBRICK; i = ID_FBRICK; break;
      case Key_H:		// Ladder.
	    editObj = LADDER; i = ID_LADDER; break;
      case Key_Z:		// Hidden ladder (appears at end of level).
	    editObj = HLADDER; i = ID_HLADDER; break;
      case Key_N:		// Nugget (gold).
	    editObj = NUGGET; i = ID_NUGGET; break;
      case Key_T:		// Horizontal bar (pole).
	    editObj = POLE; i = ID_POLE; break;
      default:
	    i = 0; break;
      }
      // Switch the toggles on the Edit Toolbar buttons.
      if (i != 0) {
#ifdef QT1
	editToolbar->setButton (pressedButton, FALSE);
	pressedButton = i;
	editToolbar->setButton (pressedButton, TRUE);
#else
	switch (i) {
	case ID_FREE:		freeSlot(); break;
	case ID_ENEMY:		edenemySlot(); break;
	case ID_HERO:		edheroSlot(); break;
	case ID_BETON:		betonSlot(); break;
	case ID_BRICK:		brickSlot(); break;
	case ID_FBRICK:		fbrickSlot(); break;
	case ID_LADDER:		ladderSlot(); break;
	case ID_HLADDER:	hladderSlot(); break;
	case ID_NUGGET:		nuggetSlot(); break;
	case ID_POLE:		poleSlot(); break;
	default:		break;
	}
#endif
      }
      return;
    }

    // If first player-input and not Escape or S, start playing.  Escape and S
    // are exceptions so that the player can save the game without starting it.
    if ((! hero->started) && (k->key() != Key_Escape) && (k->key() != Key_S)) {
	startPlaying();
    }

    switch (k->key()) {

    // The following keys were the original keyboard controls for KGoldrunner.
    // They have been commented out since mouse control was implemented in v1.0,
    // because they do not work properly now.  If they are needed in the future,
    // they would have to be backed up by a menu item to disable/enable mouse
    // or keyboard mode and the supporting code changes.

    // case Key_Up: hero->setKey(UP);break;
    // case Key_Right: hero->setKey(RIGHT);break;
    // case Key_Down: hero->setKey(DOWN);break;
    // case Key_Left: hero->setKey(LEFT);break;
    // case Key_Control:
    // case Key_Space: hero->dig();break;
    // case Key_S: hero->digLeft();break;
    // case Key_F: hero->digRight();break;

    case Key_K:     herosDead(); break;			// Kill the hero.
    case Key_S:     saveGame(); break;			// Save the game.
    case Key_Plus:  hero->setSpeed(+1); break;		// Increase game speed.
    case Key_Minus: hero->setSpeed(-1); break;		// Decrease game speed.

    // Halt or restart play.
    case Key_Escape: if (!(KGrObj::frozen)) freeze(); else unfreeze(); break;

    // The following keys are for authors' debugging aids.
    case Key_Period:					// Do one step of
		if (KGrObj::frozen) doStep(); break;	// animation.
    case Key_B: if (KGrObj::bugFixed) {			// Turn a bug fix
		    KGrObj::bugFixed = FALSE;}		// on/off dynamically.
	        else {
		    KGrObj::bugFixed = TRUE;} break;
    case Key_D: if (KGrObj::frozen) showFigurePositions(); break;
    case Key_L: if (KGrObj::frozen)			// Turn logging on/off.
		    KGrObj::logging = (KGrObj::logging)? FALSE:TRUE; break;

    case Key_H: if (KGrObj::frozen) showHeroState(); break;
    case Key_0: if (KGrObj::frozen) showEnemyState(0); break;
    case Key_1: if (KGrObj::frozen) showEnemyState(1); break;
    case Key_2: if (KGrObj::frozen) showEnemyState(2); break;
    case Key_3: if (KGrObj::frozen) showEnemyState(3); break;
    case Key_4: if (KGrObj::frozen) showEnemyState(4); break;
    case Key_5: if (KGrObj::frozen) showEnemyState(5); break;
    case Key_6: if (KGrObj::frozen) showEnemyState(6); break;
    case Key_O: if (KGrObj::frozen) showObjectState(); break;

    default: break;
    }
}

void KGoldrunner::setBlankLevel(bool playable)
{
    for (int j=0;j<20;j++)
      for (int i=0;i<28;i++) {
	if (playable) {
	    playfield[i+1][j+1] = new KGrFree (freebg, nuggetbg, false, view);
	}
	else {
	    playfield[i+1][j+1] = new KGrEditable (freebg, view);
	}
	editObjArray[i+1][j+1] = FREE;
      }
    for (int j=0;j<30;j++) {
      playfield[j][0]=new KGrBeton(QPixmap ());
      editObjArray[j][0] = BETON;
      playfield[j][21]=new KGrBeton(QPixmap ());
      editObjArray[j][21] = BETON;
    }
    for (int i=0;i<22;i++) {
      playfield[0][i]=new KGrBeton(QPixmap ());
      editObjArray[0][i] = BETON;
      playfield[29][i]=new KGrBeton(QPixmap ());
      editObjArray[29][i] = BETON;
    }
    for (int j=0;j<22;j++)
      for (int i=0;i<30;i++) {
	playfield[i][j]->move(16+i*16,16+j*16);
    }
}

void KGoldrunner::startLevel (int action)
{
    // Use dialog box to select collection and level: action ID_NEW or ID_OPEN.
    int	lev = selectLevel (action);
    if (lev > 0) {
	newGame (lev);		// If OK, start the game at that level.
    }
}

void KGoldrunner::newGame (const int lev)
{
    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = TRUE;		// "loadLevel (level)" will reset it.

    if (editMode) {
	editMode = FALSE;
	paintEditObj = FALSE;
	editObj = BRICK;

	edit_menu->setItemEnabled(ID_SAVEFILE, FALSE);
	file_menu->setItemEnabled(ID_SAVEFILE, FALSE);

#ifdef QT1
	editToolbar->hide();
#else
	removeToolBar (editToolbar);

	// Force QMainWindow to recalculate its widget sizes immediately.
	qApp->processEvents();

	// Force QMainWindow to release the toolbar space and keep the layout
	// clean.  Not nice, but I've tried everything else in the Qt book.
	this->setFixedSize (this->minimumSize());
#endif
    } // End "if (editMode)".

#ifdef QT1
    setMenu(menuBar);
#endif

    newLevel = TRUE;
    level = lev;

    lifes = 5;				// Start with 5 lives.
    score = 0;

    changeLifes (lifes);
    changeScore (score);
    changeLevel (level);

    enemyCount = 0;
    enemies.clear();

    newLevel = TRUE;;
    loadLevel (level);
    newLevel = FALSE;
}

void KGoldrunner::startTutorial()
{
    int i, offset, index;
    int imax = collections.count();
    bool found = FALSE;

    offset = 0;
    index = 0;
    for (i = 0; i < imax; i++) {
	if ((offset == 0) && (collections.at(i)->owner != SYSTEM)) {
	    // Note offset of USER collections, if the search gets that far.
	    offset = i;
	}
	index = i - offset;				// Index within owner.
	if (collections.at(i)->prefix == "tute") {
	    found = TRUE;
	    break;
	}
    }
    if (found) {
	// Start the tutorial.
	collection = collections.at (offset + index);
	owner = collection->owner;
	rCheck ((collection->settings == 'K') ? ID_KGR : ID_TRAD);
	collnOffset = offset;
	collnIndex = index;
	level = 1;
	newGame (level);
    }
    else {
	QMessageBox::information (this, "Start Tutorial",
	    "Cannot find the Tutorial game (file-prefix \"tute\") in\n"
	    "the \"games.dat\" files.  Please contact your System\n"
	    "Administrator.");
    }
}

void KGoldrunner::showHint()
{
    // Put out a hint for this level.
    QString caption = "HINT for " + getTitle();

    if (levelHint.length() > 0)
	myMessage (this, caption, levelHint);
    else
	myMessage (this, caption, "Sorry, there is no hint for this level.");
}

int KGoldrunner::loadLevel (int levelNo)
{
  int i,j;
  QFile openlevel;

  if (! openLevelFile (levelNo, openlevel)) {
      return 0;
  }

  // Ignore player input from keyboard or mouse while the screen is set up.
  loading = TRUE;

  // Turn on the "Save Game" menu option.
  file_menu->setItemEnabled (ID_SAVEGAME, TRUE);
  nuggets = 0;
  enemyCount=0;

  // lade den Level
  for (j=1;j<21;j++)
    for (i=1;i<29;i++) {
	changeObject(openlevel.getch(),i,j);
    }

  // Absorb a newline character, then read in the level name and hint (if any).
  int c = openlevel.getch();
  levelName = "";
  levelHint = "";
  i = 1;
  while ((c = openlevel.getch()) != EOF) {
      switch (i) {
      case 1:	if (c == '\n')			// Level name is on one line.
		    i = 2;
		else
		    levelName += (char) c;
		break;

      case 2:	levelHint += (char) c;		// Hint is on rest of file.
		break;
      }
  }

  // Indicate on the menus whether there is a hint for this level.
#ifdef QT1
  if (levelHint.length() > 0) {
      file_menu->changeItem (klocale->translate("Get a &Hint"), ID_HINT);
  }
  else {
      file_menu->changeItem (klocale->translate("NO &Hint"), ID_HINT);
  }
#else
  if (levelHint.length() > 0) {
      file_menu->changeItem (ID_HINT, "Get a &Hint");
      help_menu->changeItem (ID_HINT, "&Hint");
  }
  else {
      file_menu->changeItem (ID_HINT, "NO &Hint");
      help_menu->changeItem (ID_HINT, "NO &Hint");
  }
#endif

  // Disconnect edit-mode slots from signals from border of "view".
  disconnect (view, SIGNAL(mouseClick(int)), 0, 0);
  disconnect (view, SIGNAL(mouseLetGo(int)), 0, 0);

  // Connect play-mode slot to signal from border of "view".
  connect (view, SIGNAL(mouseClick(int)), SLOT(doDig(int)));

  if (newLevel) {
      hero->setEnemyList (&enemies);
      for (enemy=enemies.first();enemy != 0; enemy = enemies.next())
	enemy->setEnemyList(&enemies);
  }

  hero->setNuggets(nuggets);
  setTimings();

  // Set direction-flags to use during enemy searches.
  initSearchMatrix();

  // Re-draw the playfield frame, level title and figures.
  view->repaint();

  // Check if this is a tutorial collection and we are not on the "ENDE" screen.
  if ((collection->prefix.left(4) == "tute") && (levelNo != 0)) {
      // At the start of a tutorial, put out an introduction.
      if (levelNo == 1)
	  myMessage (this, collection->name, collection->about);

      // Put out an explanation of this level.
      myMessage (this, getTitle(), levelHint);
  }

  // Put the mouse pointer on the hero.
  playfield[startI][startJ]->setMousePos();

  // Make sure the figures appear.
#ifdef QT1
  kapp->processEvents();
#else
  qApp->processEvents();
#endif
  this->repaint();

  // Re-enable player input.
  loading = FALSE;

  return 1;
}

bool KGoldrunner::openLevelFile (int levelNo, QFile & openlevel)
{
  QString filePath;
  QString levelOwner;

  filePath = getFilePath (owner, collection, levelNo);

  openlevel.setName (filePath);

  // gucken ob und welcher Level existiert

  if (! openlevel.exists()) {
#ifdef QT1
	  KMsgBox::message(this,klocale->translate("lade Level"),
			   klocale->translate("Dieser Level ist nicht vorhanden.\nBitte fragen sie Ihren Systemadministrator"));
#else
      QMessageBox::information (this, "Load Level",
	    "Cannot find file \"" + filePath +
	    "\"\nPlease contact your System Administrator.");
#endif
      return (FALSE);
  }

  // öffne Level zum lesen
  if (! openlevel.open (IO_ReadOnly)) {
#ifdef QT1
      KMsgBox::message(this,klocale->translate("lade Level"),
		       klocale->translate("Fehler beim öffnen des Levels"));
#else
      QMessageBox::information (this, "Load Level",
	    "Cannot open file \"" + filePath +
	    "\"\nfor read-only.  Please contact your System Administrator.");
#endif
      return (FALSE);
  }

  levelOwner = (owner == SYSTEM) ? "System Level" : "User Level";
#ifdef QT1
  statusBar->changeItem(klocale->translate(levelOwner),ID_LEVELPLACE);
#else
  levelOwner = levelOwner.leftJustify (L_GROUP, ' ');
  statusString.replace (ID_GROUP, L_GROUP, levelOwner);
  statusBar()->message (statusString);
#endif

  return (TRUE);
}

void KGoldrunner::changeObject (unsigned char kind, int i, int j)
{
  delete playfield[i][j];
  switch(kind) {
  case FREE: createObject(new KGrFree(freebg,nuggetbg,false,view),i,j);break;
  case LADDER: createObject(new KGrLadder(ladderbg,view),i,j);break;
  case HLADDER: createObject(new KGrHladder(freebg,nuggetbg,ladderbg,false,
				view),i,j);break;
  case BRICK: createObject(new KGrBrick(digpix,view),i,j);break;
  case BETON: createObject(new KGrBeton(betonbg,view),i,j);break;
  case FBRICK: createObject(new KGrFbrick(brickbg,view),i,j);break;
  case POLE: createObject(new KGrPole(polebg,view),i,j);break;
  case NUGGET: createObject(new KGrFree(freebg,nuggetbg,true,view),i,j);
		nuggets++;break;
  case HERO: createObject(new KGrFree(freebg,nuggetbg,false,view),i,j);
    hero->init(i,j);
    startI = i; startJ = j;
    hero->started = FALSE;
    hero->showFigure();
    break;
  case ENEMY: createObject(new KGrFree(freebg,nuggetbg,false,view),i,j);
    if (newLevel){
      // Starting a level for the first time.
      enemy = new KGrEnemy (QPixmap(enemy1_xpm), QPixmap(enemy2_xpm), i, j);
      enemy->setPlayfield(&playfield);
      enemy->enemyId=enemyCount++;
      enemies.append(enemy);
      connect(enemy, SIGNAL(lostNugget()), SLOT(loseNugget()));
      connect(enemy, SIGNAL(trapped(int)), SLOT(incScore(int)));
      connect(enemy, SIGNAL(killed(int)),  SLOT(incScore(int)));
    } else {
      // Starting a level again after losing.
      enemy=enemies.at(enemyCount);
      enemy->enemyId=enemyCount++;
      enemy->setNuggets(0);
      enemy->init(i,j);	// Re-initialise the enemy's state information.
    }
    enemy->showFigure();
    break;
  default :  createObject(new KGrBrick(digpix,view),i,j);break;
  }
}

void KGoldrunner::createObject (KGrObj *o, int x, int y)
{
    playfield[x][y] = o;
    playfield[x][y]->move(32+(x-1)*16,32+(y-1)*16);
    playfield[x][y]->show();

    // Connect play-mode slot to signal from playfield element.
    // Note: Edit-mode slots were disconnected when element was deleted.
    connect (o, SIGNAL(mouseClick(int)), SLOT(doDig(int)));
}

void KGoldrunner::setTimings ()
{
    Timing *	timing;
    int		c = -1;

    if (KGrFigure::variableTiming) {
	c = enemies.count();			// Timing based on enemy count.
	c = (c > 5) ? 5 : c;
	timing = &(KGrFigure::varTiming[c]);
    }
    else {
	timing = &(KGrFigure::fixedTiming);	// Fixed timing.
    }

    KGrHero::WALKDELAY		= timing->hwalk;
    KGrHero::FALLDELAY		= timing->hfall;
    KGrEnemy::WALKDELAY		= timing->ewalk;
    KGrEnemy::FALLDELAY		= timing->efall;
    KGrEnemy::CAPTIVEDELAY	= timing->ecaptive;
    KGrBrick::HOLETIME		= timing->hole;
}

void KGoldrunner::initSearchMatrix()
{
  // Called at start of level and also when hidden ladders appear.
  int i,j;

  for (i=1;i<21;i++){
    for (j=1;j<29;j++)
      {
	// If on ladder, can walk L, R, U or D.
	if (playfield[j][i]->whatIam()==LADDER)
	  playfield[j][i]->searchValue = CANWALKLEFT + CANWALKRIGHT +
					CANWALKUP + CANWALKDOWN;
	else
	  // If on solid ground, can walk L or R.
	  if ((playfield[j][i+1]->whatIam()==BRICK)||
	      (playfield[j][i+1]->whatIam()==HOLE)||
	      (playfield[j][i+1]->whatIam()==USEDHOLE)||
	      (playfield[j][i+1]->whatIam()==BETON))
	    playfield[j][i]->searchValue=CANWALKLEFT+CANWALKRIGHT;
	  else
	    // If on pole or top of ladder, can walk L, R or D.
	    if ((playfield[j][i]->whatIam()==POLE)||
		(playfield[j][i+1]->whatIam()==LADDER))
	      playfield[j][i]->searchValue=CANWALKLEFT+CANWALKRIGHT+CANWALKDOWN;
	    else
	      // Otherwise, gravity takes over ...
	      playfield[j][i]->searchValue=CANWALKDOWN;

	// Clear corresponding bits if there are solids to L, R, U or D.
	if(playfield[j][i-1]->blocker)
	  playfield[j][i]->searchValue &= ~CANWALKUP;
	if(playfield[j-1][i]->blocker)
	  playfield[j][i]->searchValue &= ~CANWALKLEFT;
	if(playfield[j+1][i]->blocker)
	  playfield[j][i]->searchValue &= ~CANWALKRIGHT;
	if(playfield[j][i+1]->blocker)
	  playfield[j][i]->searchValue &= ~CANWALKDOWN;
      }
  }
}

void KGoldrunner::startPlaying () {
    if (! hero->started) {
	// Start the enemies and the hero.
	for (--enemyCount; enemyCount>=0; --enemyCount) {
	    enemy=enemies.at(enemyCount);
	    enemy->startSearching();
	}
	hero->start();
    }
}

void KGoldrunner::saveGame()
{
    if (editMode || hero->started) {
	myMessage (this, "Save Game",
	"Sorry, for reasons of simplicity, you cannot save a game while you "
	"are playing or editing.\n\n"
	"Please wait till you are about to start a level, then press the 'S' "
	"key or press the Escape key (Esc), to avoid starting play as you are "
	"moving the mouse, then select menu items Game and Save Game.");
	return;
    }

    QDate today = QDate::currentDate();
    QTime now =   QTime::currentTime();
    QString saved;
    QString day;
    day = today.dayName(today.dayOfWeek());
    saved = saved.sprintf
		("%-6s %03d %03ld %7ld    %s %04d-%02d-%02d %02d:%02d\n",
		collection->prefix.myStr(), level, lifes, score,
		day.myStr(),
		today.year(), today.month(), today.day(),
		now.hour(), now.minute());

    QFile file1 (userDataDir + "savegame.dat");
    QFile file2 (userDataDir + "savegame.tmp");

    if (! file2.open (IO_WriteOnly)) {
	QMessageBox::information (this, "Save Game",
		"Cannot open file \"" + userDataDir + "savegame.tmp"
		"\".\nfor output.  Please contact your System Administrator.");
	return;
    }
    QTextStream text2 (&file2);
    text2 << saved;

    if (file1.exists()) {
	if (! file1.open (IO_ReadOnly)) {
	    QMessageBox::information (this, "Save Game",
		"Cannot open file \"" + userDataDir + "savegame.dat"
		"\".\nfor input.  Please contact your System Administrator.");
	    return;
	}

	QTextStream text1 (&file1);
	int n = 30;			// Limit the file to the last 30 saves.
	while ((! text1.endData()) && (--n > 0)) {
	    saved = text1.readLine() + "\n";
	    text2 << saved;
	}
	file1.close();
    }

    file2.close();

    QDir dir;
    dir.rename (file2.name(), file1.name(), TRUE);
    QMessageBox::information (this, "Save Game", "Your game has been saved.");
}

void KGoldrunner::loadGame()
{
    QFile savedGames (userDataDir + "savegame.dat");
    if (! savedGames.exists()) {
	// Use myMessage() because it stops the game while the message appears.
	myMessage (this, "Load Game", "Sorry, there are no saved games.");
	return;
    }

    if (! savedGames.open (IO_ReadOnly)) {
	QMessageBox::information (this, "Load Game",
	    "Cannot open file \"" + userDataDir + "savegame.dat"
	    "\".\nfor input.  Please contact your System Administrator.");
	return;
    }

    // Halt the game during the loadGame() dialog.
    modalFreeze = FALSE;
    if (!KGrObj::frozen) {
	modalFreeze = TRUE;
	freeze();
    }

    QDialog *		lg       = new QDialog (view, "loadGameDialog", TRUE,
			WStyle_Customize | WStyle_NormalBorder | WStyle_Title);
    QLabel *		lgHeader = new QLabel (
			    "Game                       Level/Lives/Score   "
			    "Day   Date     Time", lg);
    QListBox *		lgList   = new QListBox (lg);
    QPushButton *	OK       = new QPushButton ("OK", lg);
    QPushButton *	CANCEL   = new QPushButton ("Cancel", lg);

    QPoint		p = view->mapToGlobal (QPoint (0,0));
    int			selectedGame = -1;
			lgHighlight  = -1;

    lg->	setCaption ("Select Game to be Loaded ...");

    lg->	setGeometry (p.x() + 50, p.y() + 50, 520, 290);
    lgHeader->	setGeometry (15, 10, 500, 20);
    lgList->	setGeometry (10, 40, 500, 200);
    OK->	setGeometry (10, 260, 90, 25);
    CANCEL->	setGeometry (210, 260, 90,  25);

    OK->	setAccel (Key_Return);
    CANCEL->	setAccel (Key_Escape);

    QFont	f ("courier", 12);
    f.setFixedPitch (TRUE);
    lgList->	setFont (f);
    f.setBold (TRUE);
    lgHeader->	setFont (f);

    QTextStream	gameText (&savedGames);
    QString	s = "";
    QString	pr = "";
    int		i;
    int		imax = collections.count();

    // Read the saved games into the list box.
    while (! gameText.endData()) {
	s = gameText.readLine();		// Read in one saved game.
	pr = s.left (s.find (" ", 0, FALSE));	// Get the collection prefix.
	for (i = 0; i < imax; i++) {		// Get the collection name.
	    if (collections.at(i)->prefix == pr) {
		s = s.insert (0,
		    collections.at(i)->name.leftJustify (20, ' ', TRUE) + " ");
		break;
	    }
	}
	lgList-> insertItem (s);
    }
    savedGames.close();

    // Mark row 0 (the most recently saved game) as the default selection.
    lgList->	setCurrentItem (0);
    lgList->	setSelected (0, TRUE);
		lgHighlight = 0;

    connect (lgList, SIGNAL (highlighted (int)), this, SLOT (lgSelect (int)));
    connect (OK,     SIGNAL (clicked ()),        lg,   SLOT (accept ()));
    connect (CANCEL, SIGNAL (clicked ()),        lg,   SLOT (reject ()));

    if (lg->exec() == QDialog::Accepted) {
	selectedGame = lgHighlight;
    }

    int  offset = 0;
    bool found = FALSE;
    int  lev;
    if (selectedGame >= 0) {
#ifdef QT1
	s = lgList->text(selectedGame);
#else
	s = lgList->currentText();
#endif
	pr = s.mid (21, 7);			// Get the collection prefix.
	pr = pr.left (pr.find (" ", 0, FALSE));
	for (i = 0; i < imax; i++) {		// Find the collection.
	    if ((collections.at(i)->owner != SYSTEM) && (offset == 0))
		offset = i;
	    if (collections.at(i)->prefix == pr) {
		collection = collections.at(i);
		collnOffset = offset;
		collnIndex  = i - offset;
		owner = collections.at(i)->owner;
		found = TRUE;
		break;
	    }
	}
	if (found) {
	    // Set the rules for the selected game.
	    rCheck ((collection->settings == 'K') ? ID_KGR : ID_TRAD);
	    lev   = s.mid (28, 3).toInt();
	    newGame (lev);			// Re-start the selected game.
	    lifes = s.mid (32, 3).toLong();	// Update the lives.
	    changeLifes (lifes);
	    score = s.mid (36, 7).toLong();	// Update the score.
	    changeScore (score);
	}
	else {
	    QMessageBox::information (this, "Load Game",
		"Cannot find the game with prefix \"" + pr +
		"\".\nPlease contact your System Administrator.");
	}
    }

    // Unfreeze the game, but only if it was previously unfrozen.
    if (modalFreeze) {
        unfreeze();
	modalFreeze = FALSE;
    }

    delete lg;
}

void KGoldrunner::lgSelect (int n)
{
    lgHighlight = n;
}

void KGoldrunner::checkHighScore()
{
    bool	prevHigh  = TRUE;
    Q_INT16	prevLevel = 0;
    Q_INT32	prevScore = 0;
    QString	thisUser  = "Unknown";
    int		highCount = 0;

    // Don't keep high scores for tutorial games.
    if (collection->prefix.left(4) == "tute")
	return;

    if (score <= 0)
	return;

    // Look for user's high-score file or for a released high-score file.
    QFile high1 (userDataDir + "hi_" + collection->prefix + ".dat");
    QDataStream s1;

    if (! high1.exists()) {
	high1.setName (systemDataDir + "hi_" + collection->prefix + ".dat");
	if (! high1.exists()) {
	    prevHigh = FALSE;
	}
    }

    // If a previous high score file exists, check the current score against it.
    if (prevHigh) {
	if (! high1.open (IO_ReadOnly)) {
	    QString high1_name = high1.name();
	    QMessageBox::information (this, "Check for High Score",
		"Cannot open file \"" + high1_name +
		"\".\nfor input.  Please contact your System Administrator.");
	    return;
	}

	// Read previous users, levels and scores from the high score file.
	s1.setDevice (&high1);
	bool found = FALSE;
	highCount = 0;
	while (! s1.endData()) {
	    char * prevUser;
	    char * prevDate;
	    s1 >> prevUser;
	    s1 >> prevLevel;
	    s1 >> prevScore;
	    s1 >> prevDate;
	    delete prevUser;
	    delete prevDate;
	    highCount++;
	    if (score > prevScore) {
		found = TRUE;			// We have a high score.
		break;
	    }
	}

	// Check if higher than one on file or fewer than 10 previous scores.
	if ((! found) && (highCount >= 10)) {
	    return;				// We did not have a high score.
	}
    }

    /* ************************************************************* */
    /* If we have come this far, we have a new high score to record. */
    /* ************************************************************* */

    QFile high2 (userDataDir + "hi_" + collection->prefix + ".tmp");
    QDataStream s2;

    if (! high2.open (IO_WriteOnly)) {
	QMessageBox::information (this, "Check for High Score",
	"Cannot open file \"" + userDataDir + "hi_" + collection->prefix +
	".tmp" "\".\nfor output.  Please contact your System Administrator.");
	return;
    }

    // Dialog to ask the user to enter their name.
    QDialog *		hsn = new QDialog (view, "hsNameDialog", TRUE,
			WStyle_Customize | WStyle_NormalBorder | WStyle_Title);
    QLabel *		hsnMessage  = new QLabel (
			"Congratulations !!!  You have achieved a high score\n"
			"in this game.  Please enter your name so that it may\n"
			"be enshrined in the KGoldrunner Hall of Fame.",
			hsn);
    QLineEdit *		hsnUser = new QLineEdit (hsn);
    QPushButton *	OK = new QPushButton ("OK", hsn);

    QPoint		p = view->mapToGlobal (QPoint (0,0));

    hsn->		setCaption ("Save High Score");

    hsn->		setGeometry (p.x() + 50, p.y() + 50, 310, 160);
    hsnMessage->	setGeometry (10, 10, 290, 70);
    hsnUser->		setGeometry (70, 90, 180,  20);
    OK->		setGeometry (10, 130, 90,  25);

    OK->		setAccel (Key_Return);
    hsnUser->		setFocus();		// Set the keyboard input on.

    connect	(hsnUser, SIGNAL (returnPressed ()), hsn, SLOT (accept ()));
    connect	(OK,      SIGNAL (clicked ()),       hsn, SLOT (accept ()));

    while (TRUE) {
	hsn->exec();
	thisUser = hsnUser->text();
	if (thisUser.length() > 0)
	    break;
	QMessageBox::information (this, "Save High Score",
				"You must enter something.  Please try again.");
    }

    delete hsn;

    QDate today = QDate::currentDate();
    QString hsDate;
    QString day = today.dayName(today.dayOfWeek());
    hsDate = hsDate.sprintf
		("%s %04d-%02d-%02d",
		day.myStr(),
		today.year(), today.month(), today.day());

    s2.setDevice (&high2);

    if (prevHigh) {
	high1.reset();
	bool scoreRecorded = FALSE;
	highCount = 0;
	while ((! s1.endData()) && (highCount < 10)) {
	    char * prevUser;
	    char * prevDate;
	    s1 >> prevUser;
	    s1 >> prevLevel;
	    s1 >> prevScore;
	    s1 >> prevDate;
	    if ((! scoreRecorded) && (score > prevScore)) {
		highCount++;
		s2 << thisUser.myStr();
		s2 << (Q_INT16) level;
		s2 << (Q_INT32) score;
		s2 << hsDate.myStr();
		scoreRecorded = TRUE;
	    }
	    if (highCount < 10) {
		highCount++;
		s2 << prevUser;
		s2 << prevLevel;
		s2 << prevScore;
		s2 << prevDate;
	    }
	    delete prevUser;
	    delete prevDate;
	}
	if ((! scoreRecorded) && (highCount < 10)) {
	    s2 << thisUser.myStr();
	    s2 << (Q_INT16) level;
	    s2 << (Q_INT32) score;
	    s2 << hsDate.myStr();
	}
	high1.close();
    }
    else {
	s2 << thisUser.myStr();
	s2 << (Q_INT16) level;
	s2 << (Q_INT32) score;
	s2 << hsDate.myStr();
    }

    high2.close();

    QDir dir;
    dir.rename (high2.name(),
		userDataDir + "hi_" + collection->prefix + ".dat", TRUE);
    QMessageBox::information (this, "Save High Score",
				"Your high score has been saved.");

    showHighScores();
    return;
}

void KGoldrunner::showHighScores()
{
    // Don't keep high scores for tutorial games.
    if (collection->prefix.left(4) == "tute") {
	QMessageBox::information (this, "Show High Scores",
		    "Sorry, we do not keep high scores for tutorial games.");
	return;
    }

    Q_INT16	prevLevel = 0;
    Q_INT32	prevScore = 0;

    // Look for user's high-score file or for a released high-score file.
    QFile high1 (userDataDir + "hi_" + collection->prefix + ".dat");
    QDataStream s1;

    if (! high1.exists()) {
	high1.setName (systemDataDir + "hi_" + collection->prefix + ".dat");
	if (! high1.exists()) {
	    QMessageBox::information (this, "Show High Scores",
		    "Sorry, there are no high scores for\nthe \"" +
		    collection->name + "\" game yet.");
	    return;
	}
    }

    if (! high1.open (IO_ReadOnly)) {
	QString high1_name = high1.name();
	QMessageBox::information (this, "Show High Scores",
	    "Cannot open file \"" + high1_name +
	    "\".\nfor input.  Please contact your System Administrator.");
	return;
    }

    QDialog *		hs = new QDialog (view, "hsDialog", TRUE,
			WStyle_Customize | WStyle_NormalBorder | WStyle_Title);
#ifdef QT1
    QLabel *		hsHeader = new QLabel (
					"KGOLDRUNNER HALL OF FAME\n\n\""
					+ collection->name + "\" Game",
			hs);
			hsHeader->setAlignment (AlignCenter);
    QString		s = hsHeader->fontInfo().family();
    int			i = hsHeader->fontInfo().pointSize();
			hsHeader->setFont (QFont (s, i, QFont::Bold));
#else
    QLabel *		hsHeader = new QLabel (
					"<center><h2>"
					"KGoldrunner Hall of Fame"
					"</h2></center><br/>"
					"<center><h3>\"" + collection->name +
					"\" Game</h3></center>",
			hs);
#endif
    QLabel *		hsColHeader  = new QLabel (
					"    Name                          "
					"Level  Score       Date", hs);
    QLabel *		hsLine [10];
    QPushButton *	OK = new QPushButton ("Close", hs);

    QPoint		p = view->mapToGlobal (QPoint (0,0));

    hs->		setCaption ("High Scores");

    hs->		setGeometry (p.x() + 50, p.y() + 50, 470, 360);
    hsHeader->		setGeometry (10, 10,  450, 75);
    hsColHeader->	setGeometry (10, 90,  450, 20);
    OK->		setGeometry (10, 330, 90,  25);

    OK->		setAccel (Key_Return);
    connect		(OK, SIGNAL (clicked ()), hs, SLOT (accept ()));

    QFont		f ("courier", 12);
    f.			setFixedPitch (TRUE);
    f.			setBold (TRUE);
    hsColHeader->	setFont (f);

    // Set up the format for the high-score lines.
    f.			setBold (FALSE);
    QString		line;
    const char *	hsFormat = "%2d. %-30.30s %3d %7ld  %s";

    // Read and display the users, levels and scores from the high score file.
    s1.setDevice (&high1);
    int		n = 0;
    while ((! s1.endData()) && (n < 10)) {
	char * prevUser;
	char * prevDate;
	s1 >> prevUser;
	s1 >> prevLevel;
	s1 >> prevScore;
	s1 >> prevDate;

	line = line.sprintf (hsFormat,
			     n+1, prevUser, prevLevel, prevScore, prevDate);
	hsLine [n] = new QLabel (line, hs);
	hsLine [n]->setGeometry (10, 115 + 20*n, 450, 20);
	hsLine [n]->setFont (f);

	delete prevUser;
	delete prevDate;
	n++;
    }

    // Start up the dialog box.
    hs->		exec();

    delete hs;
}

void KGoldrunner::pauseResume()
{
    commandCallback (ID_PAUSE);		// Pause/Resume button was clicked.
}

void KGoldrunner::freeze()
{
    pauseBtn->setText ("Resume (Esc)");
    KGrObj::frozen = TRUE;	// Halt the game, by blocking all timer events.
}

void KGoldrunner::unfreeze()
{
    pauseBtn->setText ("PAUSE (Esc)");
    KGrObj::frozen = FALSE;	// Restart the game.  Because frozen = FALSE,
    doStep();			// the game goes on running after the step.
}

QString KGoldrunner::getFilePath (Owner o, KGrCollection * colln, int lev)
{
    QString filePath;

    if (lev == 0) {
	// End of game: show the "ENDE" screen.
	o = SYSTEM;
	filePath = "level000.grl";
    }
    else {
	filePath.setNum (lev);		// Convert INT -> QString.
	filePath = filePath.rightJustify (3,'0'); // Add 0-2 zeros at left.
	filePath.append (".grl");	// Add KGoldrunner level-suffix.
	filePath.prepend (colln->prefix);	// Add collection file-prefix.
    }

    filePath.prepend (((o == SYSTEM)? systemDataDir : userDataDir) + "levels/");

    return (filePath);
}

void KGoldrunner::initStatusBar()
{
#ifdef QT1
  QString s = statusBar->fontInfo().family();
  int i = statusBar->fontInfo().pointSize();
  statusBar->setFont (QFont (s, i, QFont::Bold));
  statusBar->insertItem(klocale->translate("Lives: 005"),ID_LIFES);
  statusBar->insertItem(klocale->translate("Score: 00000"),ID_SCORE);
  statusBar->insertItem(klocale->translate("Level: 000"),ID_LEVEL);
  statusBar->insertItem(klocale->translate("System Level"),ID_LEVELPLACE);
  statusBar->insertItem(" KGoldrunner ",ID_DUMMY);
  statusBar->setAlignment(ID_DUMMY,KStatusBar::Right);
#else
  QString s = statusBar()->fontInfo().family();
  int i = statusBar()->fontInfo().pointSize();
  statusBar()->setFont (QFont (s, i, QFont::Bold));
  statusBar()->setSizeGripEnabled (FALSE);
  statusString.fill (' ', ID_DUMMY);
  statusString.replace (ID_LIFES, L_LIFES, "Lives: 005");
  statusString.replace (ID_SCORE, L_SCORE, "Score: 00000");
  statusString.replace (ID_LEVEL, L_LEVEL, "Level: 000");
  statusString.replace (ID_GROUP, L_GROUP, "System Level");
  statusBar()->message (statusString);
#endif
}

void KGoldrunner::changeLevel (int newLevelNo)
{
  QString tmp;
  tmp.setNum (newLevelNo);
  if (newLevelNo < 100)
      tmp = tmp.rightJustify (3, '0');
#ifdef QT1
  tmp.insert (0, klocale->translate("Level: "));
  statusBar->changeItem (tmp, ID_LEVEL);
#else
  tmp.insert (0, "Level: ");
  tmp = tmp.leftJustify (L_LEVEL, ' ');
  statusString.replace (ID_LEVEL, L_LEVEL, tmp);
  statusBar()->message (statusString);
#endif
}

void KGoldrunner::changeLifes (int newLives)
{
  QString tmp;
  tmp.setNum (newLives);
  if (newLives < 100)
      tmp = tmp.rightJustify (3, '0');
#ifdef QT1
  tmp.insert (0, klocale->translate("Lives: "));
  statusBar->changeItem (tmp, ID_LIFES);
#else
  tmp.insert (0, "Lives: ");
  tmp = tmp.leftJustify (L_LIFES, ' ');
  statusString.replace (ID_LIFES, L_LIFES, tmp);
  statusBar()->message (statusString);
#endif
}

void KGoldrunner::changeScore (int newScore)
{
  QString tmp;
  tmp.setNum (newScore);
  if (newScore < 10000) {
      tmp = tmp.rightJustify (5, '0');
  }
#ifdef QT1
  tmp.insert (0, klocale->translate("Score: "));
  statusBar->changeItem (tmp, ID_SCORE);
#else
  tmp.insert (0, "Score: ");
  tmp = tmp.leftJustify (L_SCORE, ' ');
  statusString.replace (ID_SCORE, L_SCORE, tmp);
  statusBar()->message (statusString);
#endif
}

/******************************************************************************/
/****************************  GAME CONTROL SLOTS  ****************************/
/******************************************************************************/

void KGoldrunner::commandCallback(int i)
{
    bool b;

    switch (i) {
    // Game-play menu actions.
    case ID_NEW:	if (saveOK ()) {startLevel (ID_NEW);} break;
    case ID_OPEN:	if (saveOK ()) {startLevel (ID_OPEN);} break;
    case ID_NEXT:	if (saveOK ()) {level++; startLevel (ID_OPEN);} break;
    case ID_KILL:	if (!editMode) {herosDead();} break;
    case ID_SAVEGAME:	saveGame(); break;
    case ID_LOADGAME:	if (saveOK ()) {loadGame();} break;
    case ID_HIGH:	showHighScores(); break;
    case ID_EXIT:	if (saveOK ()) {exitWarning=TRUE; close (TRUE);} break;
			// Causes "lastWindowClosed()" signal.

    // Pause/Resume (Esc).
    case ID_PAUSE:	if (!(KGrObj::frozen)) freeze(); else unfreeze(); break;

    // Game-edit menu and file toolbar actions.
    case ID_CREATE:	createLevel (); break;		// Slot does saveOK().
    case ID_UPDATE:	updateLevel (); break;          // Slot does saveOK().
    case ID_EDITNEXT:	if (saveOK ()) {level++; updateLevel ();} break;
    case ID_SAVEFILE:	b = saveLevelFile (); break;
    case ID_MOVEFILE:	if (saveOK ()) {moveLevelFile ();} break;
    case ID_DELEFILE:	if (saveOK ()) {deleteLevelFile ();} break;
    case ID_ECOLL:	if (saveOK ()) {editCollection (ID_ECOLL);} break;
    case ID_NCOLL:	if (saveOK ()) {editCollection (ID_NCOLL);} break;

    // Settings (options) menu.
    case ID_KGR:	rCheck (ID_KGR); break;		// Set KGr defaults.
    case ID_TRAD:	rCheck (ID_TRAD); break;	// Set Trad defaults.
    case ID_MESLOW:	KGrFigure::variableTiming =
				(KGrFigure::variableTiming ? FALSE : TRUE);
			rCheck (ID_MESLOW); break;
    case ID_ACGOLD:	KGrFigure::alwaysCollectNugget =
				(KGrFigure::alwaysCollectNugget ? FALSE : TRUE);
			rCheck (ID_ACGOLD); break;
    case ID_RTHOLE:	KGrFigure::runThruHole =
				(KGrFigure::runThruHole ? FALSE : TRUE);
			rCheck (ID_RTHOLE); break;
    case ID_RATOP:	KGrFigure::reappearAtTop =
				(KGrFigure::reappearAtTop ? FALSE : TRUE);
			rCheck (ID_RATOP); break;
    case ID_SRCH1:	KGrFigure::searchStrategy = LOW;
			rCheck (ID_SRCH1); break;
    case ID_SRCH2:	KGrFigure::searchStrategy = MEDIUM;
			rCheck (ID_SRCH2); break;
    case ID_NSPEED:	hero->setSpeed (NSPEED);      sCheck (ID_NSPEED); break;
    case ID_BSPEED:	hero->setSpeed (BEGINSPEED);  sCheck (ID_BSPEED); break;
    case ID_VSPEED:	hero->setSpeed (NOVICESPEED); sCheck (ID_VSPEED); break;
    case ID_CSPEED:	hero->setSpeed (CHAMPSPEED);  sCheck (ID_CSPEED); break;
    case ID_ISPEED:	hero->setSpeed (+1);          sCheck (ID_ISPEED); break;
    case ID_DSPEED:	hero->setSpeed (-1);          sCheck (ID_DSPEED); break;

    // Help-menu actions.  Tute and hint are on the game menu in both QT1 & QT2.
    case ID_TUTE:	if (saveOK ()) {startTutorial();} break;
    case ID_HINT:	if (!editMode) {showHint();} break;
#ifndef QT1
    // These are for QT2 only.  KDE 1 provides the Help-menu in the QT1 version.
    case ID_MANUAL:	showManual (); break;
    case ID_ABOUTKGR:	QMessageBox::about (this, "About KGoldrunner", about);
			break;
    case ID_ABOUTQT:	QMessageBox::aboutQt (this); break;
#endif

#ifdef QT1
    // Edit toolbar/menu actions.
    case ID_FREE:	editObj = FREE; break;		// Empty space.
    case ID_HERO:	editObj = HERO; break;		// Hero.
    case ID_ENEMY:	editObj = ENEMY; break;		// Enemy.
    case ID_BRICK:	editObj = BRICK; break;		// Brick (can dig).
    case ID_FBRICK:	editObj = FBRICK; break;	// Trap (fall through).
    case ID_BETON:	editObj = BETON; break;		// Concrete.
    case ID_LADDER:	editObj = LADDER; break;	// Ladder.
    case ID_HLADDER:	editObj = HLADDER; break;	// Hidden ladder.
    case ID_POLE:	editObj = POLE; break;		// Pole (or bar).
    case ID_NUGGET:	editObj = NUGGET; break;	// Gold nugget.
#else
    // Edit toolbar/menu actions.
    case ID_FREE:	freeSlot (); break;		// Empty space.
    case ID_HERO:	edheroSlot (); break;		// Hero.
    case ID_ENEMY:	edenemySlot (); break;		// Enemy.
    case ID_BRICK:	brickSlot (); break;		// Brick (can dig).
    case ID_FBRICK:	fbrickSlot (); break;		// Trap (fall through).
    case ID_BETON:	betonSlot (); break;		// Concrete.
    case ID_LADDER:	ladderSlot (); break;		// Ladder.
    case ID_HLADDER:	hladderSlot (); break;		// Hidden ladder.
    case ID_POLE:	poleSlot (); break;		// Pole (or bar).
    case ID_NUGGET:	nuggetSlot (); break;		// Gold nugget.
#endif

    default : break;
    }

#ifdef QT1
    // Change the toggles on the edit-object buttons.
    if ((i >= ID_FREE) && (i <= ID_NUGGET)) {
	editToolbar->setButton (pressedButton, FALSE);
	pressedButton = i;
	editToolbar->setButton (pressedButton, TRUE);
    }
#endif
}

void KGoldrunner::rCheck (int ruleID)
{
    // Un-tick the default options on the Settings menu.
    opt_menu->setItemChecked (ID_KGR, FALSE);
    opt_menu->setItemChecked (ID_TRAD, FALSE);

    if (ruleID == ID_KGR) {
	// KGoldrunner defaults selected: tick the option and set the defaults.
	opt_menu->setItemChecked (ID_KGR, TRUE);
	KGrFigure::variableTiming = FALSE;
	KGrFigure::alwaysCollectNugget = FALSE;
	KGrFigure::runThruHole = FALSE;
	KGrFigure::reappearAtTop = FALSE;
	KGrFigure::searchStrategy = MEDIUM;
    }
    if (ruleID == ID_TRAD) {
	// Traditional defaults selected: tick the option and set the defaults.
	opt_menu->setItemChecked (ID_TRAD, TRUE);
	KGrFigure::variableTiming = TRUE;
	KGrFigure::alwaysCollectNugget = TRUE;
	KGrFigure::runThruHole = TRUE;
	KGrFigure::reappearAtTop = TRUE;
	KGrFigure::searchStrategy = LOW;
    }

    // Tick the rules that have been set.  Leave the others un-ticked.
    opt_menu->setItemChecked
	    (ID_MESLOW, (KGrFigure::variableTiming ? TRUE : FALSE));
    opt_menu->setItemChecked
	    (ID_ACGOLD, (KGrFigure::alwaysCollectNugget ? TRUE : FALSE));
    opt_menu->setItemChecked
	    (ID_RTHOLE, (KGrFigure::runThruHole ? TRUE : FALSE));
    opt_menu->setItemChecked
	    (ID_RATOP, (KGrFigure::reappearAtTop ? TRUE : FALSE));
    opt_menu->setItemChecked
	    (ID_SRCH1, ((KGrFigure::searchStrategy == LOW) ? TRUE : FALSE));
    opt_menu->setItemChecked
	    (ID_SRCH2, ((KGrFigure::searchStrategy == MEDIUM) ? TRUE : FALSE));
}

void KGoldrunner::sCheck (int speedID)
{
    // Un-tick all the speed options on the Settings menu.
    opt_menu->setItemChecked (ID_NSPEED, FALSE);
    opt_menu->setItemChecked (ID_BSPEED, FALSE);
    opt_menu->setItemChecked (ID_VSPEED, FALSE);
    opt_menu->setItemChecked (ID_CSPEED, FALSE);
    opt_menu->setItemChecked (ID_ISPEED, FALSE);
    opt_menu->setItemChecked (ID_DSPEED, FALSE);

    // Tick the speed option that was selected.
    opt_menu->setItemChecked (speedID, TRUE);
}

void KGoldrunner::incScore (int n)
{
  score = score + n;		// SCORING: trap enemy 75, kill enemy 75,
  changeScore (score);		// collect gold 250, complete the level 1500.
}

void KGoldrunner::showHidden()
{
  int i,j;
  for (i=1;i<21;i++)
    for (j=1;j<29;j++)
      if (playfield[j][i]->whatIam()==HLADDER)
	((KGrHladder *)playfield[j][i])->showLadder();
  initSearchMatrix();
}

void KGoldrunner::loseNugget()
{
    hero->loseNugget();		// Enemy trapped/dead and holding a nugget.
}

void KGoldrunner::herosDead()
{
    // Lose a life.
    if (--lifes > 0) {
	// Still some life remaining: repeat the current level.
	changeLifes (lifes);
	enemyCount = 0;
	loadLevel (level);
    }
    else {
	// Game over: display the "ENDE" screen.
	changeLifes (lifes);
	freeze();
#ifdef QT1
	QString gameOver = "GAME OVER !!!";
#else
	QString gameOver = "<B>" "GAME OVER !!!" "</B>";
#endif
	QMessageBox::information (this, collection->name, gameOver);
	checkHighScore();	// Check if there is a high score for this game.

	enemyCount = 0;
	enemies.clear();	// Stop the enemies catching the hero again ...
	unfreeze();		//    ... NOW we can unfreeze.
	newLevel = TRUE;
	loadLevel (0);
	newLevel = FALSE;
    }
}

void KGoldrunner::nextLevel()
{
    lifes++;			// Level completed: gain another life.
    changeLifes (lifes);
    incScore (1500);

    if (level >= collection->nLevels) {
	freeze();
	QMessageBox::information (this, collection->name,
	    "CONGRATULATIONS !!!!\n"
	    "You have conquered the last level in the \"" + collection->name +
	    "\" game !!");
	checkHighScore();	// Check if there is a high score for this game.

	unfreeze();
	level = 0;		// Game completed: display the "ENDE" screen.
    }
    else {
	changeLevel (++level);	// Go to the next level.
    }

    enemyCount = 0;
    enemies.clear();
    newLevel = TRUE;
    loadLevel (level);
    newLevel = FALSE;
}

void KGoldrunner::readMousePos()
{
    QPoint p;
    int i, j;

    // If loading a level for play or editing, ignore mouse-position input.
    if (loading) return;

    p = view->getMousePos ();
    i = ((p.x() - 32) / 16) + 1; j = ((p.y() - 32) / 16) + 1;

    if (editMode) {
	// Editing - check if we are in paint mode and have moved the mouse.
	if (paintEditObj && ((i != oldI) || (j != oldJ))) {
	    insertEditObj (i, j);
	    oldI = i;
	    oldJ = j;
	}
    }
    else {
	// Playing - if  the level has started, control the hero.
	if (KGrObj::frozen) return;	// If game is stopped, do nothing.

	hero->setDirection (i, j);

	// Start playing when the mouse moves off the hero.
	if ((! hero->started) && ((i != startI) || (j != startJ))) {
	    startPlaying();
	}
    }
}

void KGoldrunner::doDig (int button) {
    // If loading a level for play or editing, ignore mouse-button input.
    if (! loading) {
	if (! hero->started) {
	    startPlaying();		// If first player-input, start playing.
	}
	switch (button) {
	case LeftButton:	hero->digLeft  (); break;
	case RightButton:	hero->digRight (); break;
	default:		break;
	}
    }
}

/******************************************************************************/
/**************************  KGOLDRUNNER PAINT EVENT **************************/
/******************************************************************************/

void KGoldrunner::paintEvent (QPaintEvent * ev)
{
    if (editMode)		// KGrEditable class has its own repaint.  Don't
	return;			// repaint the hero's last PLAYING position.

    if (ev->rect().width() > 0)	// Use "ev": just to avoid compiler warnings.
	;

    if (hero != NULL) {
	hero->showFigure();
    }

    for (enemy=enemies.first();enemy != 0; enemy = enemies.next()) {
	enemy->showFigure();
    }
}

/******************************************************************************/
/**************************  AUTHORS' DEBUGGING AIDS **************************/
/******************************************************************************/

void KGoldrunner::doStep()
{
    bool temp;
    int i,j;

    if (editMode)		// Can't move figures when in Edit Mode.
	return;

    temp = KGrObj::frozen;
    KGrObj::frozen = FALSE;	// Temporarily restart the game, by re-running
				// any timer events that have been blocked.

    readMousePos();		// Set hero's direction.
    hero->doStep();		// Move the hero one step.

    j = enemies.count();	// Move each enemy one step.
    for (i = 0; i < j; i++) {
	enemy = enemies.at(i);	// Need to use an index because called methods
	enemy->doStep();	// change the "current()" of the "enemies" list.
    }

    for (i=1; i<=28; i++)
	for (j=1; j<=20; j++) {
	    if ((playfield[i][j]->whatIam() == HOLE) ||
		(playfield[i][j]->whatIam() == USEDHOLE) ||
		(playfield[i][j]->whatIam() == BRICK))
		((KGrBrick *)playfield[i][j])->doStep();
    }

    KGrObj::frozen = temp;	// If frozen was TRUE, halt again, which gives a
				// single-step effect, otherwise go on running.
}

void KGoldrunner::showFigurePositions()
{
    hero->showState('p');
    for (enemy=enemies.first();enemy != 0; enemy = enemies.next()) {
	enemy->showState('p');
    }
}

void KGoldrunner::showHeroState()
{
    hero->showState('s');
}

void KGoldrunner::showEnemyState(int enemyId)
{
    for (enemy=enemies.first();enemy != 0; enemy = enemies.next()) {
	if (enemy->enemyId == enemyId) enemy->showState('s');
    }
}

void KGoldrunner::showObjectState()
{
    QPoint p;
    int i, j;
    KGrObj * myObject;

    p = view->getMousePos ();
    i = ((p.x() - 32) / 16) + 1; j = ((p.y() - 32) / 16) + 1;
    myObject = playfield[i][j];
    switch (myObject->whatIam()) {
	case BRICK:
	case HOLE:
	case USEDHOLE:
		 ((KGrBrick *)myObject)->showState(i, j); break;
	default: myObject->showState(i, j); break;
    }
}

/******************************************************************************/
/**************  GAME EDITOR SLOTS ACTIVATED BY MENU OR TOOLBAR  **************/
/******************************************************************************/

void KGoldrunner::createLevel()
{
    int	i, j;

    if (! saveOK()) {					// Check unsaved work.
	return;
    }

    if (! ownerOK (USER)) {
	QMessageBox::information (this, "Create Level",
	    "You cannot create and save a level\n"
	    "until you have created a game to hold\n"
	    "it.  Try menu item \"Create a Game\".");
	return;
    }

    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = TRUE;

    level = 0;
    initEdit();
    levelName = "";
    levelHint = "";

    // Clear the playfield.
    editObj = FREE;
    for (i = 1; i <= FIELDWIDTH; i++)
    for (j = 1; j <= FIELDHEIGHT; j++) {
	insertEditObj (i, j);
	playfield[i][j]->show();
	editObjArray[i][j] = editObj;
    }

    editObj = HERO;
    insertEditObj (1, 1);
    editObjArray[1][1] = editObj;
    editObj = BRICK;

    showEditLevel();
    edit_menu->setItemEnabled (ID_SAVEFILE, TRUE);
    file_menu->setItemEnabled (ID_SAVEFILE, TRUE);

    for (j = 1; j <= FIELDHEIGHT; j++)
    for (i = 1; i <= FIELDWIDTH; i++) {
	lastSaveArray[i][j] = editObjArray[i][j];	// Copy for "saveOK()".
    }

    // Re-enable player input.
    loading = FALSE;
}

void KGoldrunner::updateLevel()
{
    if (! saveOK()) {					// Check unsaved work.
	return;
    }

    if (! ownerOK (USER)) {
	QMessageBox::information (this, "Edit Level",
	    "You cannot edit and save a level until\n"
	    "you have created a game to hold it.\n"
	    "Try menu item \"Create a Game\".");
	return;
    }

    if (level < 0) level = 0;
    int lev = selectLevel (ID_UPDATE);
    if (lev == 0)
	return;

    if (owner == SYSTEM) {
	QMessageBox::information (this, "Edit Level",
	    "It is OK to edit a System level, but you MUST save\n"
	    "the level in one of your own games.\n\n"
	    "You're not just taking a peek at the hidden ladders\n"
	    "and fall-through bricks, are you?     :-)");
    }

    loadEditLevel (lev);
}

void KGoldrunner::loadEditLevel (int lev)
{
    int i, j;

    QFile levelFile;
    if (! openLevelFile (lev, levelFile))
	return;

    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = TRUE;

    level = lev;
    initEdit();

    // Load the level.
    for (j = 1; j <= FIELDHEIGHT; j++)
    for (i = 1; i <= FIELDWIDTH;  i++) {
	editObj = levelFile.getch ();
	insertEditObj (i, j);
	playfield[i][j]->show();
	editObjArray[i][j] = editObj;
	lastSaveArray[i][j] = editObjArray[i][j];	// Copy for "saveOK()".
    }

    // Read a newline character, then read in the level name and hint (if any).
    int c = levelFile.getch();
    levelName = "";
    levelHint = "";
    i = 1;
    while ((c = levelFile.getch()) != EOF) {
	switch (i) {
	case 1:	if (c == '\n')			// Level name is on one line.
		    i = 2;
		else
		    levelName += (char) c;
		break;

	case 2:	levelHint += (char) c;		// Hint is on rest of file.
		break;
	}
    }

    editObj = BRICK;				// Reset default object.
    levelFile.close ();

    showEditLevel();				// Reconnect signals.
    view->repaint();				// Show the level name.
    edit_menu->setItemEnabled (ID_SAVEFILE, TRUE);
    file_menu->setItemEnabled (ID_SAVEFILE, TRUE);

    // Re-enable player input.
    loading = FALSE;
}

bool KGoldrunner::saveLevelFile()
{
    int i, j;
    int action;
    QString filePath;
    int lev = level;
    bool addFile;

    if (! editMode) {
	QMessageBox::information (this, "Save Level",
	"Inappropriate action: you are not editing a level.");
	return (FALSE);
    }

    // Save the current collection index.
    int N = collnIndex + collnOffset;

    if (lev == 0) {
	// New level: choose a number.
	action = ID_CREATE;
    }
    else {
	// Existing level: confirm the number or choose a new number.
	action = ID_SAVEFILE;
    }

    // Pop up dialog box, which could change the collection or level or both.
    lev = selectLevel (action);
    if (lev == 0)
	return (FALSE);

    // Get the new collection (if changed).
    int n = collnIndex + collnOffset;

    // Set the name of the output file.
    filePath = getFilePath (owner, collection, lev);
    QFile levelFile (filePath);

    if ((action == ID_SAVEFILE) && (n == N) && (lev == level)) {
	// This is a normal edit: the old file is to be re-written.
	addFile = FALSE;
    }
    else {
	addFile = TRUE;
	// Check if the file is to be inserted in or appended to the collection.
	if (levelFile.exists()) {
	    switch (QMessageBox::warning (this, "Save Level",
	    "Do you want to insert a level and move existing levels up by one?",
	    "&Insert Level", "&Cancel", "", 0, 1)) {

	    case 0:	if (! reNumberLevels
				    (n, lev, collections.at(n)->nLevels, +1)) {
			    return (FALSE);
			}
			break;
	    case 1:	return (FALSE);
			break;
	    }
	}
    }

    // Open the output file.
    if (! levelFile.open (IO_WriteOnly)) {
	QMessageBox::information (this, "Save Level",
		"Cannot open file \"" + filePath +
		"\".\nfor output.  Please contact your System Administrator.");
	return (FALSE);
    }

    // Save the level.
    for (j = 1; j < 21; j++)
    for (i = 1; i < 29; i++) {
	levelFile.putch (editObjArray[i][j]);
	lastSaveArray[i][j] = editObjArray[i][j];	// Copy for "saveOK()".
    }
    levelFile.putch ('\n');

    int len1 = levelName.length();		// Save the level name (if any).
    if (len1 > 0) {
	for (i = 0; i < len1; i++)
	    levelFile.putch (levelName.myChar(i));
	levelFile.putch ('\n');			// Add a newline.
    }

    int len2 = levelHint.length();		// Save the level hint (if any).
    char ch = '\0';
    if (len2 > 0) {
	if (len1 <= 0)
	    levelFile.putch ('\n');		// Leave blank line for name.
	for (i = 0; i < len2; i++) {
	    ch = levelHint.myChar(i);
	    levelFile.putch (ch);		// Copy the character.
	}
	if (ch != '\n')
	    levelFile.putch ('\n');		// Add a newline character.
    }

    levelFile.close ();

    if (addFile) {
	collections.at(n)->nLevels++;
	saveCollections (owner);
    }

    level = lev;
    changeLevel (level);
    view->repaint();				// Display new title.
    return (TRUE);
}

/******************************************************************************/
/***************  GAME EDITOR FUNCTIONS ACTIVATED BY MENU ONLY  ***************/
/******************************************************************************/

void KGoldrunner::moveLevelFile ()
{
    QString filePath1;
    QString filePath2;
    int action = ID_MOVEFILE;

    if (level <= 0) {
	QMessageBox::information (this, "Move Level",
		"You must first load a level to be moved.\n"
		"Use the \"Game\" or \"Edit\" menu.");
	return;
    }

    int fromC = collnIndex + collnOffset;
    int fromL = level;
    int toC   = fromC;
    int toL   = fromL;

    if (collections.at(fromC)->owner != USER) {
	QMessageBox::information (this, "Move Level",
		"Sorry, you cannot move a System level.");
	return;
    }

    // Pop up dialog box to get the collection and level number to move to.
    while ((toC == fromC) && (toL == fromL)) {
	toL = selectLevel (action);
	if (toL == 0)
	    return;

	toC = collnIndex + collnOffset;

	if ((toC == fromC) && (toL == fromL)) {
	    QMessageBox::information (this, "Move Level",
		    "You must change the level or the game or both.");
	}
    }

    QDir dir;

    // Save the "fromN" file under a temporary name.
    filePath1 = getFilePath (USER, collections.at(fromC), fromL);
    filePath2 = filePath1;
    filePath2 = filePath2.append (".tmp");
    dir.rename (filePath1, filePath2, TRUE);

    if (toC == fromC) {					// Same collection.
	if (toL < fromL) {				// Decrease level.
	    // Move "toL" to "fromL - 1" up by 1.
	    if (! reNumberLevels (toC, toL, fromL-1, +1)) {
		return;
	    }
	}
	else {						// Increase level.
	    // Move "fromL + 1" to "toL" down by 1.
	    if (! reNumberLevels (toC, fromL+1, toL, -1)) {
		return;
	    }
	}
    }
    else {						// Different collection.
	// In "fromC", move "fromL + 1" to "nLevels" down and update "nLevels".
	if (! reNumberLevels (fromC, fromL + 1,
				    collections.at(fromC)->nLevels, -1)) {
	    return;
	}
	collections.at(fromC)->nLevels--;

	// In "toC", move "toL + 1" to "nLevels" up and update "nLevels".
	if (! reNumberLevels (toC, toL, collections.at(toC)->nLevels, +1)) {
	    return;
	}
	collections.at(toC)->nLevels++;

	saveCollections (USER);
    }

    // Rename the saved "fromL" file to become "toL".
    filePath1 = getFilePath (USER, collections.at(toC), toL);
    dir.rename (filePath2, filePath1, TRUE);

    level = toL;
    collection = collections.at(toC);
    view->repaint();			// Re-display details of level.
    changeLevel (level);
}

void KGoldrunner::deleteLevelFile ()
{
    QString filePath;
    int action = ID_DELEFILE;
    int lev = level;

    if (! ownerOK (USER)) {
	QMessageBox::information (this, "Delete Level",
	    "You cannot delete a level until you\n"
	    "have created a game and a level.\n"
	    "Try menu item \"Create a Game\".");
	return;
    }

    // Pop up dialog box to get the collection and level number.
    lev = selectLevel (action);
    if (lev == 0)
	return;

    // Set the name of the file to be deleted.
    int n = collnIndex + collnOffset;
    filePath = getFilePath (USER, collections.at(n), lev);
    QFile levelFile (filePath);

    // Delete the file for the selected collection and level.
    if (levelFile.exists()) {
	if (lev < collections.at(n)->nLevels) {
	    switch (QMessageBox::warning (this, "Delete Level",
	    "Do you want to delete a level and move higher levels down by one?",
	    "&Delete Level", "&Cancel", "", 0, 1)) {
	    case 0:	break;
	    case 1:	return; break;
	    }
	    levelFile.remove ();
	    if (! reNumberLevels (n, lev + 1, collections.at(n)->nLevels, -1)) {
		return;
	    }
	}
	else {
	    levelFile.remove ();
	}
    }
    else {
	QMessageBox::information (this, "Delete Level",
		"Cannot find file \"" + filePath + "\".\nto be deleted.  " +
		"Please contact your System Administrator.");
	return;
    }

    collections.at(n)->nLevels--;
    saveCollections (USER);
    if (lev <= collections.at(n)->nLevels) {
	level = lev;
    }
    else {
	level = collections.at(n)->nLevels;
    }

    // Repaint the screen with the level that now has the selected number.
    if (editMode && (level > 0)) {
	loadEditLevel (level);			// Load level in edit mode.
    }
    else if (level > 0) {
	enemyCount = 0;				// Load level in play mode.
	enemies.clear();
	newLevel = TRUE;;
	loadLevel (level);
	newLevel = FALSE;
    }
    else {
	createLevel();				// No levels left in collection.
    }
    changeLevel (level);
}

/******************************************************************************/
/*********************  SUPPORTING GAME EDITOR FUNCTIONS  *********************/
/******************************************************************************/

bool KGoldrunner::saveOK()
{
    int		i, j;
    bool	result;
    QString	option2 = "&Go on editing";

    result = TRUE;

    if (exitWarning) {					// If window has closed,
	option2 = "";					// can't go on editing.
    }
    if (editMode) {
	for (j = 1; j <= FIELDHEIGHT; j++)
	for (i = 1; i <= FIELDWIDTH; i++) {		// Check cell changes.
	    if (editObjArray[i][j] != lastSaveArray[i][j]) {
		switch (QMessageBox::warning (this, "KGoldrunner Edit",
			"You have not saved your work.  "
			"Do you want to save it now?\n",
			"&Save", "&DON'T Save", option2,
			0, 2)) {
		case 0: result = saveLevelFile(); break;// Save and continue.
		case 1: break;				// Continue: don't save.
		case 2: result = FALSE; break;		// Go back to editing.
		}
		return (result);
	    }
	}
    }
    return (result);
}

void KGoldrunner::initEdit()
{
    if (! editMode) {

	editMode = TRUE;

	// We were previously in play mode: stop the hero running or falling.
	hero->init (1, 1);

	// Show the editor's toolbar.
#ifdef QT1
	editToolbar->setBarPos (KToolBar::Top);
	editToolbar->show();
	setMenu(menuBar);
#else
	addToolBar (editToolbar, "Editor", QMainWindow::Top);

	// Force QMainWindow to re-calculate its sizes.
	qApp->processEvents();
#endif
    }

    paintEditObj = FALSE;

    // Set the default object and button.
    editObj = BRICK;

#ifdef QT1
    editToolbar->setButton (pressedButton, FALSE);
    pressedButton = ID_BRICK;
    editToolbar->setButton (pressedButton, TRUE);
#else
    pressedButton->setOn (FALSE);
    pressedButton = brickBtn;
    pressedButton->setOn (TRUE);
#endif

    oldI = 0;
    oldJ = 0;
    heroCount = 0;
    enemyCount = 0;
    enemies.clear();
    nuggets = 0;

    changeLevel(level);
    changeLifes(0);
    changeScore(0);

    deleteLevel();
    setBlankLevel(FALSE);	// Fill play field with Editable objects.

    view->repaint();		// Show title of level.
}

void KGoldrunner::deleteLevel()
{
    int i,j;
    for (i = 1; i <= FIELDHEIGHT; i++)
    for (j = 1; j <= FIELDWIDTH; j++)
	delete playfield[j][i];
}

void KGoldrunner::insertEditObj (int i, int j)
{
    if ((i < 1) || (j < 1) || (i > FIELDWIDTH) || (j > FIELDHEIGHT))
	return;		// Do nothing: mouse pointer is out of playfield.

    if (editObjArray[i][j] == HERO) {
	// The hero is in this cell: remove him.
	editObjArray[i][j] = FREE;
	heroCount = 0;
    }

    switch (editObj) {
    case FREE:
	((KGrEditable *)playfield[i][j])->setType (FREE, freebg); break;
    case LADDER:
	((KGrEditable *)playfield[i][j])->setType (LADDER, ladderbg); break;
    case HLADDER:
	((KGrEditable *)playfield[i][j])->setType (HLADDER, hladderbg); break;
    case BRICK:
	((KGrEditable *)playfield[i][j])->setType (BRICK, brickbg); break;
    case BETON:
	((KGrEditable *)playfield[i][j])->setType (BETON, betonbg); break;
    case FBRICK:
	((KGrEditable *)playfield[i][j])->setType (FBRICK, fbrickbg); break;
    case POLE:
	((KGrEditable *)playfield[i][j])->setType (POLE, polebg); break;
    case NUGGET:
	((KGrEditable *)playfield[i][j])->setType (NUGGET, nuggetbg); break;
    case ENEMY:
	((KGrEditable *)playfield[i][j])->setType (ENEMY, edenemybg); break;
    case HERO:
	if (heroCount != 0) {
	    // Can only have one hero: remove him from his previous position.
	    for (int m = 1; m <= FIELDWIDTH; m++)
	    for (int n = 1; n <= FIELDHEIGHT; n++) {
		if (editObjArray[m][n] == HERO) {
		    ((KGrEditable *)playfield[m][n])-> setType (FREE, freebg);
		    editObjArray[m][n] = FREE;
		}
	    }
	}
	heroCount = 1;
	((KGrEditable *)playfield[i][j])->setType (HERO, edherobg); break;
    default:
	((KGrEditable *)playfield[i][j])->setType (BRICK, brickbg); break;
    }

    editObjArray[i][j] = editObj;
    playfield[i][j]->show();
}

void KGoldrunner::showEditLevel()
{
    int i, j;

    // Disconnect play-mode slots from signals from border of "view".
    disconnect (view, SIGNAL(mouseClick(int)), 0, 0);
    disconnect (view, SIGNAL(mouseLetGo(int)), 0, 0);

    for (j = 1; j <= FIELDHEIGHT; j++)
    for (i = 1; i <= FIELDWIDTH; i++) {
	// Connect edit-mode slots to signals from playfield element.
	// Note: Play-mode slot was disconnected when element was deleted.
	connect (playfield[i][j], SIGNAL(mouseClick(int)), SLOT(doEdit(int)));
	connect (playfield[i][j], SIGNAL(mouseLetGo(int)), SLOT(endEdit(int)));
    }

    // Connect edit-mode slots to signals from border of "view".
    connect (view, SIGNAL(mouseClick(int)), SLOT(doEdit(int)));
    connect (view, SIGNAL(mouseLetGo(int)), SLOT(endEdit(int)));
}

bool KGoldrunner::reNumberLevels (int cIndex, int first, int last, int inc)
{
    int i, n, step;
    QDir dir;
    QString file1, file2;

    if (inc > 0) {
	i = last;
	n = first - 1;
	step = -1;
    }
    else {
	i = first;
	n = last + 1;
	step = +1;
    }

    while (i != n) {
	file1 = getFilePath (USER, collections.at(cIndex), i);
	file2 = getFilePath (USER, collections.at(cIndex), i - step);
	if (! dir.rename (file1, file2, TRUE)) {	// Allow absolute paths.
	    QMessageBox::information (this, "Save Level",
		"Cannot rename file \"" + file1 + "\".\nto \"" + file2 +
		"\".\nPlease contact your System Administrator.");
	    return (FALSE);
	}
	i = i + step;
    }

    return (TRUE);
}

void KGoldrunner::makeEditToolbar()
{
    // Set up the pixmaps for the editable objects.
    QPixmap pixmap;
    QImage image;

    pixmap = QPixmap (bricks_xpm);
    image = pixmap.convertToImage ();
    for (int i = 0; i < 10; i++) {
	digpix[i].convertFromImage (image.copy (i * 16, 0, 16, 16));
    }

    brickbg	= digpix[0];
    fbrickbg	= digpix[7];

    freebg	= QPixmap (hgbrick_xpm);
    nuggetbg	= QPixmap (nugget_xpm);
    polebg	= QPixmap (pole_xpm);
    betonbg	= QPixmap (beton_xpm);
    ladderbg	= QPixmap (ladder_xpm);
    hladderbg	= QPixmap (hladder_xpm);
    edherobg	= QPixmap (edithero_xpm);
    edenemybg	= QPixmap (editenemy_xpm);

#ifdef QT1
    editToolbar = new KToolBar(this);
#else
    editToolbar = new QToolBar ("Editor", this, QMainWindow::Top, FALSE);
    editToolbar->setHorizontalStretchable (TRUE);
#endif

    editToolbar->setPalette (QPalette (QColor (90, 90, 155)));
    setPalettePropagation (AllChildren);

#ifdef QT1
    // editToolbar->insertButton (loader->loadIcon("filenew.xpm"),
    editToolbar->insertButton (QPixmap (filenew),
			       ID_CREATE, TRUE,
			       klocale->translate("Create Level"));
    // editToolbar->insertButton (loader->loadIcon("fileopen.xpm"),
    editToolbar->insertButton (QPixmap (fileopen),
			       ID_UPDATE, TRUE,
			       klocale->translate("Edit Any Level"));
    // editToolbar->insertButton (loader->loadIcon("filefloppy.xpm"),
    editToolbar->insertButton (QPixmap (filesave),
			       ID_SAVEFILE, TRUE,
			       klocale->translate("Save Level"));
    editToolbar->insertSeparator();
    editToolbar->insertSeparator();

    editToolbar->insertButton (freebg, ID_FREE, TRUE,
			       klocale->translate("Empty space"));
    editToolbar->insertButton (edherobg, ID_HERO, TRUE,
			       klocale->translate("Hero"));
    editToolbar->insertButton (edenemybg, ID_ENEMY, TRUE,
			       klocale->translate("Enemy"));
    editToolbar->insertButton (brickbg, ID_BRICK, TRUE,
			       klocale->translate("Brick (can dig)"));
    editToolbar->insertButton (betonbg, ID_BETON, TRUE,
			      klocale->translate("Concrete (cannot dig)"));
    editToolbar->insertButton (fbrickbg, ID_FBRICK, TRUE,
			       klocale->translate("Trap (can fall through)"));
    editToolbar->insertButton (ladderbg, ID_LADDER, TRUE,
			       klocale->translate("Ladder"));
    editToolbar->insertButton (hladderbg, ID_HLADDER, TRUE,
			       klocale->translate("Hidden ladder"));
    editToolbar->insertButton (polebg, ID_POLE, TRUE,
			       klocale->translate("Pole (or bar)"));
    editToolbar->insertButton (nuggetbg, ID_NUGGET, TRUE,
			       klocale->translate("Gold nugget"));
#else
    createBtn	= new QToolButton (QPixmap (filenew), "Create Level",
		    QString::null, this, SLOT (createLevel()), editToolbar);
    updateBtn	= new QToolButton (QPixmap (fileopen), "Edit Any Level",
		    QString::null, this, SLOT (updateLevel()), editToolbar);
    savefileBtn	= new QToolButton (QPixmap (filesave), "Save Level",
		    QString::null, this, SLOT (saveLevelFile()), editToolbar);
    editToolbar->addSeparator();

    freeBtn	= new QToolButton (freebg, "Empty space",
		    QString::null, this, SLOT (freeSlot()), editToolbar);
    edheroBtn	= new QToolButton (edherobg, "Hero",
		    QString::null, this, SLOT (edheroSlot()), editToolbar);
    edenemyBtn	= new QToolButton (edenemybg, "Enemy",
		    QString::null, this, SLOT (edenemySlot()), editToolbar);
    brickBtn	= new QToolButton (brickbg, "Brick (can dig)",
		    QString::null, this, SLOT (brickSlot()), editToolbar);
    betonBtn	= new QToolButton (betonbg, "Concrete (cannot dig)",
		    QString::null, this, SLOT (betonSlot()), editToolbar);
    fbrickBtn	= new QToolButton (fbrickbg, "Trap (can fall through)",
		    QString::null, this, SLOT (fbrickSlot()), editToolbar);
    ladderBtn	= new QToolButton (ladderbg, "Ladder",
		    QString::null, this, SLOT (ladderSlot()), editToolbar);
    hladderBtn	= new QToolButton (hladderbg, "Hidden ladder",
		    QString::null, this, SLOT (hladderSlot()), editToolbar);
    poleBtn	= new QToolButton (polebg, "Pole (or bar)",
		    QString::null, this, SLOT (poleSlot()), editToolbar);
    nuggetBtn	= new QToolButton (nuggetbg, "Gold nugget",
		    QString::null, this, SLOT (nuggetSlot()), editToolbar);
    editToolbar->addSeparator();
#endif

#ifdef QT1
    editToolbar->setToggle (ID_FREE, TRUE);
    editToolbar->setToggle (ID_HERO, TRUE);
    editToolbar->setToggle (ID_ENEMY, TRUE);
    editToolbar->setToggle (ID_BRICK, TRUE);
    editToolbar->setToggle (ID_BETON, TRUE);
    editToolbar->setToggle (ID_FBRICK, TRUE);
    editToolbar->setToggle (ID_LADDER, TRUE);
    editToolbar->setToggle (ID_HLADDER, TRUE);
    editToolbar->setToggle (ID_POLE, TRUE);
    editToolbar->setToggle (ID_NUGGET, TRUE);
#else
    freeBtn->setToggleButton (TRUE);
    edheroBtn->setToggleButton (TRUE);
    edenemyBtn->setToggleButton (TRUE);
    brickBtn->setToggleButton (TRUE);
    betonBtn->setToggleButton (TRUE);
    fbrickBtn->setToggleButton (TRUE);
    ladderBtn->setToggleButton (TRUE);
    hladderBtn->setToggleButton (TRUE);
    poleBtn->setToggleButton (TRUE);
    nuggetBtn->setToggleButton (TRUE);
#endif

#ifdef QT1
    pressedButton = ID_BRICK;
    editToolbar->setButton (pressedButton, TRUE);

    addToolBar (editToolbar);
    editToolbar->setBarPos (KToolBar::Top);

    editToolbar->show();
    connect (editToolbar, SIGNAL(clicked(int)), SLOT(commandCallback(int)));
#else
    pressedButton = brickBtn;
    pressedButton->setOn (TRUE);
#endif
}

/******************************************************************************/
/*********************   EDIT ACTION SLOTS   **********************************/
/******************************************************************************/

void KGoldrunner::doEdit (int button)
{
    // Mouse button down: start making changes.
    QPoint p;
    int i, j;

    p = view->getMousePos ();
    i = ((p.x() - 32) / 16) + 1; j = ((p.y() - 32) / 16) + 1;

    switch (button) {
    case LeftButton:
    case RightButton:
        paintEditObj = TRUE;
        insertEditObj (i, j);
        oldI = i;
        oldJ = j;
        break;
    default:
        break;
    }
}

void KGoldrunner::endEdit (int button)
{
    // Mouse button released: finish making changes.
    QPoint p;
    int i, j;

    p = view->getMousePos ();
    i = ((p.x() - 32) / 16) + 1; j = ((p.y() - 32) / 16) + 1;

    switch (button) {
    case LeftButton:
    case RightButton:
        paintEditObj = FALSE;
        if ((i != oldI) || (j != oldJ)) {
	    insertEditObj (i, j);
	}
        break;
    default:
        break;
    }
}

#ifndef QT1
/******************************************************************************/
/*********************   EDIT-BUTTON SLOTS   **********************************/
/******************************************************************************/

void KGoldrunner::freeSlot()	{ editObj = FREE;    setButton (freeBtn);    }
void KGoldrunner::edheroSlot()	{ editObj = HERO;    setButton (edheroBtn);  }
void KGoldrunner::edenemySlot()	{ editObj = ENEMY;   setButton (edenemyBtn); }
void KGoldrunner::brickSlot()	{ editObj = BRICK;   setButton (brickBtn);   }
void KGoldrunner::betonSlot()	{ editObj = BETON;   setButton (betonBtn);   }
void KGoldrunner::fbrickSlot()	{ editObj = FBRICK;  setButton (fbrickBtn);  }
void KGoldrunner::ladderSlot()	{ editObj = LADDER;  setButton (ladderBtn);  }
void KGoldrunner::hladderSlot()	{ editObj = HLADDER; setButton (hladderBtn); }
void KGoldrunner::poleSlot()	{ editObj = POLE;    setButton (poleBtn);    }
void KGoldrunner::nuggetSlot()	{ editObj = NUGGET;  setButton (nuggetBtn);  }

void KGoldrunner::setButton (QToolButton * btn)
{
    pressedButton->setOn (FALSE);
    pressedButton = btn;
    pressedButton->setOn (TRUE);
}
#endif

/******************************************************************************/
/*************************   COLLECTIONS HANDLING   ***************************/
/******************************************************************************/

// NOTE: Macros "myStr" and "myChar", defined in "kgoldrunner.h", are used
//       to smooth out differences between Qt 1 and Qt2 QString classes.

void KGoldrunner::mapCollections()
{
    QDir		d;
    KGrCollection *	colln;
    QString		d_path;
    QString		fileName1;
    QString		fileName2;

    // Find KGoldrunner level files, sorted by name (same as numerical order).
    for (colln = collections.first(); colln != 0; colln = collections.next()) {
	d.setPath ((colln->owner == SYSTEM)	? systemDataDir + "levels/"
						: userDataDir + "levels/");
	d_path = d.path();
	if (! d.exists()) {
	    // There is no "levels" sub-directory: OK if game has no levels yet.
	    if (colln->nLevels > 0) {
		QMessageBox::information (this, "Check Games and Levels",
		"There is NO directory \"" + d_path + "/\" to hold levels for"
		"\nthe \"" + colln->name + "\" game.\n\nPlease contact your "
		"System Administrator.");
	    }
	    continue;
	}

	const QFileInfoList * files = d.entryInfoList
			(colln->prefix + "???.grl", QDir::Files, QDir::Name);
	QFileInfoListIterator i (* files);
	QFileInfo * file;

	if ((files->count() <= 0) && (colln->nLevels > 0)) {
	    QMessageBox::information (this, "Check Games and Levels",
	    "There are NO files \"" + d_path + "/" + colln->prefix + "???.grl"
	    "\"\nfor the \"" + colln->name + "\" game.\n\nPlease contact your "
	    "System Administrator.");
	    continue;
	}

	// If the prefix is "level", the first file is the "ENDE" screen.
	int lev = (colln->prefix == "level") ? 0 : 1;

	while ((file = i.current())) {
	    // Get the name of the file found on disk.
	    fileName1 = file->fileName();

	    while (TRUE) {
		// Work out what the file name should be, based on the level no.
		fileName2.setNum (lev);			// Convert to QString.
		fileName2 = fileName2.rightJustify (3,'0'); // Add zeros.
		fileName2.append (".grl");		// Add level-suffix.
		fileName2.prepend (colln->prefix);	// Add colln. prefix.

		if (lev > colln->nLevels) {
		    QMessageBox::information (this, "Check Games and Levels",
			"File \"" + fileName1 + "\" is beyond the highest "
			"level for the\n\"" + colln->name + "\" game and "
			"cannot be played.\n\nPlease contact your System "
			"Administrator.");
		    break;
		}
		else if (fileName1 == fileName2) {
		    lev++;
		    break;
		}
		else if (fileName1.myStr() < fileName2.myStr()) {
		    QMessageBox::information (this, "Check Games and Levels",
			"File \"" + fileName1 + "\" is before the lowest "
			"level for the\n\"" + colln->name + "\" game and "
			"cannot be played.\n\nPlease contact your System "
			"Administrator.");
		    break;
		}
		else {
		    QMessageBox::information (this, "Check Games and Levels",
			"Cannot find file \"" + fileName2 + "\" for the \"" +
			colln->name + "\" game.\n\nPlease contact your "
			"System Administrator.");
		    lev++;
		}
	    }
	    ++i;				// Go to next file info entry.
	}
    }
}

bool KGoldrunner::loadCollections (Owner o)
{
    QString	filePath;

    filePath = ((o == SYSTEM)? systemDataDir : userDataDir) + "games.dat";

    QFile c (filePath);

    if (! c.exists()) {
	// If the user has not yet created a collection, don't worry.
	if (o == SYSTEM) {
	    QMessageBox::information (this, "Load Game Info",
		"Cannot find Game Info file \"" + filePath +
		"\"\nPlease contact your System Administrator.");
	}
	return (FALSE);
    }

    if (! c.open (IO_ReadOnly)) {
	QMessageBox::information (this, "Load Game Info",
	    "Cannot open Game Info file \"" + filePath +
	    "\"\nfor read-only.  Please contact your System Administrator.");
	return (FALSE);
    }

    QString	line = "";
    QString	name = "";
    QString	prefix = "";
    char	settings = ' ';
    int		nLevels = -1;

    int ch = 0;
    while (ch >= 0) {
	ch = c.getch();
	if (((char) ch != '\n') && (ch >= 0)) {
	    // If not end-of-line and not end-of-file, add to the line.
	    if (ch == '\r')		{line += '\n';}
	    else if (ch == '\\')	{ch = c.getch(); line += '\n';}
	    else			{line += (char) ch;}
	}
	else {
	    // If first character is a digit, we have a new collection.
	    if (isdigit(line.myChar(0))) {
		if (nLevels >= 0) {
		    // If previous collection with no "about" exists, load it.
		    collections.append (new KGrCollection
				(o, name, prefix, settings, nLevels, ""));
		    name = ""; prefix = ""; settings = ' '; nLevels = -1;
		}
		// Decode the first (maybe the only) line in the new collection.
		line = line.simplifyWhiteSpace();
		int i, j, len;
		len = line.length();
		i = 0;   j = line.find(' ',i); nLevels = line.left(j).toInt();
		i = j+1; j = line.find(' ',i); settings = line.myChar(i);
		i = j+1; j = line.find(' ',i); prefix  = line.mid(i,j-i);
		i = j+1;                       name    = line.right(len-i);
	    }
	    // If first character is not a digit, the line should be an "about".
	    else if (nLevels >= 0) {
		    collections.append (new KGrCollection
				(o, name, prefix, settings, nLevels, line));
		    name = ""; prefix = ""; settings = ' '; nLevels = -1;
	    }
	    else if (ch >= 0) {
		// Not EOF: it's an empty line or out-of-context "about" line.
		QMessageBox::information (this, "Load Game Info",
		    "Format error in Game Info file \"" + filePath +
		    ".\"\nPlease contact your System Administrator.");
		c.close();
		return (FALSE);
	    }
	    line = "";
	}
    }

    c.close();
    return (TRUE);
}

bool KGoldrunner::saveCollections (Owner o)
{
    QString	filePath;

    if (o != USER) {
	QMessageBox::information (this, "Save Game Info",
	    "You can only modify USER games.");
	return (FALSE);
    }

    filePath = ((o == SYSTEM)? systemDataDir : userDataDir) + "games.dat";

    QFile c (filePath);

    // Open the output file.
    if (! c.open (IO_WriteOnly)) {
	QMessageBox::information (this, "Save Game Info",
		"Cannot open file \"" + filePath +
		"\".\nfor output.  Please contact your System Administrator.");
	return (FALSE);
    }

    // Save the collections.
    KGrCollection *	colln;
    QString		line;
    int			i, len;
    char		ch;

    for (colln = collections.first(); colln != 0; colln = collections.next()) {
	if (colln->owner == o) {
	    line.sprintf ("%03d %c %s %s\n", colln->nLevels, colln->settings,
				colln->prefix.myStr(), colln->name.myStr());
	    len = line.length();
	    for (i = 0; i < len; i++)
		c.putch (line.myChar(i));

	    len = colln->about.length();
	    if (len > 0) {
		for (i = 0; i < len; i++) {
		    ch = colln->about.myChar(i);
		    if (ch != '\n') {
			c.putch (ch);		// Copy the character.
		    }
		    else {
			c.putch ('\\');		// Change newline to \ and n.
			c.putch ('n');
		    }
		}
		c.putch ('\n');			// Add a real newline.
	    }
	}
    }

    c.close();
    return (TRUE);
}

/******************************************************************************/
/**********************    LEVEL SELECTION DIALOG BOX    **********************/
/******************************************************************************/

int KGoldrunner::selectLevel (int action)
{
    slAction = action;
    sl = new QDialog (view, "levelDialog", TRUE,
			WStyle_Customize | WStyle_NormalBorder | WStyle_Title);

    QLabel *		collnL  = new QLabel ("List of Games", sl);
#ifdef QT1
    QButtonGroup *	ownerG  = new QButtonGroup ("Owner", sl);
#else
    QButtonGroup *	ownerG  = new QButtonGroup
				    (1, QButtonGroup::Horizontal, "Owner", sl);
#endif
			systemB = new QRadioButton ("System", ownerG);
			userB   = new QRadioButton ("User",   ownerG);

			colln   = new QListBox (sl);
    			collnD  = new QLabel ("", sl);
    QPushButton *	collnA  = new QPushButton ("More ...", sl);

    QLabel *		numberL = new QLabel ("Level Number", sl);
			display = new QLineEdit (sl);
			number  = new QScrollBar (1, 150, 1, 10, 1,
						  QScrollBar::Horizontal, sl);
    QPushButton *	levelNH  = new QPushButton ("Level Name and Hint", sl);
			slName   = new QLabel (levelName, sl);
			thumbNail = new KGrThumbNail (sl);

    QPushButton *	OK = new QPushButton ("OK", sl);
    QPushButton *	HELP = new QPushButton ("Help", sl);
    QPushButton *	CANCEL = new QPushButton ("Cancel", sl);

    QPoint		p = view->mapToGlobal (QPoint (0,0));
    int			selectedLevel = 0;

    // Halt the game during the dialog.
    modalFreeze = FALSE;
    if (! KGrObj::frozen) {
	modalFreeze = TRUE;
	freeze();
    }

    sl->	setCaption ("Select Game and Level ...");

    sl->	setGeometry (p.x() + 50, p.y() + 50, 310, 290);
    collnL->	setGeometry (10,  10,  100, 20);
    ownerG->	setGeometry (10,  40,  90,  80);
#ifdef QT1
    systemB->	setGeometry (10,  15,  70,  30);
    userB->	setGeometry (10,  45,  70,  30);
#else
    // In QT v2.x the buttons are positioned in the box automatically.
#endif
    colln->	setGeometry (120, 10,  180, 120);
    collnD->	setGeometry (10,  140, 190, 20);
    collnA->	setGeometry (210, 140, 90,  25);
    numberL->	setGeometry (10,  170, 100, 20);
    display->	setGeometry (120, 170, 50,  20);
    number->	setGeometry (10,  200, 190, 15);
    levelNH->	setGeometry (10,  225, 190, 25);
    slName->	setGeometry (10,  225, 190, 25);
    thumbNail->	setGeometry (212, 187, FIELDWIDTH*3+2, FIELDHEIGHT*3+2);
    OK->	setGeometry (10,  260, 90,  25);
    HELP->	setGeometry (110, 260, 90,  25);
    CANCEL->	setGeometry (210, 260, 90,  25);

    number->	setTracking (TRUE);
    OK->	setAccel (Key_Return);
    HELP->	setAccel (Key_F1);
    CANCEL->	setAccel (Key_Escape);

    // Set the actions required when the collection or level changes.
    connect (systemB, SIGNAL (clicked ()), this, SLOT (slSystemOwner()));
    connect (userB,   SIGNAL (clicked ()), this, SLOT (slUserOwner()));

    connect (colln,   SIGNAL (highlighted (int)), this, SLOT (slColln (int)));
    connect (collnA,  SIGNAL (clicked ()), this, SLOT (slAboutColln ()));

#ifdef QT1
    connect (display, SIGNAL (textChanged (const char *)),
		this, SLOT (slUpdate (const char *)));
#else
    connect (display, SIGNAL (textChanged (const QString &)),
		this, SLOT (slUpdate (const QString &)));
#endif

    connect (number,  SIGNAL (valueChanged(int)), this, SLOT(slShowLevel(int)));

    // Enable the name and hint dialog only if saving a new or edited level.
    // At other times they have not been loaded or initialised.
    if ((slAction == ID_CREATE) || (slAction == ID_SAVEFILE)) {
	slName->hide();
	connect (levelNH,  SIGNAL (clicked ()), this, SLOT (slNameAndHint ()));
    }
    else {
	levelNH->setEnabled (FALSE);
	levelNH->hide();
	slName->show();
    }

    // Repaint the thumbnail whenever the collection or level changes.
    connect (systemB, SIGNAL (clicked ()), this, SLOT (slPaintLevel()));
    connect (userB,   SIGNAL (clicked ()), this, SLOT (slPaintLevel()));
    connect (colln,   SIGNAL (highlighted (int)), this, SLOT (slPaintLevel ()));
    connect (number,  SIGNAL (sliderReleased()), this, SLOT (slPaintLevel()));
    connect (number,  SIGNAL (nextLine()), this, SLOT (slPaintLevel()));
    connect (number,  SIGNAL (prevLine()), this, SLOT (slPaintLevel()));
    connect (number,  SIGNAL (nextPage()), this, SLOT (slPaintLevel()));
    connect (number,  SIGNAL (prevPage()), this, SLOT (slPaintLevel()));

    // Set the exits from this dialog box.
    connect (OK,      SIGNAL (clicked ()), sl,   SLOT (accept ()));
    connect (CANCEL,  SIGNAL (clicked ()), sl,   SLOT (reject ()));
    connect (HELP,    SIGNAL (clicked ()), this, SLOT (slHelp ()));

    // Set the default for the level-number in the scrollbar.
    number->setValue (level);

    // Set the radio buttons and the list of collections in the list box.
    slSetOwner (owner);
    slSetCollections (collnIndex);

    // Vary the dialog according to the action.
    switch (slAction) {
    case ID_NEW:	// Must start at level 1, but can choose a collection.
			OK->setText ("Start Game");
			number->setValue (1);
			number->setEnabled(FALSE);
			display->setEnabled(FALSE);
			break;
    case ID_OPEN:	// Can start playing at any level in any collection.
			OK->setText ("Play Level");
			break;
    case ID_UPDATE:	// Can use any level in any collection as edit input.
			OK->setText ("Edit Level");
			break;
    case ID_CREATE:	// Can save a new level only in a USER collection.
			OK->setText ("Save New");
			if (slOwner != USER) {
			    slUserOwner();
			}
			systemB->setEnabled (FALSE);
			break;
    case ID_SAVEFILE:	// Can save an edited level only in a USER collection.
			OK->setText ("Save Change");
			if (slOwner != USER) {
			    slUserOwner();
			}
			systemB->setEnabled (FALSE);
			break;
    case ID_DELEFILE:	// Can delete a level only in a USER collection.
			OK->setText ("Delete Level");
			if (slOwner != USER) {
			    slUserOwner();
			}
			systemB->setEnabled (FALSE);
			break;
    case ID_MOVEFILE:	// Can move a level only into a USER collection.
			OK->setText ("Move to ...");
			if (slOwner != USER) {
			    slUserOwner();
			}
			systemB->setEnabled (FALSE);
			break;
    case ID_ECOLL:	// Can only edit USER collection details.
			OK->setText ("Edit Game Info");
			if (slOwner != USER) {
			    slUserOwner();
			}
			systemB->setEnabled (FALSE);
			break;

    default:		break;			// Keep the default settings.
    }

    // Set value in the line-edit box.
    slShowLevel (number->value());

    display->setFocus();			// Set the keyboard input on.
    display->selectAll();
    display->setCursorPosition (0);

    // Paint a thumbnail sketch of the level.
    thumbNail->setFrameStyle (QFrame::Box | QFrame::Plain);
    thumbNail->setLineWidth (1);
    slPaintLevel();
    thumbNail->show();

    while (sl->exec() == QDialog::Accepted) {	// Run the modal dialog box.
	if ((number->value() >
		collections.at (slCollnOffset + slCollnIndex)->nLevels) &&
	    (action != ID_CREATE) && (action != ID_SAVEFILE) &&
	    (action != ID_MOVEFILE) && (action != ID_ECOLL)) {
	    QString display_text = display->text();
	    QMessageBox::information (this, "Select Level",
		"There is no level " + display_text + " in\n\"" +
		collections.at (slCollnOffset + slCollnIndex)->name +
		"\", so you\ncannot play or edit it.");
	    continue;				// Re-run the dialog box.
	}
	owner = slOwner;			// If "OK", set the results.
	collection = collections.at (slCollnOffset + slCollnIndex);
	if ((slCollnOffset + slCollnIndex) != (collnOffset + collnIndex)) {
	    // Collection has changed: set default rules for new collection.
	    rCheck ((collection->settings == 'K') ? ID_KGR : ID_TRAD);
	}
	collnOffset = slCollnOffset;
	collnIndex = slCollnIndex;
	selectedLevel = number->value();
	break;
    }

    // Unfreeze the game, but only if it was previously unfrozen.
    if (modalFreeze) {
	unfreeze();
	modalFreeze = FALSE;
    }

    delete sl;
    return (selectedLevel);
}

bool KGoldrunner::ownerOK (Owner o)
{
    // Check that this owner has at least one collection.
    KGrCollection * c;
    bool OK = FALSE;

    for (c = collections.first(); c != 0; c = collections.next()) {
	if (c->owner == o) {
	    OK = TRUE;
	    break;
	}
    }

    return (OK);
}

void KGoldrunner::slSetOwner (Owner o)
{
    if (! ownerOK (o)) {
	if (o == USER) {
	    QMessageBox::information (sl, "Set Owner",
		"You cannot change the game owner\n"
		"until you have created a game to\n"
		"hold the levels you compose.  Try\n"
		"menu item \"Create a Game\".");
	    userB->setChecked (FALSE);
	    systemB->setChecked (TRUE);
	}
	else {
	    QMessageBox::information (sl, "Set Owner",
		"Something is wrong.  No SYSTEM games\n"
		"have been loaded.  Please contact your\n"
		"System Administrator.");
	    systemB->setChecked (FALSE);
	}
	return;
    }

    // Set the radio buttons.
    slOwner = o;

    systemB->setChecked (FALSE);
    userB->setChecked (FALSE);
    if (slOwner == SYSTEM)
	systemB->setChecked (TRUE);
    else
	userB->setChecked (TRUE);

    // Display the collections belonging to this owner.
    slSetCollections (0);
}

void KGoldrunner::slSetCollections (int cIndex)
{
    // Set values in the combo box that holds collection names.
    colln->clear();
    slCollnOffset = -1;

    int i;
    int imax = collections.count();

    for (i = 0; i < imax; i++) {
	if (collections.at(i)->owner == slOwner) {
	    colln->insertItem (collections.at(i)->name, -1);
	    if (slCollnOffset < 0) {
		slCollnOffset = i;	// Set the start index for this owner.
	    }
	}
    }

    if (slCollnOffset < 0) {
	return;				// This owner has no collections.
    }

    // Mark the currently selected collection (or default 0).
    colln->setCurrentItem (cIndex);
    colln->setSelected (cIndex, TRUE);

    // Fetch and display information on the selected collection.
    slColln (cIndex);
}

/******************************************************************************/
/*****************    SLOTS USED BY LEVEL SELECTION DIALOG    *****************/
/******************************************************************************/

void KGoldrunner::slSystemOwner () {slSetOwner (SYSTEM);}	// Radio
void KGoldrunner::slUserOwner ()   {slSetOwner (USER);  }	// buttons.

void KGoldrunner::slColln (int i)
{
    if (slCollnOffset < 0) {
	// Ignore the "highlighted" signal caused by inserting in an empty box.
	return;
    }

    // User "highlighted" a new collection (with one click) ...
    colln->setSelected (i, TRUE);			// One click = selected.
    slCollnIndex = i;
    int n = slCollnIndex + slCollnOffset;		// Collection selected.
    int N = collnIndex + collnOffset;			// Current collection.
#ifdef QT1
    if (collections.at(n)->nLevels > 0)
	number->setRange (1, collections.at(n)->nLevels);
    else
	number->setRange (1, 1);			// Avoid range errors.
#else
    if (collections.at(n)->nLevels > 0)
	number->setMaxValue (collections.at(n)->nLevels);
    else
	number->setMaxValue (1);			// Avoid range errors.
#endif

    // Set a default level number for the selected collection.
    switch (slAction) {
    case ID_OPEN:
    case ID_UPDATE:
    case ID_DELEFILE:
    case ID_ECOLL:
	// If selecting the current collection, use the current level number.
	if (n == N)
	    number->setValue (level);
	else
	    number->setValue (1);			// Else use level 1.
	break;
    case ID_CREATE:
    case ID_SAVEFILE:
    case ID_MOVEFILE:
	if ((n == N) && (slAction != ID_CREATE)) {
	    // Saving/moving level in current collection: use current number.
	    number->setValue (level);
	}
	else {
	    // Saving new/edited level or relocating a level: use "nLevels + 1".
#ifdef QT1
	    number->setRange (1, collections.at(n)->nLevels + 1);
#else
	    number->setMaxValue (collections.at(n)->nLevels + 1);
#endif
	    number->setValue (number->maxValue());
	}
	break;
    default:
	number->setValue (1);				// Default is level 1.
	break;
    }

    slShowLevel (number->value());

    QString levCnt;
    levCnt = levCnt.setNum (collections.at(n)->nLevels);
    if (collections.at(n)->settings == 'K')
	collnD->setText ("KGoldrunner settings, " + levCnt + " levels.");
    else
	collnD->setText ("Traditional settings, " + levCnt + " levels.");
}

void KGoldrunner::slAboutColln ()
{
    // User clicked the "About" button ...
    int		n = slCollnIndex + slCollnOffset;
    QString	title = "About \"" + collections.at(n)->name + "\"";

    if (collections.at(n)->about.length() > 0) {
	myMessage (sl, title, collections.at(n)->about);
    }
    else {
	myMessage (sl, title,
	    "Sorry, there is no further information about this game.");
    }
}

void KGoldrunner::slShowLevel (int i)
{
    // Display the level number as the slider is moved.
    QString tmp;
    if (sl) {
	tmp.setNum(i);
	tmp = tmp.rightJustify(3,'0');
	display->setText(tmp.data());
    }
}

#ifdef QT1
void KGoldrunner::slUpdate (const char * text)
#else
void KGoldrunner::slUpdate (const QString & text)
#endif
{
    // Move the slider when a valid level number is entered.
    if (sl) {
	QString s = text;
	bool ok = FALSE;
	int n = s.toInt (&ok);
	if (ok) {
	    number->setValue (n);
	    slPaintLevel();
	}
	else
#ifdef QT1
	    KMsgBox::message (this, klocale->translate("Select Level"),
			klocale->translate("This level number is not valid.\n"
					   "It can NOT be used."));
#else
	    QMessageBox::information (this, "Select Level",
			"This level number is not valid.\n"
					   "It can NOT be used.");
#endif
    }
}

void KGoldrunner::slPaintLevel ()
{
    // Repaint the thumbnail sketch of the level whenever the level changes.
    int		n = slCollnIndex + slCollnOffset;
    if ((n < 0) || (!collections.at(n))) {
	return;					// Owner has no collections.
    }
    QString	filePath = getFilePath
				(slOwner, collections.at(n), number->value());
    thumbNail->setFilePath (filePath, slName);
    thumbNail->repaint();			// Will call "drawContents (p)".
}

void KGoldrunner::slHelp ()
{
    // Help for "Select Game and Level" dialog box.
    QString s =
	"The button at the bottom left echoes the menu action you selected.  "
	"Click it after choosing a game and level - or use \"Cancel\".";

    if (slAction == ID_NEW) {
	s += "\n\nIf this is your first time in KGoldrunner, click \"Cancel\" "
	     "and try the Tutorial game (in the Game or Help menu).  It gives "
	     "you hints as you go.  Otherwise, just click on the name of a "
	     "game (in the list box), then click on the bottom left button to "
	     "start at level 001.  Play begins when you press any key or move "
	     "the mouse.";
    }
    else {
	s += "\n\nClick on the list box to choose a game.  Click on \"System\" "
	     "or \"User\" to see games that come with the system or those you "
	     "have composed with the Editor.  Below the Owner box there is "
	     "\"More\" about the game, how many levels there are and what "
	     "rules the enemies follow (see the Settings menu).\n\n"
	     "You select "
	     "a level number by typing it or using the scroll bar.  As "
	     "you vary the game or level, the thumbnail sketch shows a "
	     "preview of your choice.";
	switch (slAction) {
	case ID_UPDATE:
	    s += "\n\nYou can select System levels for editing (or copying), "
		 "but you must save the result in a game you have created.";
	    break;
	case ID_CREATE:
	    s += "\n\nYou can add a name and hint to your new level here, but "
		 "you must save the level you have created into one of "
		 "your own games.";
	    break;
	case ID_SAVEFILE:
	    s += "\n\nYou can create or edit a name and hint here, before "
		 "saving.  By varying the game or level, you can do a copy or "
		 "\"Save as\", but you must always save into one of your "
		 "own games.  If you save a level into the middle of a series, "
		 "the other levels are automatically re-numbered.";
	    break;
	case ID_DELEFILE:
	    s += "\n\nNote: If you delete a level from the middle of a series "
		 "the other levels are automatically re-numbered.  You can "
		 "only delete levels from one of your own games.";
	    break;
	case ID_MOVEFILE:
	    s += "\n\nTo move (re-number) a level, you must first select it "
		 "by using \"Edit Any Level\", then you can use \"Move Level\" "
		 "to assign it a new number or even a different game.  Other "
		 "levels are automatically re-numbered as required.  You can "
		 "only move levels within your own games.";
	    break;
	case ID_ECOLL:
	    s += "\n\nNote: When editing game info you need only choose a "
		 "game, then you will go to a dialog where you can edit the "
		 "details of the game.";
	    break;
	default:
	    break;
	}
    }

    myMessage (sl, "Help: Select Game and Level", s);
}

void KGoldrunner::slNameAndHint ()
{
    QDialog *		nh = new QDialog (view, "nameHintDialog", TRUE);

    QLabel *		nameL  = new QLabel ("Name of Level", nh);
    QLineEdit *		nhName  = new QLineEdit (nh);

    QLabel *		mleL = new QLabel ("Hint for Level", nh);
    QMultiLineEdit *	mle = new QMultiLineEdit (nh);

    QPushButton *	OK = new QPushButton ("OK", nh);
    QPushButton *	CANCEL = new QPushButton ("Cancel", nh);

    QPoint		p = view->mapToGlobal (QPoint (0,0));

    nh->		setCaption ("Edit/Create Level Name and Hint");

    nh->		setGeometry (p.x() + 50, p.y() + 50, 310, 340);
    nameL->		setGeometry (10,  10,  100, 20);
    nhName->		setGeometry (120, 10,  150, 20);
    mleL->		setGeometry (10,  40,  290, 20);
    mle->		setGeometry (10,  65,  290, 180);

    nhName->		setText (levelName);

    // Configure the QMultiLineEdit box.
#ifndef QT1
    mle->		setWordWrap (QMultiLineEdit::WidgetWidth);
    mle->		setAlignment (AlignLeft);
#endif
    mle->		setFixedVisibleLines (9);

    mle->		setText (levelHint);

    OK->		setGeometry (10,  80 + mle->height(), 90,  25);
    // OK->		setAccel (Key_Return);	// No!  We need it in "mle" box.
    connect (OK, SIGNAL (clicked ()), nh, SLOT (accept ()));

    CANCEL->		setGeometry (210,  80 + mle->height(), 90,  25);
    CANCEL->		setAccel (Key_Escape);
    connect (CANCEL, SIGNAL (clicked ()), nh, SLOT (reject ()));

    nh->		resize (310, 110 + mle->height());

    if (nh->exec()) {
	levelName = nhName->text();
	levelHint = mle->text();
    }

    delete nh;
}

/******************************************************************************/
/**********************    CLASS TO DISPLAY THUMBNAIL   ***********************/
/******************************************************************************/

KGrThumbNail::KGrThumbNail (QWidget * parent, const char * name)
			: QFrame (parent, name)
{
    // Let the parent do all the work.  We need a class here so that
    // QFrame::drawContents (QPainter *) can be re-implemented and
    // the thumbnail can be automatically re-painted when required.
}

void KGrThumbNail::setFilePath (QString & fp, QLabel * sln)
{
    filePath = fp;				// Keep safe copies of file
    lName = sln;				// path and level name field.
}

void KGrThumbNail::drawContents (QPainter * p)	// Activated via "paintEvent".
{
    QFile	openFile;
    QPen	pen = p->pen();
    char	obj = FREE;
    int		fw = 1;				// Set frame width.
    int		n = 3;				// Set thumbnail cell-size.

    pen.setColor (darkGray);
    p->setPen (pen);

    openFile.setName (filePath);
    if ((! openFile.exists()) || (! openFile.open (IO_ReadOnly))) {
	// There is no file, so fill the thumbnail with "FREE" cells.
	p->drawRect (QRect(fw, fw, FIELDWIDTH*n, FIELDHEIGHT*n));
	return;
    }

    for (int j = 0; j < FIELDHEIGHT; j++)
    for (int i = 0; i < FIELDWIDTH; i++) {

	obj = openFile.getch();

	// Set the colour of each object.
	switch (obj) {
	case BRICK:
	case BETON:
	case FBRICK:
	    pen.setColor (QColor(200,50,50)); p->setPen (pen); break;
	case LADDER:
	    pen.setColor (QColor(225,150,70)); p->setPen (pen); break;
	case POLE:
	    pen.setColor (QColor(200,200,200)); p->setPen (pen); break;
	case HERO:
	    pen.setColor (green); p->setPen (pen); break;
	case ENEMY:
	    pen.setColor (blue); p->setPen (pen); break;
	default:
	    // Set the background for FREE, HLADDER and NUGGET.
	    pen.setColor (darkGray); p->setPen (pen); break;
	}

	// Draw nxn pixels as three lines of length n.
	p->drawLine (i*n+fw, j*n+fw, i*n+(n-1)+fw, j*n+fw);
	if (obj == POLE) {
	    // For a pole, only the top line is drawn in white.
	    pen.setColor (darkGray);
	    p->setPen (pen);
	}
	p->drawLine (i*n+fw, j*n+1+fw, i*n+(n-1)+fw, j*n+1+fw);
	p->drawLine (i*n+fw, j*n+2+fw, i*n+(n-1)+fw, j*n+2+fw);

	// For a nugget, add just a vertical touch  of yellow (2 pixels).
	if (obj == NUGGET) {
	    pen.setColor (yellow);
	    p->setPen (pen);
	    p->drawLine (i*n+1+fw, j*n+(n-2)+fw, i*n+1+fw, j*n+(n-1)+fw);
	}
    }

    // Absorb a newline character, then read in the level name (if any).
    int c = openFile.getch();
    QString s = "";
    while ((c = openFile.getch()) != EOF) {
	if (c == '\n')			// Level name is on one line.
	    break;
	s += (char) c;
    }
    lName->setText (s);

    openFile.close();
}

/******************************************************************************/
/**********************    COLLECTION EDIT DIALOG BOX    **********************/
/******************************************************************************/

void KGoldrunner::editCollection (int action)
{
    int lev = level;
    int n = collnIndex + collnOffset;

    // If editing, choose a collection.
    if (action == ID_ECOLL) {
	lev = selectLevel (ID_ECOLL);
	if (lev == 0)
	    return;
	level = lev;
	n = collnIndex + collnOffset;
    }

    QDialog *		ec       = new QDialog (view, "collnDialog", TRUE);

    QLabel *		nameL    = new QLabel ("Name of Game", ec);
    QLineEdit *		ecName   = new QLineEdit (ec);
    QLabel *		prefixL  = new QLabel ("File Name Prefix", ec);
    QLineEdit *		ecPrefix = new QLineEdit (ec);
#ifdef QT1
    QButtonGroup *	ecGrp    = new QButtonGroup ("Settings", ec);
#else
    QButtonGroup *	ecGrp    = new QButtonGroup (1,
					QButtonGroup::Vertical, "Settings", ec);
#endif
			ecKGrB   = new QRadioButton ("KGoldrunner", ecGrp);
			ecTradB  = new QRadioButton ("Traditional", ecGrp);
    QLabel *		nLevL    = new QLabel ("", ec);

    QLabel *		mleL     = new QLabel ("About this game ...", ec);
    QMultiLineEdit *	mle      = new QMultiLineEdit (ec);

    QPushButton *	OK       = new QPushButton ("OK", ec);
    QPushButton *	CANCEL   = new QPushButton ("Cancel", ec);

    QPoint		p        = view->mapToGlobal (QPoint (0,0));

    ec->		setCaption ("Edit/Create Game Info");

    ec->		setGeometry (p.x() + 50, p.y() + 50, 300, 400);
    nameL->		setGeometry (10,  10,  100, 20);
    ecName->		setGeometry (120, 10,  150, 20);
    prefixL->		setGeometry (10,  40,  100, 20);
    ecPrefix->		setGeometry (120, 40,  50,  20);
    nLevL->		setGeometry (180, 40,  100, 20);
    ecGrp->		setGeometry (10,  70,  220, 42);
#ifdef QT1
    ecKGrB->		setGeometry (10,  15,  100, 25);
    ecTradB->		setGeometry (110, 15,  90,  25);
#else
    // In QT v2.x the buttons are positioned in the box automatically.
#endif
    mleL->		setGeometry (10, 115,  280, 20);
    mle->		setGeometry (10, 140,  280, 100);

    if (action == ID_ECOLL) {			// Edit existing collection.
	ecName->	setText (collections.at(n)->name);
	ecPrefix->	setText (collections.at(n)->prefix);
	if (collections.at(n)->nLevels > 0) {
	    // Collection already has some levels, so cannot change the prefix.
	    ecPrefix->	setEnabled (FALSE);
	}
	QString		s;
	nLevL->		setText (s.sprintf ("%03d levels",
					collections.at(n)->nLevels));
	OK->setText ("Save Changes");
    }
    else {					// Create a collection.
	ecName->        setText ("");
	ecPrefix->      setText ("");
	nLevL->         setText ("000 levels");
	OK->setText ("Save New");
    }

    if ((action == ID_NCOLL) || (collections.at(n)->settings == 'K')) {
	ecSetRules ('K');			// KGoldrunner settings.
    }
    else {
	ecSetRules ('T');			// Traditional settings.
    }
    connect (ecKGrB,  SIGNAL (clicked ()), this, SLOT (ecSetKGr ()));
    connect (ecTradB, SIGNAL (clicked ()), this, SLOT (ecSetTrad ()));

    // Configure the QMultiLineEdit box.
#ifndef QT1
    mle->		setWordWrap (QMultiLineEdit::WidgetWidth);
    mle->		setAlignment (AlignLeft);
#endif
    mle->		setFixedVisibleLines (8);

    if ((action == ID_ECOLL) && (collections.at(n)->about.length() > 0)) {
	mle->		setText (collections.at(n)->about);
    }
    else {
	mle->		setText ("");
    }

    OK->		setGeometry (10,  145 + mle->height(), 100,  25);
    // OK->		setAccel (Key_Return);	// No!  We need it in "mle" box.
    connect (OK, SIGNAL (clicked ()), ec, SLOT (accept ()));

    CANCEL->		setGeometry (190,  145 + mle->height(), 100,  25);
    CANCEL->		setAccel (Key_Escape);
    connect (CANCEL, SIGNAL (clicked ()), ec, SLOT (reject ()));

    ec->		resize (300, 175 + mle->height());

    while (ec->exec()) {		// Loop through dialog until valid.

	// Validate the collection details.
	QString ec_text = ecName->text();
	int len = ec_text.length();
	if (len == 0) {
	    QMessageBox::information (this, "Save Game Info",
		"You must enter a name for the game.");
	    continue;
	}

	ec_text = ecPrefix->text();
	len = ec_text.length();
	if (len == 0) {
	    QMessageBox::information (this, "Save Game Info",
		"You must enter a filename prefix for the game.");
	    continue;
	}
	if (len > 5) {
	    QMessageBox::information (this, "Save Game Info",
		"The filename prefix should not be more than 5 characters.");
	    continue;
	}

	bool allAlpha = TRUE;
	for (int i = 0; i < len; i++) {
	    if (! isalpha(ec_text.myChar(i))) {
		allAlpha = FALSE;
		break;
	    }
	}
	if (! allAlpha) {
	    QMessageBox::information (this, "Save Game Info",
		"The filename prefix should be all alphabetic characters.");
	    continue;
	}

	// Save the collection details.
	char settings = 'K';
	if (ecTradB->isChecked()) {
	    settings = 'T';
	}
	if (action == ID_NCOLL) {
	    collections.append (new KGrCollection (USER, ecName->text(),
			ecPrefix->text(), settings, 0, mle->text()));
	}
	else {
	    collection->name		= ecName->text();
	    collection->prefix		= ecPrefix->text();
	    collection->settings	= settings;
	    collection->about		= mle->text();
	}

	saveCollections (USER);
	break;				// All done now.
    }

    delete ec;
}

void KGoldrunner::ecSetRules (const char settings)
{
    ecKGrB->	setChecked (FALSE);
    ecTradB->	setChecked (FALSE);
    if (settings == 'K')
	ecKGrB->	setChecked (TRUE);
    else
	ecTradB->	setChecked (TRUE);
}

void KGoldrunner::ecSetKGr ()  {ecSetRules ('K');}		// Radio
void KGoldrunner::ecSetTrad () {ecSetRules ('T');}		// buttons.

/******************************************************************************/
/**********************    WORD-WRAPPED MESSAGE BOX    ************************/
/******************************************************************************/

void KGoldrunner::myMessage (QWidget * parent, QString title, QString contents)
{
    QDialog *		mm = new QDialog (parent, "myMessage", TRUE,
			WStyle_Customize | WStyle_NormalBorder | WStyle_Title);

    // Make text background grey not white (i.e. same as widget background).
    QPalette		pl = mm->palette();
#ifdef QT1
    QColorGroup		cg = pl.normal();
    QColorGroup		cgNew (cg.foreground(), cg.background(), cg.light(),
			       cg.dark(), cg.mid(), cg.text(), cg.background());
			pl.setNormal (cgNew);
#else
			pl.setColor (QColorGroup::Base, mm->backgroundColor());
#endif
			mm->setPalette (pl);

    QMultiLineEdit *	mle = new QMultiLineEdit (mm);
    QPushButton *	OK = new QPushButton ("OK", mm);

    QPoint		p = parent->mapToGlobal (QPoint (0,0));

    // Halt the game while the message is displayed.
    messageFreeze = FALSE;
    if (!KGrObj::frozen) {
	messageFreeze = TRUE;
	freeze();
    }

    mm->		setCaption (title);

    mm->		setGeometry (p.x() + 100, p.y() + 200, 350, 200);
    mle->		setGeometry (15, 15, 330, 155);

    mle->		setFrameStyle (QFrame::NoFrame);
#ifndef QT1
    mle->		setWordWrap (QMultiLineEdit::WidgetWidth);
    mle->		setAlignment (AlignLeft);
#endif
    mle->		setReadOnly (TRUE);
    mle->		setText (contents);

    mle->		setFixedVisibleLines (10);
#ifdef QT1
			// In QT 1 environment, wrap words into lines.
			wordWrap (mle, 10);
#endif
    
    if (mle->		numLines() < 10) {
	mle->		setFixedVisibleLines (mle->numLines());
    }

    OK->		setGeometry (10,  25 + mle->height(), 50,  25);
    OK->		setAccel (Key_Return);

    connect (OK, SIGNAL (clicked ()), mm, SLOT (accept ()));

    mm->		setFixedSize (350, 55 + mle->height());
    mm->		exec ();

    // Unfreeze the game, but only if it was previosly unfrozen.
    if (messageFreeze) {
	unfreeze();
	messageFreeze = FALSE;
    }

    delete mm;
}

#ifdef QT1
// This code word-wraps a multi-line edit box.  It is not needed with Qt 2.
void KGoldrunner::wordWrap (QMultiLineEdit * mle, int visibleLines)
{
    QString s1 = mle->text() + ' ';	// ' ' forces a break check at the end.
    QString s2;
    QFontMetrics m = mle->fontMetrics();
    int wMax = mle->width();
    int iMax = s1.length();
    int nLines = 0;
    int w = 0;
    int lastBreak = -1;
    int i = 0;
    int iMin = 0;
    char ch = '\0';

    while (TRUE) {

	s2 = "";
	nLines = 1;
	iMin = 0;
	lastBreak = -1;

	for (i = 0; i < iMax; i++) {

	    ch = s1.myChar(i);
	    switch (ch) {

	    case ' ':			// Optional break.
		if (i != iMax - 1)	// Don't copy the last space.
		    s2 = s2 + ch;
		if (i > iMin) {
		    w = m.width ((s1.mid(iMin, i - iMin)).myStr(), -1);
		    if (w > wMax) {
			if (lastBreak > iMin) {
			    // Use the break before this word.
			    s2.replace (lastBreak, 1, "\n");
			    iMin = lastBreak + 1;
			}
			else {
			    // This word is longer than the line.
			    s2.replace (i, 1, "\n");
			    iMin = i + 1;
			}
			nLines++;
		    }
		    lastBreak = i;
		}
		break;

	    case '\n':			// Forced break.
		s2 = s2 + ch;
		if (i > iMin) {
		    w = m.width ((s1.mid(iMin, i - iMin)).myStr(), -1);
		    if ((w > wMax) && (lastBreak > iMin)) {
			// Line too long: put in a break before this word.
			s2.replace (lastBreak, 1, "\n");
			nLines++;
		    }
		}
		iMin = i + 1;
		lastBreak = i;
		nLines++;
		break;

	    default:			// Ordinary text character.
		s2 = s2 + ch;
		break;
	    }
	}

	// Stop when the text will fit the window.
	if ((nLines <= visibleLines - 3) || (wMax < mle->width()))
	    break;

	// Allow room for a scrollbar at the right and try again.
	wMax = wMax - 25;
    }

    // Put the word-wrapped text into the box.
    mle->setText (s2.myStr());
}
#endif

/******************************************************************************/
/**********************    QT2's SIMPLE HTML BROWSER   ************************/
/******************************************************************************/

#ifndef QT1
// The MOC compiler "simply skips any preprocessor directives it encounters"
// (see the QT documentation, "Using the MetaObject Compiler", "Limitations"
// section).  The following lines are commented/uncommented by the "fix_src"
// script, invoked during the "make init" step of installation.

// #include "qt2_help.cpp"

void KGoldrunner::showManual ()
{
    HelpWindow *help = new HelpWindow (systemHTMLDir /* + "index.html" */,
					userDataDir, 0, "help viewer");

    if (QApplication::desktop()->width() > 400 &&
	QApplication::desktop()->height() > 500)
	help->show();
    else
	help->showMaximized();
}

/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <qstatusbar.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qiconset.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qstylesheet.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qapplication.h>
#include <qcombobox.h>
#include <qlistbox.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qobjectlist.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qdatastream.h>
#include <qprinter.h>
#include <qsimplerichtext.h>
#include <qpaintdevicemetrics.h>

#include <ctype.h>

#include "back.xpm"
#include "forward.xpm"
#include "home.xpm"

HelpWindow::HelpWindow( const QString& home_, const QString& _path,
			QWidget* parent, const char *name )
    : QMainWindow( parent, name, WDestructiveClose ),
      pathCombo( 0 ), selectedURL()
{
    saveDir = _path;

    readHistory();
    readBookmarks();

    browser = new QTextBrowser( this );
    browser->mimeSourceFactory()->setFilePath( _path );
    browser->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    connect( browser, SIGNAL( textChanged() ),
	     this, SLOT( textChanged() ) );

    setCentralWidget( browser );

    bool homeFound = FALSE;
    if (! home_.isEmpty() )
	homeFound = setSource( home_ );

    connect( browser, SIGNAL( highlighted( const QString&) ),
	     statusBar(), SLOT( message( const QString&)) );

    // resize( 640,700 );
    resize( 640,500 );

    QPopupMenu* file = new QPopupMenu( this );
    file->insertItem( tr("&New Window"), this, SLOT( newWindow() ), ALT | Key_N );
    file->insertItem( tr("&Open File"), this, SLOT( openFile() ), ALT | Key_O );
    file->insertItem( tr("&Print"), this, SLOT( print() ), ALT | Key_P );
    file->insertSeparator();
    file->insertItem( tr("&Close"), this, SLOT( close() ), ALT | Key_Q );
    // file->insertItem( tr("E&xit"), qApp, SLOT( closeAllWindows() ), ALT | Key_X );

    // The same three icons are used twice each.
    QPixmap backPm (back_xpm);
    QPixmap forwardPm (forward_xpm);
    QPixmap homePm (home_xpm);

    QIconSet icon_back (backPm);
    QIconSet icon_forward (forwardPm);
    QIconSet icon_home (homePm);

    QPopupMenu* go = new QPopupMenu( this );
    backwardId = go->insertItem (icon_back, tr("&Backward"),
				 browser, SLOT (backward()), ALT | Key_Left);
    forwardId =  go->insertItem (icon_forward, tr("&Forward"),
				 browser, SLOT (forward()), ALT | Key_Right);
    go->insertItem (icon_home, tr("&Home"), browser, SLOT (home()));

    QPopupMenu* help = new QPopupMenu( this );
    help->insertItem( tr("&About ..."), this, SLOT( about() ) );
    help->insertItem( tr("About &Qt ..."), this, SLOT( aboutQt() ) );

    hist = new QPopupMenu( this );
    QStringList::Iterator it = history.begin();
    for ( ; it != history.end(); ++it )
	mHistory[ hist->insertItem( *it ) ] = *it;
    connect( hist, SIGNAL( activated( int ) ),
	     this, SLOT( histChosen( int ) ) );

    bookm = new QPopupMenu( this );
    bookm->insertItem( tr( "Add Bookmark" ), this, SLOT( addBookmark() ) );
    bookm->insertSeparator();

    QStringList::Iterator it2 = bookmarks.begin();
    for ( ; it2 != bookmarks.end(); ++it2 )
	mBookmarks[ bookm->insertItem( *it2 ) ] = *it2;
    connect( bookm, SIGNAL( activated( int ) ),
	     this, SLOT( bookmChosen( int ) ) );

    menuBar()->insertItem( tr("&File"), file );
    menuBar()->insertItem( tr("&Go"), go );
    menuBar()->insertItem( tr( "History" ), hist );
    menuBar()->insertItem( tr( "Bookmarks" ), bookm );
    menuBar()->insertSeparator();
    menuBar()->insertItem( tr("&Help"), help );

    menuBar()->setItemEnabled( forwardId, FALSE);
    menuBar()->setItemEnabled( backwardId, FALSE);
    connect( browser, SIGNAL( backwardAvailable( bool ) ),
	     this, SLOT( setBackwardAvailable( bool ) ) );
    connect( browser, SIGNAL( forwardAvailable( bool ) ),
	     this, SLOT( setForwardAvailable( bool ) ) );


    QToolBar* toolbar = new QToolBar( this );
    addToolBar (toolbar, "Toolbar");
    QToolButton* button;

    button = new QToolButton( icon_back, tr("Backward"), "", browser, SLOT(backward()), toolbar );
    connect( browser, SIGNAL( backwardAvailable(bool) ), button, SLOT( setEnabled(bool) ) );
    button->setEnabled( FALSE );
    button = new QToolButton( icon_forward, tr("Forward"), "", browser, SLOT(forward()), toolbar );
    connect( browser, SIGNAL( forwardAvailable(bool) ), button, SLOT( setEnabled(bool) ) );
    button->setEnabled( FALSE );
    button = new QToolButton( icon_home, tr("Home"), "", browser, SLOT(home()), toolbar );

    toolbar->addSeparator();

    pathCombo = new QComboBox( TRUE, toolbar );
    connect( pathCombo, SIGNAL( activated( const QString & ) ),
	     this, SLOT( pathSelected( const QString & ) ) );
    toolbar->setStretchableWidget( pathCombo );
    setRightJustification( TRUE );
    setDockEnabled( Left, FALSE );
    setDockEnabled( Right, FALSE );

    if (homeFound)
	pathCombo->insertItem( home_ );

    browser->setFocus();
}

bool HelpWindow::setSource (const QString & path_)
{
    QString s = path_;
    QFileInfo f (s);
    if (! f.exists()) {
	QMessageBox::information (this, "Set Source File",
	    "Cannot find file \"" + path_ + "\".");
	return (FALSE);
    }
    if (f.isDir()) {
	if (s.right(1) != "/")
	    s += "/";
	f.setFile (s + "index.html");
	if (! f.exists()) {
	    f.setFile (s + "index.htm");
	    if (! f.exists()) {
		QMessageBox::information (this, "Set Source File",
		    "Can find neither \"" + s + "index.html\"\n"
		    "nor \"" + s + "index.htm\".");
		return (FALSE);
	    }
	}
    }
    browser->setSource (f.filePath());
    return (TRUE);
}

void HelpWindow::setBackwardAvailable( bool b)
{
    menuBar()->setItemEnabled( backwardId, b);
}

void HelpWindow::setForwardAvailable( bool b)
{
    menuBar()->setItemEnabled( forwardId, b);
}


void HelpWindow::textChanged()
{
    if ( browser->documentTitle().isNull() )
	setCaption( browser->context() );
    else
	setCaption( browser->documentTitle() ) ;

    selectedURL = caption();
    if ( !selectedURL.isEmpty() && pathCombo ) {
	bool exists = FALSE;
	int i;
	for ( i = 0; i < pathCombo->count(); ++i ) {
	    if ( pathCombo->text( i ) == selectedURL ) {
		exists = TRUE;
		break;
	    }
	}
	if ( !exists ) {
	    pathCombo->insertItem( selectedURL, 0 );
	    pathCombo->setCurrentItem( 0 );
	    mHistory[ hist->insertItem( selectedURL ) ] = selectedURL;
	} else
	    pathCombo->setCurrentItem( i );
	selectedURL = QString::null;
    }
}

HelpWindow::~HelpWindow()
{
    history.clear();
    QMap<int, QString>::Iterator it = mHistory.begin();
    for ( ; it != mHistory.end(); ++it )
	history.append( *it );

    QFile f( saveDir + "/.history" );
    f.open( IO_WriteOnly );
    QDataStream s( &f );
    s << history;
    f.close();

    bookmarks.clear();
    QMap<int, QString>::Iterator it2 = mBookmarks.begin();
    for ( ; it2 != mBookmarks.end(); ++it2 )
	bookmarks.append( *it2 );

    QFile f2( saveDir + "/.bookmarks" );
    f2.open( IO_WriteOnly );
    QDataStream s2( &f2 );
    s2 << bookmarks;
    f2.close();
}

void HelpWindow::about()
{
    QMessageBox::about( this, "HelpViewer Example",
			"<p>This example implements a simple HTML help viewer "
			"using Qt's rich text capabilities</p>"
			"<p>It's just about 100 lines of C++ code, so don't expect too much :-)</p>"
			);
}


void HelpWindow::aboutQt()
{
    QMessageBox::aboutQt( this, "QBrowser" );
}

void HelpWindow::openFile()
{
#ifndef QT_NO_FILEDIALOG
    QString fn = QFileDialog::getOpenFileName( QString::null, QString::null, this );
    if ( !fn.isEmpty() )
	setSource( fn );
#endif
}

void HelpWindow::newWindow()
{
    ( new HelpWindow(browser->source(), "qbrowser") )->show();
}

void HelpWindow::print()
{
#ifndef QT_NO_PRINTER
    QPrinter printer;
    printer.setFullPage(TRUE);
    if ( printer.setup() ) {
	QPainter p( &printer );
	QPaintDeviceMetrics metrics(p.device());
	int dpix = metrics.logicalDpiX();
	int dpiy = metrics.logicalDpiY();
	const int margin = 72; // pt
	QRect body(margin*dpix/72, margin*dpiy/72,
		   metrics.width()-margin*dpix/72*2,
		   metrics.height()-margin*dpiy/72*2 );
	QFont font("times", 10);
	QSimpleRichText richText( browser->text(), font, browser->context(), browser->styleSheet(),
				  browser->mimeSourceFactory(), body.height() );
	richText.setWidth( &p, body.width() );
	QRect view( body );
	int page = 1;
	do {
	    p.setClipRect( body );
	    richText.draw( &p, body.left(), body.top(), view, colorGroup() );
	    p.setClipping( FALSE );
	    view.moveBy( 0, body.height() );
	    p.translate( 0 , -body.height() );
	    p.setFont( font );
	    p.drawText( view.right() - p.fontMetrics().width( QString::number(page) ),
			view.bottom() + p.fontMetrics().ascent() + 5, QString::number(page) );
	    if ( view.top()  >= richText.height() )
		break;
	    printer.newPage();
	    page++;
	} while (TRUE);
    }
#endif
}

void HelpWindow::pathSelected( const QString &_path )
{
    if (! setSource( _path )) {
	pathCombo->removeItem (pathCombo->currentItem());
	return;
    }
    QMap<int, QString>::Iterator it = mHistory.begin();
    bool exists = FALSE;
    for ( ; it != mHistory.end(); ++it ) {
	if ( *it == _path ) {
	    exists = TRUE;
	    break;
	}
    }
    if ( !exists )
	mHistory[ hist->insertItem( _path ) ] = _path;
}

void HelpWindow::readHistory()
{
    if ( QFile::exists( saveDir + "/.history" ) ) {
	QFile f( saveDir + "/.history" );
	f.open( IO_ReadOnly );
	QDataStream s( &f );
	s >> history;
	f.close();
	while ( history.count() > 20 )
	    history.remove( history.begin() );
    }
}

void HelpWindow::readBookmarks()
{
    if ( QFile::exists( saveDir + "/.bookmarks" ) ) {
	QFile f( saveDir + "/.bookmarks" );
	f.open( IO_ReadOnly );
	QDataStream s( &f );
	s >> bookmarks;
	f.close();
    }
}

void HelpWindow::histChosen( int i )
{
    if ( mHistory.contains( i ) )
	setSource( mHistory[ i ] );
}

void HelpWindow::bookmChosen( int i )
{
    if ( mBookmarks.contains( i ) )
	setSource( mBookmarks[ i ] );
}

void HelpWindow::addBookmark()
{
    mBookmarks[ bookm->insertItem( caption() ) ] = caption();
}
#endif
