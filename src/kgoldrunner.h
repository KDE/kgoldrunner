/***************************************************************************
                          kgoldrunner.h  -  description
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

#ifndef KGOLDRUNNER_H
#define KGOLDRUNNER_H

// Macros to smooth out the differences between Qt 1 and Qt 2 classes.
//
// "myStr" converts a QString object to a C language "char*" character string.
// "myChar" extracts a C language character (type "char") from a QString object.
// "endData" checks for an end-of-file condition.
//
#define myStr		latin1
#define myChar(i)	at((i)).latin1()
#define endData		atEnd

/******************************************************************************/
/*****************************    INCLUDEs     ********************************/
/******************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream.h>
#include <ctype.h>

#include <qapp.h>

#include <qmessagebox.h>
#include <qdatetime.h>

#include "kgrobj.h"

#include <qmainwindow.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qstatusbar.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>

#include <qcolor.h>
#include <qkeycode.h>
#ifdef QT3
#include <qptrlist.h>
#else
#include <qlist.h>
#endif
#include <qstring.h>

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdatastream.h>

#include <qdialog.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlistbox.h>
#include <qlabel.h>
#include <qscrollbar.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qmultilineedit.h>

class KGoldrunnerWidget;	// Forward declare the central widget class.

/******************************************************************************/
/******************************   CONSTANTS   *********************************/
/******************************************************************************/

// DateiMenue (Game Menu)
const int ID_GAMEMENU =  100;
const int ID_NEW =  101;
const int ID_OPEN = 102;
const int ID_SAVE = 103;
const int ID_EXIT = 104;
const int ID_NEXT = 105;
const int ID_KILL = 106;
const int ID_SAVEGAME = 107;
const int ID_LOADGAME = 108;
const int ID_HIGH = 110;

// Pause/Resume Toggle
const int ID_PAUSE = 109;

// Edit Menu
const int ID_EDITMENU = 300;
const int ID_CREATE   = 301;
const int ID_UPDATE   = 302;
const int ID_EDITNEXT = 303;
const int ID_SAVEFILE = 304;
const int ID_MOVEFILE = 305;
const int ID_DELEFILE = 306;
const int ID_ECOLL    = 307;
const int ID_NCOLL    = 308;

// Settings Menu
const int ID_OPT      = 400;
const int ID_MOUSE    = 421;
const int ID_KEYBOARD = 422;
const int ID_KGR      = 401;
const int ID_TRAD     = 402;
const int ID_MESLOW   = 403;
const int ID_ACGOLD   = 404;
const int ID_RTHOLE   = 405;
const int ID_RATOP    = 406;
const int ID_SRCH1    = 407;
const int ID_SRCH2    = 408;
const int ID_NSPEED   = 409;
const int ID_BSPEED   = 410;
const int ID_VSPEED   = 411;
const int ID_CSPEED   = 412;
const int ID_ISPEED   = 413;
const int ID_DSPEED   = 414;

// Size Menu
#ifdef QT3
const int ID_SIZE     = 500;
const int ID_LARGER   = 501;
const int ID_SMALLER  = 502;
#endif

// HilfeMenue (Help Menu)
const int ID_HELPMENU = 200;
const int ID_TUTE     = 201;
const int ID_HINT     = 202;
const int ID_MANUAL   = 203;
const int ID_ABOUTKGR = 204;
const int ID_ABOUTQT  = 205;

// Statusbar
const int ID_LIFES      = 5;		// Text posns/lengths in Qt2 status bar.
const int ID_SCORE      = 20;
const int ID_LEVEL      = 37;
const int ID_MSG        = 52;
const int ID_DUMMY      = 80;
const int L_LIFES	= ID_SCORE - ID_LIFES;
const int L_SCORE	= ID_LEVEL - ID_SCORE;
const int L_LEVEL	= ID_MSG   - ID_LEVEL;
const int L_MSG		= ID_DUMMY - ID_MSG;

// Edit toolbar
const int ID_FREE	= 810;
const int ID_HERO	= 811;
const int ID_ENEMY	= 812;
const int ID_LADDER	= 813;
const int ID_HLADDER	= 814;
const int ID_BRICK	= 815;
const int ID_BETON	= 816;
const int ID_FBRICK	= 817;
const int ID_POLE	= 818;
const int ID_NUGGET	= 819;

enum Owner {SYSTEM, USER};

// Keyboard action codes
enum KBAction
    {KB_UP, KB_DOWN, KB_LEFT, KB_RIGHT, KB_DIGLEFT, KB_DIGRIGHT, KB_STOP};

/******************************************************************************/
/***********************    COLLECTION DATA CLASS    **************************/
/******************************************************************************/

