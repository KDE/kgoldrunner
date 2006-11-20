/***************************************************************************
 *   Copyright (C) 2003 by Ian Wadham and Marco Krüger                     *
 *   ianw2@optusnet.com.au                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef KGRGAME_H
#define KGRGAME_H

// Macros to smooth out the differences between Qt 1 and Qt 2 classes.
//
// "myStr" converts a QString object to a C language "char*" character string.
// "myChar" extracts a C language character (type "char") from a QString object.
// "endData" checks for an end-of-file condition.
//
#define myStr		latin1
#define myChar(i)	at((i)).latin1()
#define endData		atEnd

#include <qobject.h>

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

#include <qcolor.h>

/**
Sets up games and levels in KGoldrunner and controls the play.

@author Ian Wadham
*/

class KGrObject;
class KGrHero;
class KGrEnemy;
class KGrCollection;

class KGrGame : public QObject
{
Q_OBJECT
public:
    KGrGame (KGrCanvas * theView, QString theSystemDir, QString theUserDir);
    ~KGrGame();

    bool initCollections();
    KGrHero * getHero();

    int getLevel();

    void startPlaying();

    bool inMouseMode();			// True if the game is in mouse mode.
    bool inEditMode();			// True if the game is in editor mode.
    bool isLoading();			// True if a level is being loaded.

    bool saveOK (bool exiting);		// Check if edits were saved.

    QString	getTitle();		// Collection - Level NNN, Name.

    void setEditObj (char newEditObj);	// Set object for editor to paint.

    QString getFilePath (Owner o, KGrCollection * colln, int lev);

public slots:
    void startLevelOne();		// Start any game from level 1.
    void startAnyLevel();		// Start any game from any level.
    void startNextLevel();		// Start next level of current game.

    void setMouseMode (bool on_off);	// Set mouse OR keyboard control.
    void startLevel (int startingAt, int requestedLevel);
    void newGame (const int lev, const int gameIndex);
    void startTutorial();		// Start tutorial game.
    void showHint();			// Show hint for current level.

    void showHighScores();		// Show high scores for current game.

    void incScore(int);			// Update the score.
    void herosDead();			// Hero was caught or he quit (key Q).
    void showHiddenLadders();		// Show hidden ladders (nuggets gone).
    void goUpOneLevel();		// Hero completed the level.
    void loseNugget();			// Nugget destroyed (not collected).
    void heroAction (KBAction movement);// Move hero under keyboard control.

    void saveGame();			// Save game ID, score and level.
    void loadGame();			// Re-load game, score and level.

signals:
    void showScore (long);		// For main window to show the score.
    void showLives (long);		// For main window to show lives left.
    void showLevel (int);		// For main window to show the level.

    void hintAvailable (bool);		// For main window to adjust menu text.

    void setEditMenu (bool);		// Enable/Disable edit menu items.
    void defaultEditObj();		// Set default edit-toolbar button.

    void markRuleType (char);		// Mark KGoldrunner/Traditional rules.
    void gameFreeze (bool);		// Do visual feedback in the GUI.

private slots:
    void finalBreath ();		// Hero is dead: re-start the level.
    void readMousePos ();		// Timed reading of mouse position.
    void doDig (int button);		// Dig when under mouse-button control.

private:
    void setBlankLevel (bool playable);
    int  loadLevel (int levelNo);
    bool openLevelFile (int levelNo, QFile & openlevel);
    void changeObject (unsigned char kind, int i, int j);
    void createObject (KGrObject *o, char picType, int x, int y);
    void setTimings ();
    void initSearchMatrix();

    void checkHighScore();		// Check if high score for current game.

    int  selectLevel (int action, int requestedLevel);
    int  selectedGame;

    void restart();			// Kickstart the game action.

/******************************************************************************/
/**************************  PLAYFIELD AND GAME DATA  *************************/
/******************************************************************************/

private:
    KGrCanvas *			view;		// Where the game is displayed.
    QString			systemDataDir;	// System games are stored here.
    QString			userDataDir;	// User games are stored here.

    KGrObject *		playfield[30][22];	// Array of playfield objects.
    char		editObjArray[30][22];	// Character-code equivalent.
    char		lastSaveArray[30][22];	// Copy for use in "saveOK()".

    int				level;		// Current play/edit level.
    QString			levelName;	// Level name (optional).
    QString			levelHint;	// Level hint (optional).

    long			lives;		// Lives remaining.
    long			score;		// Current score.
    long			startScore;	// Score at start of level.

    KGrHero *			hero;		// The HERO figure !!  Yay !!!
    int				startI, startJ;	// The hero's starting position.

#ifdef QT3
    QPtrList<KGrEnemy>		enemies;	// The list of enemies.
#else
    QList<KGrEnemy>		enemies;	// The list of enemies.
#endif
    int				enemyCount;	// How many enemies.
    KGrEnemy *			enemy;		// One of the enemies.

