/***************************************************************************
    Copyright 2003 Marco Krüger <grisuji@gmx.de>
    Copyright 2003 Ian Wadham <ianw2@optusnet.com.au>
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
//
#define myStr		toLatin1().constData
#define myChar(i)	at((i)).toLatin1()

#include "kgrglobals.h"

#include <QObject>
#include <QList>
#include <QVector>

/**
 * Sets up games and levels in KGoldrunner and controls the play.
 *
 * @short   KGoldrunner Game Controller.
 */
class KGrCanvas;
class KDialog;
class KGrSoundBank;
class KGrEditor;
class KGrLevelPlayer;
class KRandomSequence;
class QTimer;

class KGrGame : public QObject
{
Q_OBJECT
public:
    KGrGame (KGrCanvas * theView,
             const QString & theSystemDir, const QString & theUserDir);
    ~KGrGame();

    bool initGameLists();

    // TODO - Needs to be public?
    void quickStartDialog();

    void setInitialTheme (const QString & themeFilepath);

    // TODO - Needed?
    bool inEditMode();			// True if the game is in editor mode.

    bool saveOK();			// Check if edits were saved.

    // Flags to control author's debugging aids.
    static bool bugFix;
    static bool logging;

public slots:
    void initGame();			// Do the game object's first painting.

    void gameActions        (const int action);
    void editActions        (const int action);
    void editToolbarActions (const int action);
    void settings           (const int action);

    void kbControl          (const int dirn);

    void incScore           (const int n);	// Update the score.

private:
    bool modeSwitch (const int action,
                     int & selectedGame, int & selectedLevel);
    bool selectGame (const SelectAction slAction,
                     int & selectedGame, int & selectedLevel);

    void toggleSoundsOnOff();		// Set sound enabled or disabled.

    // Set mouse, keyboard or laptop-hybrid control of the hero.
    void setControlMode (const int mode);
    void setTimeScale (const int action);

    void newGame   (const int lev, const int gameIndex);
    void runReplay (const int action,
                    const int selectedGame, const int selectedLevel);
    bool startDemo (const Owner demoOwner, const QString & pPrefix,
                                           const int levelNo);
    void runNextDemoLevel();
    void finishDemo();

private slots:
    void interruptDemo();

private:
    void startInstantReplay();
    void replayLastLevel();

    void showHint();			// Show hint for current level.

    QString	getTitle();		// Collection - Level NNN, Name.

    void showHighScores();		// Show high scores for current game.

    void freeze (const bool userAction, const bool on_off);

    QString getDirectory (Owner o);

    void showHiddenLadders();		// Show hidden ladders (nuggets gone).

    void herosDead();			// Hero was caught or he quit (key Q).
    void levelCompleted();		// Hero completed the level.

    // Save game ID, score and level.
    void saveGame();

    // Select a saved game, score and level.
    bool selectSavedGame (int & selectedGame, int & selectedLevel);

    // Load and run a saved game, score and level.
    void loadGame (const int index, const int lev);

    QString loadedData;

private slots:
    void endLevel (const int result);	// Hero completed the level or he died.

    void finalBreath();			// Hero is dead: re-start the level.
    void goUpOneLevel();		// Start next level.

    void heroStep (bool climbing);	// The hero has put a foot on the floor.
    void heroFalls (bool startStop);	// The hero has started/stopped falling.
    void heroDigs();			// The hero is digging.

signals:
    // These signals go to the GUI in most cases.
    void showScore (long);		// For main window to show the score.
    void showLives (long);		// For main window to show lives left.
    void showLevel (int);		// For main window to show the level.

    void hintAvailable (bool);		// For main window to adjust menu text.

    void setEditMenu (bool);		// Enable/Disable edit menu items.

    void gameFreeze (bool);		// Do visual feedback in the GUI.

    void quitGame();			// Used for Quit option in Quick Start.

    // Used to set/clear toggle actions and enable/disable actions.
    void setToggle (const char * actionName, const bool onOff);
    void setAvail  (const char * actionName, const bool onOff);

private:
    KDialog * qs;			// Pointer to Quick Start dialog box.
    QString initialThemeFilepath;

private slots:
    void quickStartPlay();
    void quickStartNewGame();
    void quickStartUseMenu();
    void quickStartQuit();

private:
    bool playLevel (const Owner fileOwner, const QString & prefix,
                    const int levelNo, const bool newLevel);
    void setupLevelPlayer();
    void showTutorialMessages (int levelNo);

    void checkHighScore();		// Check if high score for current game.

    int  selectedGame;

/******************************************************************************/
/**************************  PLAYFIELD AND GAME DATA  *************************/
/******************************************************************************/

    KRandomSequence *           randomGen;	// Random number generator.
    KGrLevelPlayer *            levelPlayer;	// Where the level is played.
    KGrRecording *              recording;	// A recording of the play.
    bool                        playback;	// Play back or record?

    KGrCanvas *			view;		// Where the game is displayed.
    QString			systemDataDir;	// System games are stored here.
    QString			userDataDir;	// User games are stored here.
    int                         timeScale;	// The speed of the game (2-20).
    float                       fTimeScale;	// Speed as a float (0.2-2.0).

    QList<KGrGameData *>        gameList;	// A list of available games.
    int				gameIndex;	// The index in the game-list.
    Owner			owner;		// The game's owner.

    QString                     prefix;		// Prefix for game or demo file.
    int				level;		// Current play/edit/demo level.
    int                         levelMax;	// Last level no in game/demo.
    QString			levelName;	// Level name (optional).
    QString			levelHint;	// Level hint (optional).

    QString                     mainDemoName;	// File-prefix for Main Demo.
    GameAction                  demoType;	// The type of replay or demo.
    bool                        startupDemo;	// Startup demo running?

    Owner                       playbackOwner;	// Owner for current demo-file.
    QString                     playbackPrefix;	// File-prefix for current demo.
    int                         playbackIndex;	// Record-index for curr demo.
    int                         playbackMax;	// Max index for current demo.

    long			lives;		// Lives remaining.
    long			score;		// Current score.
    long			startScore;	// Score at start of level.

    bool			gameFrozen;	// Game stopped.
    bool			programFreeze;	// Stop game during dialog, etc.

    QTimer *			dyingTimer;	// For pause when the hero dies.

    int				lgHighlight;	// Row selected in "loadGame()".

/******************************************************************************/
/*******************************  SOUND SUPPORT *******************************/
/******************************************************************************/
    KGrSoundBank * effects;
    enum { 
	    GoldSound, 
	    StepSound, 
	    ClimbSound, 
	    FallSound, 
	    DigSound, 
	    LadderSound, 
	    DeathSound, 
	    CompletedSound, 
	    VictorySound,
	    GameOverSound,
	    NumSounds };
    QVector<int> fx;

public slots:
    void dbgControl (const int code);	// Authors' debugging aids.

private:
    KGrEditor * editor;		// The level-editor object.

    int controlMode;		// How to control the hero (e.g. K/B or mouse).

/******************************************************************************/
/***********************   GAME PROPERTIES AND METHODS   **********************/
/******************************************************************************/

    bool loadGameData      (Owner);
    bool initRecordingData (const Owner fileOwner, const QString & prefix,
                                                   const int levelNo);
    void saveRecording();
    bool loadRecording     (const QString & dir,   const QString & prefix,
                                                   const int levelNo);
    void loadSounds();

/******************************************************************************/
/**********************    WORD-WRAPPED MESSAGE BOX    ************************/
/******************************************************************************/

    void myMessage (QWidget * parent, const QString &title, const QString &contents);
};

#endif // KGRGAME_H