class KGrCollection
{
public:
    KGrCollection (Owner o, const QString & n, const QString & p,
		   const char s, int nl, const QString & a);
    Owner	owner;		// Collection owner: "System" or "User".
    QString	name;		// Collection name.
    QString	prefix;		// Collection's filename prefix.
    char	settings;	// Collection rules: KGoldrunner or Traditional.
    int		nLevels;	// Number of levels in the collection.
    QString	about;		// Optional text about the collection.
};

/******************************************************************************/
/**********************    CLASS TO DISPLAY THUMBNAIL   ***********************/
/******************************************************************************/

class KGrThumbNail : public QFrame
{
public:
    KGrThumbNail (QWidget *parent = 0, const char *name = 0);
    void setFilePath (QString &, QLabel *);	// Set filepath and name field.
protected:
    void drawContents (QPainter *);		// Draw a preview of a level.
    QString filePath;
    QLabel * lName;
};

/******************************************************************************/
/**********************    KGOLDRUNNER (MAIN) CLASS    ************************/
/******************************************************************************/

class KGoldrunner : public QMainWindow			// QT2PLUS_ONLY
{

	Q_OBJECT
public:
	KGoldrunner(QWidget *parent = 0, const char *name = 0, WFlags = 0);
	virtual ~KGoldrunner();

/******************************************************************************/
/***********************  MENUS AND APPLICATION CONTROL  **********************/
/******************************************************************************/

private:
	// "getDirectories()" is HIGHLY DEPENDENT on the operating system and
	// desktop environment.  See the implementation code for clues as to
	// where the KGoldrunner files are physically located.

	bool getDirectories ();		// Get directory paths, as below.
	QString systemHTMLDir;		// Where the manual is stored.
	QString systemDataDir;		// Where the system levels are stored.
	QString userDataDir;		// Where the user levels are stored.

	QPopupMenu *file_menu;
	QPopupMenu *edit_menu;
	QPopupMenu *opt_menu;
#ifdef QT3
	QPopupMenu *size_menu;
#endif

	QPopupMenu *help_menu;
	QString    statusString;
	QPushButton *pauseBtn;

	bool exitWarning;
	QString about;

signals:
	void userGuideRequested();

private slots:
        void myQuit();				// Menu "Quit" or window-close.

/******************************************************************************/
/**************************  PLAYFIELD AND GAME DATA  *************************/
/******************************************************************************/

private:
	KGoldrunnerWidget *view;		// Widget to hold playfield.

	QTimer * mouseSampler;			// Timer for mouse tracking.

	KGrObj *playfield[30][22];		// Array of playfield objects.
	char	editObjArray[30][22];		// Character-code equivalent.
	char	lastSaveArray[30][22];		// Copy for use in "saveOK()".

	KGrHero *hero;				// The HERO figure !!!
	int startI, startJ;			// The hero's starting position.
#ifdef QT3
	QPtrList<KGrEnemy> enemies;		// The list of enemies.
#else
	QList<KGrEnemy> enemies;		// The list of enemies.
#endif
	int enemyCount;				// How many enemies.
	KGrEnemy *enemy;			// One of the enemies.

	int nuggets;				// How many nuggets.

#ifdef QT3
	QPtrList<KGrCollection>	collections;	// List of ALL collections.
#else
	QList<KGrCollection>	collections;	// List of ALL collections.
#endif
	KGrCollection *		collection;	// Collection currently in use.
	Owner			owner;		// Collection owner.
	int			collnOffset;	// Owner's index in collections.
	int			collnIndex;	// Index within owner.

	int	level;				// Current play/edit level.
	QString	levelName;			// Level name (optional).
	QString	levelHint;			// Level hint (optional).

	long	lifes;				// Lives remaining.
	long	score;				// Current score.

public:
	QString	getTitle();			// Collection - Level NNN, Name.

/******************************************************************************/
/**************************  GAME CONTROL PROCEDURES  *************************/
/******************************************************************************/

protected:
	void keyPressEvent (QKeyEvent *);

private:
	void setBlankLevel (bool);
	void startLevel (int action);
	void newGame (int);
	void startTutorial();
	void showHint();

	bool mouseMode;			// T = mouse control, F = keyboard.
        bool newLevel;			// T = new level, F = reloading current.
        bool loading;			// Inhibits input until level is loaded.
	int loadLevel (int);		// Loads from a file in a collection.
	bool openLevelFile (int, QFile &);
	void changeObject (unsigned char, int, int);
	void createObject (KGrObj*, char, int, int);
	void setTimings();
	void initSearchMatrix();