    int				nuggets;	// How many gold nuggets.

    bool			newLevel;	// Next level will be a new one.
    bool			loading;	// Stop input until it's loaded.

    bool			modalFreeze;	// Stop game during dialog.
    bool			messageFreeze;	// Stop game during message.

    QTimer *			mouseSampler;	// Timer for mouse tracking.
    QTimer *			dyingTimer;	// For pause when the hero dies.

    int				lgHighlight;	// Row selected in "loadGame()".

/******************************************************************************/
/**************************  AUTHORS' DEBUGGING AIDS **************************/
/******************************************************************************/

public slots:
    void doStep();		// Do one animation step.
    void showFigurePositions();	// Show everybody's co-ordinates.
    void showHeroState();	// Show hero's co-ordinates and state.
    void showEnemyState (int);	// Show enemy's co-ordinates and state.
    void showObjectState();	// Show an object's state.
    void bugFix();		// Turn a bug fix on/off dynamically.
    void startLogging();	// Turn logging on/off.

/******************************************************************************/
/********************  GAME EDITOR PROPERTIES AND METHODS  ********************/
/******************************************************************************/

public slots:			// Slots connected to the Menu and Edit Toolbar.
    void createLevel();		// Set up a blank level-display for edit.
    void updateLevel();         // Update an existing level.
    void updateNext();          // Update the current level + 1.
    void editNameAndHint();	// Run a dialog to edit the level name and hint.
    bool saveLevelFile();	// Save the edited level in a text file (.grl).
    void moveLevelFile();	// Move level to another collection or number.
    void deleteLevelFile();	// Delete a level file.

    void editCollection (int action);

    void setLevel (int lev);	// Set level to be edited.

    void freeze();		// Stop the gameplay action.
    void unfreeze();		// Restart the gameplay action.
    void setMessageFreeze (bool);

private:
    bool mouseMode;		// Flag to set up keyboard OR mouse control.
    bool editMode;		// Flag to change keyboard and mouse functions.
    char editObj;		// Type of object to be painted by the mouse.
    bool paintEditObj;		// Sets painting on/off (toggled by clicking).
    int  oldI, oldJ;		// Last mouse position painted.
    int  editLevel;		// Level to be edited (= 0 for new level).
    int  heroCount;		// Can enter at most one hero.
    bool shouldSave;		// True if name or hint was edited.

private:
    void loadEditLevel (int);	// Load and display an existing level for edit.
    void initEdit();
    void deleteLevel();
    void insertEditObj (int, int);
    void setEditableCell (int, int, char);
    void showEditLevel();
    bool reNumberLevels (int, int, int, int);
    bool ownerOK (Owner o);

    // Pixmaps for repainting objects as they are edited.
    QPixmap digpix[10];
    QPixmap brickbg, fbrickbg;
    QPixmap freebg, nuggetbg, polebg, betonbg, ladderbg, hladderbg;
    QPixmap edherobg, edenemybg;

private slots:
    void doEdit(int);		// For mouse-click when in edit-mode.
    void endEdit(int);		// For mouse-release when in edit-mode.

/******************************************************************************/
/********************   COLLECTION PROPERTIES AND METHODS   *******************/
/******************************************************************************/

private:

// Note that a collection of KGoldrunner levels is the same thing as a "game".
#ifdef QT3
    QPtrList<KGrCollection>	collections;	// List of ALL collections.
#else
    QList<KGrCollection>	collections;	// List of ALL collections.
#endif

    KGrCollection *		collection;	// Collection currently in use.
    Owner			owner;		// Collection owner.
    int				collnIndex;	// Index in collections list.

    void mapCollections();
    bool loadCollections (Owner);
    bool saveCollections (Owner);

/******************************************************************************/
/**********************    WORD-WRAPPED MESSAGE BOX    ************************/
/******************************************************************************/

    void myMessage (QWidget * parent, QString title, QString contents);
};

/******************************************************************************/
/**********************    CLASS TO DISPLAY THUMBNAIL   ***********************/
/******************************************************************************/

class KGrThumbNail : public QFrame
{
public:
    KGrThumbNail (QWidget *parent = 0, const char *name = 0);
    void setFilePath (QString &, QLabel *);	// Set filepath and name field.

    static QColor backgroundColor;
    static QColor brickColor;
    static QColor ladderColor;
    static QColor poleColor;

protected:
    void drawContents (QPainter *);		// Draw a preview of a level.
    QString filePath;
    QLabel * lName;
};

/******************************************************************************/
/***********************    COLLECTION DATA CLASS    **************************/
/******************************************************************************/

// Note that a collection of KGoldrunner levels is the same thing as a "game".
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

#endif