	void startPlaying();		// Set the hero and enemies moving.
	void saveGame();		// Save game ID, score and level.
	void loadGame();		// Re-load game, score and level.
	int  lgHighlight;		// Row selected in loadGame QListBox.
	void checkHighScore();		// Check if high score for current game.
	void showHighScores();		// Show high scores for current game.

	void freeze();			// Stop the game.
	void unfreeze();		// Restart the game.

	QString getFilePath (Owner, KGrCollection *, int);

	void initStatusBar();		// Status bar control.
	void changeLevel (int);
	void changeLifes (int);
	void changeScore (int);

	QTimer * dyingTimer;		// Provides a pause when the hero dies.

private slots:
        void commandCallback(int);	// Menu actions.
        void rCheck(int);		// Set/unset ticks on rule menu-items.
        void sCheck(int);		// Set/unset ticks on speed menu-items.
	void pauseResume();		// Stop/restart play.

	void incScore(int);		// Update the score.
	void showHidden();		// Show hidden ladders (nuggets gone).
	void loseNugget();		// Nugget destroyed (not collected).
	void herosDead();		// Hero caught or killed (key K).
	void nextLevel();		// Hero completed the level.

	void readMousePos ();		// Timed reading of mouse position.
	void doDig(int);		// For mouse-click when in play-mode.
	void setKey(KBAction);		// For keyboard control of hero.

	void lgSelect(int);		// User selected a saved game to load.

	void finalBreath ();		// Hero is dead: re-start the level.

/******************************************************************************/
/**************************  AUTHORS' DEBUGGING AIDS **************************/
/******************************************************************************/

private:
	void doStep();			// Do one animation step.
	void showFigurePositions();	// Show everybody's co-ordinates.
	void showHeroState();		// Show hero's co-ordinates and state.
	void showEnemyState (int);	// Show enemy's co-ordinates and state.
	void showObjectState();		// Show an object's state.

/******************************************************************************/
/********************  GAME EDITOR PROPERTIES AND METHODS  ********************/
/******************************************************************************/

private:
	bool editMode;		// Flag to change keyboard and mouse behaviour.
	char editObj;		// Type of object to be painted by the mouse.
	bool paintEditObj;	// Sets painting on/off (toggled by clicking).
	int  oldI, oldJ;	// Last mouse position painted.
	int  editLevel;		// Level to be edited (= 0 for new level).
	int  heroCount;		// Can enter at most one hero.

private slots:			// Slots connected to the Menu and Edit Toolbar.
	void createLevel();	// Set up a blank level-display for edit.
	void updateLevel();	// Select and load an existing level for edit.
	bool saveLevelFile();	// Save the edited level in a text file (.grl).

private:
	void loadEditLevel(int);// Load and display an existing level for edit.
	void moveLevelFile();	// Move level to another collection or number.
	void deleteLevelFile();	// Delete a level file.
	bool saveOK ();		// Check if changes have been saved.
	void initEdit();
	void deleteLevel();
	void insertEditObj (int, int);
	void setEditableCell (int, int, char);
	void showEditLevel();
	bool reNumberLevels (int, int, int, int);

	void makeEditToolbar();
	QToolBar    *editToolbar;
	QToolButton *createBtn, *updateBtn, *savefileBtn;
	QToolButton *freeBtn, *edheroBtn, *edenemyBtn;
	QToolButton *brickBtn, *betonBtn, *fbrickBtn, *ladderBtn, *hladderBtn;
	QToolButton *poleBtn, *nuggetBtn;
	QToolButton *pressedButton;
	void setButton (QToolButton *);

	// Pixmaps for repainting objects as they are edited.
	QPixmap digpix[10];
	QPixmap brickbg, fbrickbg;
	QPixmap freebg, nuggetbg, polebg, betonbg, ladderbg, hladderbg;
	QPixmap edherobg, edenemybg;

private slots:
	void doEdit(int);		// For mouse-click when in edit-mode.
	void endEdit(int);		// For mouse-release when in edit-mode.

	void freeSlot();				// QT2PLUS_ONLY
	void edheroSlot();				// QT2PLUS_ONLY
	void edenemySlot();				// QT2PLUS_ONLY
	void brickSlot();				// QT2PLUS_ONLY
	void betonSlot();				// QT2PLUS_ONLY
	void fbrickSlot();				// QT2PLUS_ONLY
	void ladderSlot();				// QT2PLUS_ONLY
	void hladderSlot();				// QT2PLUS_ONLY
	void poleSlot();				// QT2PLUS_ONLY
	void nuggetSlot();				// QT2PLUS_ONLY

/******************************************************************************/
/*************************   COLLECTIONS HANDLING   ***************************/
/******************************************************************************/

private:
	void mapCollections ();		// Check collections vs. ".grl" files.
	bool loadCollections (Owner);	// Load "collection.dat" file.
	bool saveCollections (Owner);	// Save "collection.dat" file.

/******************************************************************************/
/**********************    LEVEL SELECTION DIALOG BOX    **********************/
/******************************************************************************/

private:
	int selectLevel (int);		// Select collection and level.

	bool		modalFreeze;	// True when game is frozen temporarily.

	QDialog		*sl;		// Popup dialog box.
	QRadioButton	*systemB;	// Selects "System" collections.
	QRadioButton	*userB;		// Selects "User" collections.
	QListBox	*colln;		// Selects collection from a list.
	QLabel		*collnD;	// Describes collection selected.
	QScrollBar	*number;	// Slider to select level number.
	QLineEdit	*display;	// Text-entry version of level number.
	KGrThumbNail	*thumbNail;	// Preview of level selected.
	QLabel		*slName;	// Name of level selected.

	int		slAction;	// Current editing/play action.
	Owner		slOwner;	// Owner of selected collections.
	int		slCollnOffset;	// Index of that owner's collections.
	int		slCollnIndex;	// Index of selected collection.

	bool ownerOK(Owner);		// Check that the owner owns something.
	void slSetOwner(Owner);
	void slSetCollections(int);

private slots:
	void slSystemOwner();
	void slUserOwner();
	void slColln(int);
	void slAboutColln();
	void slShowLevel(int);
	void slUpdate (const QString &);		// QT2PLUS_ONLY
	void slPaintLevel();
	void slHelp();
	void slNameAndHint();

/******************************************************************************/
/**********************    COLLECTION EDIT DIALOG BOX    **********************/
/******************************************************************************/

private:
	void editCollection (int);	// Edit/Create details of a collection.
	QRadioButton	*ecKGrB;
	QRadioButton	*ecTradB;
	void ecSetRules (const char);

private slots:
	void ecSetKGr ();
	void ecSetTrad ();


/******************************************************************************/
/**********************    WORD-WRAPPED MESSAGE BOX    ************************/
/******************************************************************************/

private:
	void myMessage (QWidget *, QString, QString);
	bool	messageFreeze;		// True when game is frozen temporarily.

/******************************************************************************/
/**********************    QT2's SIMPLE HTML BROWSER   ************************/
/******************************************************************************/

// The MOC compiler "simply skips any preprocessor directives it encounters"
// (see the QT documentation, "Using the MetaObject Compiler", "Limitations"
// section).  The following lines are commented/uncommented by the "fix_src"
// script, invoked during the "make init" step of installation.

private:						// QT2PLUS_ONLY
	void showManual ();				// QT2PLUS_ONLY
};

/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include <qmainwindow.h>
#include <qtextbrowser.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qdir.h>

class QComboBox;
class QPopupMenu;

// The MOC compiler "simply skips any preprocessor directives it encounters"
// (see the QT documentation, "Using the MetaObject Compiler", "Limitations"
// section).  The following lines are commented/uncommented by the "fix_src"
// script, invoked during the "make init" step of installation.

class HelpWindow : public QMainWindow
{
    Q_OBJECT						// QT2PLUS_ONLY
public:
    HelpWindow( const QString& home_,  const QString& path, QWidget* parent = 0, const char *name=0 );
    ~HelpWindow();

private slots:						// QT2PLUS_ONLY
    void setBackwardAvailable( bool );			// QT2PLUS_ONLY
    void setForwardAvailable( bool );			// QT2PLUS_ONLY

    void textChanged();					// QT2PLUS_ONLY
    void about();					// QT2PLUS_ONLY
    void aboutQt();					// QT2PLUS_ONLY
    void openFile();					// QT2PLUS_ONLY
    void newWindow();					// QT2PLUS_ONLY
    void print();					// QT2PLUS_ONLY

    void pathSelected( const QString & );		// QT2PLUS_ONLY
    void histChosen( int );				// QT2PLUS_ONLY
    void bookmChosen( int );				// QT2PLUS_ONLY
    void addBookmark();					// QT2PLUS_ONLY

private:
    QString saveDir;			// Where to save history and bookmarks.

    bool setSource (const QString & path_);
    void readHistory();
    void readBookmarks();

    QTextBrowser* browser;
    QComboBox *pathCombo;
    int backwardId, forwardId;
    QString selectedURL;
    QStringList history, bookmarks;
    QMap<int, QString> mHistory, mBookmarks;
    QPopupMenu *hist, *bookm;

};

#endif // HELPWINDOW_H

#endif // KGOLDRUNNER_H
