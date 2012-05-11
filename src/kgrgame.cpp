#include "kgrdebug.h"

/****************************************************************************
 *    Copyright 2003  Marco Krüger <grisuji@gmx.de>                         *
 *    Copyright 2003  Ian Wadham <iandw.au@gmail.com>                       *
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                       *
 *                                                                          *
 *    This program is free software; you can redistribute it and/or         *
 *    modify it under the terms of the GNU General Public License as        *
 *    published by the Free Software Foundation; either version 2 of        *
 *    the License, or (at your option) any later version.                   *
 *                                                                          *
 *    This program is distributed in the hope that it will be useful,       *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *    GNU General Public License for more details.                          *
 *                                                                          *
 *    You should have received a copy of the GNU General Public License     *
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ****************************************************************************/

#include "kgrgame.h"

#include "kgrcanvas.h"
#include "kgrselector.h"

// KGoldrunner loads and plays .ogg files and requires OpenAL + SndFile > v0.21.
// Fallback to Phonon by the KgSound library does not give good results.
//#include <libkdegames_capabilities.h>
#ifdef KGAUDIO_BACKEND_OPENAL
    #include "kgrsounds.h"
#endif

#include "kgreditor.h"
#include "kgrlevelplayer.h"
#include "kgrdialog.h"
#include "kgrgameio.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>

#include <QStringList>
#include <QTimer>
#include <QDateTime>

#include <KRandomSequence>
#include <KPushButton>
#include <KStandardGuiItem>
#include <KStandardDirs>
#include <KApplication>
#include <KDebug>

// TODO - Can we change over to KScoreDialog?

// Do NOT change KGoldrunner over to KScoreDialog until we have found a way
// to preserve high-score data pre-existing from the KGr high-score methods.
// #define USE_KSCOREDIALOG 1 // IDW - 11 Aug 07.

#ifdef USE_KSCOREDIALOG
#include <KScoreDialog>
#else

#include <QByteArray>
#include <QTextStream>
#include <QLabel>
#include <QVBoxLayout>
#include <QDate>
#include <QSpacerItem>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QDir>

#endif

#define UserPause       true
#define ProgramPause    false
#define NewLevel        true

/******************************************************************************/
/***********************    KGOLDRUNNER GAME CLASS    *************************/
/******************************************************************************/

KGrGame::KGrGame (KGrCanvas * theView, 
                  const QString & theSystemDir, const QString & theUserDir)
        : 
	QObject       (theView),	// Game is destroyed if view closes.
        levelPlayer   (0),
        recording     (0),
        playback      (false),
        view          (theView),
        systemDataDir (theSystemDir),
        userDataDir   (theUserDir),
        level         (0),
        mainDemoName  ("demo"),
        // mainDemoName  ("CM"), // IDW test.
        demoType      (DEMO),
        startupDemo   (false),
        programFreeze (false),
        effects       (0),
        fx            (NumSounds),
        soundOn       (false),
        stepsOn       (false),
        editor        (0)
{
    dbgLevel = 0;

    gameFrozen = false;

    dyingTimer = new QTimer (this);
    connect (dyingTimer, SIGNAL (timeout()),  SLOT (finalBreath()));

    // Initialise random number generator.
    randomGen = new KRandomSequence (time (0));
    kDebug() << "RANDOM NUMBER GENERATOR INITIALISED";
}

KGrGame::~KGrGame()
{
    qDeleteAll(gameList);
    delete randomGen;
    delete levelPlayer;
    delete recording;
}

// Flags to control author's debugging aids.
bool KGrGame::bugFix  = false;		// Start game with dynamic bug-fix OFF.
bool KGrGame::logging = false;		// Start game with dynamic logging OFF.

bool KGrGame::modeSwitch (const int action,
                          int & selectedGame, int & selectedLevel)
{
    // If editing, check that the user has saved changes.
    // Note: KGoldrunner::setEditMenu disables/enables SAVE_GAME, PAUSE,
    // HIGH_SCORE, KILL_HERO, HINT and INSTANT_REPLAY, so they are not relevant
    // here.  All the other GameActions require a save-check.
    if (! saveOK()) {
        return false;			// The user needs to go on editing.
    }
    bool result = true;
    SelectAction slAction = SL_NONE;
    switch (action) {
    case NEW:
        slAction = SL_ANY;
        break;
    case SOLVE:
        slAction = SL_SOLVE;
        break;
    case REPLAY_ANY:
        slAction = SL_REPLAY;
        break;
    case LOAD:
        // Run the Load Game dialog.  Return false if failed or cancelled.
        result = selectSavedGame (selectedGame, selectedLevel);
        break;
    case HIGH_SCORE:			// (Disabled during demo/replay)
    case KILL_HERO:			// (Disabled during demo/replay)
    case PAUSE:
    case HINT:
        return result;			// If demo/replay, keep it going.
        break;
    default:
        break;
    }
    if (slAction != SL_NONE) {
        // Select game and level.  Return false if failed or cancelled.
        result = selectGame (slAction, selectedGame, selectedLevel);
    }
    if (playback && (result == true)) {
        setPlayback (false);		// If demo/replay, kill it.
    }
    return result;
}

void KGrGame::gameActions (const int action)
{
    int selectedGame  = gameIndex;
    int selectedLevel = level;
    if (! modeSwitch (action, selectedGame, selectedLevel)) {
        return;	
    }
    switch (action) {
    case NEW:
        newGame (selectedLevel, selectedGame);
        showTutorialMessages (level);
        break;
    case NEXT_LEVEL:
        if (level >= levelMax) {
            KGrMessage::information (view, i18n ("Play Next Level"),
                i18n ("There are no more levels in this game."));
            return;
        }
        level++;
        kDebug() << "Game" << gameList.at(gameIndex)->name << "level" << level;
        newGame (level, gameIndex);
        showTutorialMessages (level);
        break;
    case LOAD:
        loadGame (selectedGame, selectedLevel);
        break;
    case SAVE_GAME:
        saveGame();
        break;
    case PAUSE:
        freeze (UserPause, (! gameFrozen));
        break;
    case HIGH_SCORE:
        // Stop the action during the high-scores dialog.
        freeze (ProgramPause, true);
        showHighScores();
        freeze (ProgramPause, false);
        break;
    case KILL_HERO:
        if (levelPlayer && (! playback)) {
            // Record the KILL_HERO code, then emit signal endLevel (DEAD).
            levelPlayer->killHero();
        }
	break;
    case HINT:
	showHint();
	break;
    case DEMO:
        setPlayback (true);
        demoType  = DEMO;
        if (! startDemo (SYSTEM, mainDemoName, 1)) {
            setPlayback (false);
        }
	break;
    case SOLVE:
        runReplay (SOLVE, selectedGame, selectedLevel);
	break;
    case INSTANT_REPLAY:
        if (levelPlayer) {
            startInstantReplay();
        }
	break;
    case REPLAY_LAST:
        replayLastLevel();
        break;
    case REPLAY_ANY:
        runReplay (REPLAY_ANY, selectedGame, selectedLevel);
        break;
    default:
	break;
    }
}

void KGrGame::editActions (const int action)
{
    bool editOK    = true;
    bool newEditor = (editor) ? false : true;
    int  editLevel = level;
    dbk << "Level" << level << prefix << gameIndex;
    if (newEditor) {
        if (action == SAVE_EDITS) {
            KGrMessage::information (view, i18n ("Save Level"),
                i18n ("Inappropriate action: you are not editing a level."));
            return;
        }

        // If there is no editor running, start one.
        freeze (ProgramPause, true);
        editor = new KGrEditor (view, systemDataDir, userDataDir, gameList);
        emit setEditMenu (true);	// Enable edit menu items and toolbar.

        // Pass the editor's showLevel signal on to the KGoldrunner GUI object.
        connect (editor, SIGNAL (showLevel(int)),
                 this,   SIGNAL (showLevel(int)));
    }

    switch (action) {
    case CREATE_LEVEL:
	editOK = editor->createLevel (gameIndex);
	break;
    case EDIT_ANY:
	editOK = editor->updateLevel (gameIndex, editLevel);
	break;
    case SAVE_EDITS:
	editOK = editor->saveLevelFile ();
	break;
    case MOVE_LEVEL:
	editOK = editor->moveLevelFile (gameIndex, editLevel);
	break;
    case DELETE_LEVEL:
	editOK = editor->deleteLevelFile (gameIndex, editLevel);
	break;
    case CREATE_GAME:
	editOK = editor->editGame (-1);
	break;
    case EDIT_GAME:
	editOK = editor->editGame (gameIndex);
	break;
    default:
	break;
    }

    if (newEditor) {
        if (editOK) {
            // If a level or demo is running, close it, with no win/lose result.
            setPlayback (false);
            if (levelPlayer) {
                endLevel (NORMAL);
                view->deleteAllSprites();
            }

            emit showLives (0);
            emit showScore (0);
        }
        else {
            // Edit failed or was cancelled, so close the editor.
            emit setEditMenu (false);	// Disable edit menu items and toolbar.
            delete editor;
            editor = 0;
        }
        freeze (ProgramPause, false);
    }

    if (! editOK) {
        return;				// Continue play, demo or previous edit.
    }

    int game = gameIndex, lev = level;
    editor->getGameAndLevel (game, lev);

    if (((game != gameIndex) || (lev != level)) && (lev != 0)) {
        gameIndex = game;
        prefix    = gameList.at (gameIndex)->prefix;
        level     = lev;

        kDebug() << "Saving to KConfigGroup";
        KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
        gameGroup.writeEntry ("GamePrefix", prefix);
        gameGroup.writeEntry ("Level_" + prefix, level);
        gameGroup.sync();		// Ensure that the entry goes to disk.
    }
}

void KGrGame::editToolbarActions (const int action)
{
    // If game-editor is inactive or action-code is not recognised, do nothing.
    if (editor) {
        switch (action) {
        case EDIT_HINT:
            // Edit the level-name or hint.
            editor->editNameAndHint();
	    break;
        case FREE:
        case ENEMY:
        case HERO:
        case CONCRETE:
        case BRICK:
        case FBRICK:
        case HLADDER:
        case LADDER:
        case NUGGET:
        case BAR:
            // Set the next object to be painted in the level-layout.
	    editor->setEditObj (action);
	    break;
        default:
            break;
        }
    }
}

void KGrGame::settings (const int action)
{
    // TODO - Bad   - Configure Keys does not pause a demo. IDW
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    bool onOff = false;
    switch (action) {
    case PLAY_SOUNDS:
    case PLAY_STEPS:
        toggleSoundsOnOff (action);
        break;
    case STARTUP_DEMO:
        // Toggle the startup demo to be on or off.
        onOff = (! gameGroup.readEntry ("StartingDemo", true));
        gameGroup.writeEntry ("StartingDemo", onOff);
        break;
    case MOUSE:
    case KEYBOARD:
    case LAPTOP:
        setControlMode (action);
        gameGroup.writeEntry ("ControlMode", action);
        break;
    case CLICK_KEY:
    case HOLD_KEY:
        if (controlMode == KEYBOARD) {
            setHoldKeyOption (action);
            gameGroup.writeEntry ("HoldKeyOption", action);
        }
        break;
    case NORMAL_SPEED:
    case BEGINNER_SPEED:
    case CHAMPION_SPEED:
        setTimeScale (action);
        gameGroup.writeEntry ("SpeedLevel", action);
        gameGroup.writeEntry ("ActualSpeed", timeScale);
        break;
    case INC_SPEED:
    case DEC_SPEED:
        setTimeScale (action);
        gameGroup.writeEntry ("ActualSpeed", timeScale);
        break;
    default:
        break;
    }
    gameGroup.sync();
}

void KGrGame::setInitialTheme (const QString & themeFilepath)
{
    initialThemeFilepath = themeFilepath;
}

void KGrGame::initGame()
{
#ifndef KGAUDIO_BACKEND_OPENAL
        KGrMessage::information (view, i18n ("No Sound"),
            i18n ("Warning: This copy of KGoldrunner has no sound.\n"
                  "\n"
                  "This is because no development versions of the OpenAL and "
                  "SndFile libraries were present when it was compiled and built."),
                  "WarningNoSound");
#endif
    kDebug() << "Entered, draw the initial graphics now ...";

    // Get the most recent collection and level that was played by this user.
    // If he/she has never played before, set it to Tutorial, level 1.
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    QString prevGamePrefix = gameGroup.readEntry ("GamePrefix", "tute");
    int prevLevel          = gameGroup.readEntry ("Level_" + prevGamePrefix, 1);

    kDebug()<< "Config() Game and Level" << prevGamePrefix << prevLevel;

    // Use that game and level, if it is among the current games.
    // Otherwise, use the first game in the list and level 1.
    gameIndex = 0;
    level     = 1;
    int n     = 0;
    dbk1 << gameIndex << level << "Search:" << prevGamePrefix << prevLevel;
    foreach (KGrGameData * gameData, gameList) {
        dbk1 << "Trying:" << n << gameData->prefix;
        if (gameData->prefix == prevGamePrefix) {
            gameIndex = n;
            level     = prevLevel;
            dbk1 << "FOUND:" << gameIndex << prevGamePrefix << level;
            break;
        }
        n++;
    }

    // Set control-mode, hold-key option (for when K/B is used) and game-speed.
    settings (gameGroup.readEntry ("ControlMode", (int) MOUSE));
    emit setToggle (((controlMode == MOUSE) ?    "mouse_mode" :
                    ((controlMode == KEYBOARD) ? "keyboard_mode" :
                                                 "laptop_mode")), true);

    holdKeyOption = gameGroup.readEntry ("HoldKeyOption", (int) CLICK_KEY);
    emit setToggle (((holdKeyOption == CLICK_KEY) ? "click_key" :
                                                    "hold_key"), true);

    int speedLevel = gameGroup.readEntry ("SpeedLevel", (int) NORMAL_SPEED);
    settings (speedLevel);
    emit setToggle ((speedLevel == NORMAL_SPEED) ?    "normal_speed" :
                    ((speedLevel == BEGINNER_SPEED) ? "beginner_speed" :
                                                      "champion_speed"), true);
    timeScale = gameGroup.readEntry ("ActualSpeed", 10);

#ifdef KGAUDIO_BACKEND_OPENAL
        // Set up sounds, if required in config.
        soundOn = gameGroup.readEntry ("Sound", false);
        kDebug() << "Sound" << soundOn;
        if (soundOn) {
            loadSounds();
            effects->setMuted (false);
        }
        emit setToggle ("options_sounds", soundOn);

        stepsOn = gameGroup.readEntry ("StepSounds", false);
        kDebug() << "StepSounds" << stepsOn;
        emit setToggle ("options_steps", stepsOn);
#endif

    dbk1 << "Owner" << gameList.at (gameIndex)->owner
             << gameList.at (gameIndex)->name << level;

    kDebug() << "Calling the first view->changeTheme() ...";
    view->changeTheme (initialThemeFilepath);

    setPlayback            ( gameGroup.readEntry ("StartingDemo", true));
    if (playback && (startDemo (SYSTEM, mainDemoName, 1))) {
        startupDemo = true;		// Demo is starting.
        demoType    = DEMO;
    }
    else {
        setPlayback (false);		// Load previous session's level.
        newGame (level, gameIndex);
        quickStartDialog();
    }
    emit setToggle ("options_demo", startupDemo);
}

bool KGrGame::startDemo (const Owner demoOwner, const QString & pPrefix,
                                                const int levelNo)
{
    // Find the relevant file and the list of levels it contains.
    QString     dir      = (demoOwner == SYSTEM) ? systemDataDir : userDataDir;
    QString     filepath = dir + "rec_" + pPrefix + ".txt";
    KConfig     config (filepath, KConfig::SimpleConfig);
    QStringList demoList = config.groupList();
    dbk1 << "DEMO LIST" << demoList.count() << demoList;

    // Find the required level (e.g. CM007) in the list available on the file.
    QString s = pPrefix + QString::number(levelNo).rightJustified(3,'0');
    int index = demoList.indexOf (s) + 1;
    dbk1 << "DEMO looking for" << s << "found at" << index;
    if (index <= 0) {
        setPlayback (false);
        kDebug() << "DEMO not found in" << filepath << s << pPrefix << levelNo;
        return false;
    }

    // Load and run the recorded level(s).
    if (playLevel (demoOwner, pPrefix, levelNo, (! NewLevel))) {
        playbackOwner  = demoOwner;
        playbackPrefix = pPrefix;
        playbackIndex  = levelNo;

        // Play back all levels in Main Demo or just one level in other demos.
        playbackMax    = (playbackPrefix == mainDemoName) ?
                          demoList.count() : levelNo;
        if (levelPlayer) {
            levelPlayer->prepareToPlay();
        }
        kDebug() << "DEMO started ..." << filepath << pPrefix << levelNo;
        return true;
    }
    else {
        setPlayback (false);
        kDebug() << "DEMO failed ..." << filepath << pPrefix << levelNo;
        return false;         
    }
}

void KGrGame::runNextDemoLevel()
{
    dbk << "index" << playbackIndex << "max" << playbackMax << playbackPrefix
        << "owner" << playbackOwner;
    if (playbackIndex < playbackMax) {
        playbackIndex++;
        if (playLevel (playbackOwner, playbackPrefix,
                       playbackIndex, (! NewLevel))) {
            if (levelPlayer) {
                levelPlayer->prepareToPlay();
            }
            kDebug() << "DEMO continued ..." << playbackPrefix << playbackIndex;
            return;
        }
    }
    finishDemo();
}

void KGrGame::finishDemo()
{
    setPlayback (false);
    newGame (level, gameIndex);
    if (startupDemo) {
        // The startup demo was running, so run the Quick Start Dialog now.
        startupDemo = false;
        quickStartDialog();
    }
    else {
        // Otherwise, run the last level used.
        showTutorialMessages (level);
    }
}

void KGrGame::interruptDemo()
{
    kDebug() << "DEMO interrupted ...";
    if ((demoType == INSTANT_REPLAY) || (demoType == REPLAY_LAST)) {
        setPlayback (false);
        levelMax = gameList.at (gameIndex)->nLevels;
        freeze (UserPause, true);
        KGrMessage::information (view, i18n ("Game Paused"),
            i18n ("The replay has stopped and the game is pausing while you "
                  "prepare to go on playing. Please press the Pause key "
                  "(default P or Esc) when you are ready."),
                  "Show_interruptDemo");
    }
    else {
        finishDemo();		// Initial DEMO, main DEMO or SOLVE.
    }
}

void KGrGame::startInstantReplay()
{
    dbk << "Start INSTANT_REPLAY";
    demoType = INSTANT_REPLAY;

    // Terminate current play.
    delete levelPlayer;
    levelPlayer = 0;
    view->deleteAllSprites();

    // Redisplay the starting score and lives.
    lives = recording->lives;
    emit showLives (lives);
    score = recording->score;
    emit showScore (score);

    // Restart the level in playback mode.
    setPlayback (true);
    setupLevelPlayer();
    levelPlayer->prepareToPlay();
}

void KGrGame::replayLastLevel()
{
    // Replay the last game and level completed by the player.
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    QString lastPrefix = gameGroup.readEntry ("LastGamePrefix", "");
    int lastLevel      = gameGroup.readEntry ("LastLevel",      -1);

    if (lastLevel > 0) {
        setPlayback (true);
        demoType = REPLAY_LAST;
        if (! startDemo (USER, lastPrefix, lastLevel)) {
            setPlayback (false);
            KGrMessage::information (view, i18n ("Replay Last Level"),
                i18n ("ERROR: Could not find and replay a recording of "
                      "the last level you played."));
        }
    }
    else {
        KGrMessage::information (view, i18n ("Replay Last Level"),
            i18n ("There is no last level to replay.  You need to play a "
                  "level to completion, win or lose, before you can use "
                  "the Replay Last Level action."));
    }
}

/******************************************************************************/
/**********************  QUICK-START DIALOG AND SLOTS  ************************/
/******************************************************************************/

void KGrGame::quickStartDialog()
{
    // Make sure the game does not start during the Quick Start dialog.
    freeze (ProgramPause, true);

    qs = new KDialog (view);

    // Modal dialog, 4 buttons, vertically: the PLAY button has the focus.
    qs->setModal (true);
    qs->setCaption (i18n("Quick Start"));
    qs->setButtons
            (KDialog::Ok | KDialog::Cancel | KDialog::User1 | KDialog::User2);
    qs->setButtonFocus (KDialog::Ok);
    qs->setButtonsOrientation (Qt::Vertical);

    // Set up the PLAY button.
    qs->setButtonText (KDialog::Ok,
            i18nc ("Button text: start playing a game", "&PLAY"));
    qs->setButtonToolTip (KDialog::Ok, i18n ("Start playing this level"));
    qs->setButtonWhatsThis (KDialog::Ok,
            i18n ("Set up to start playing the game and level being shown, "
                 "as soon as you click, move the mouse or press a key"));

    // Set up the Quit button.
    qs->setButtonText (KDialog::Cancel, i18n ("&Quit"));
    qs->setButtonToolTip (KDialog::Cancel, i18n ("Close KGoldrunner"));

    // Set up the New Game button.
    qs->setButtonText (KDialog::User1, i18n ("&New Game..."));
    qs->setButtonToolTip (KDialog::User1,
            i18n ("Start a different game or level"));
    qs->setButtonWhatsThis (KDialog::User1,
            i18n ("Use the Select Game dialog box to choose a "
                 "different game or level and start playing it"));

    // Set up the Use Menu button.
    qs->setButtonText (KDialog::User2, i18n ("&Use Menu"));
    qs->setButtonToolTip (KDialog::User2,
            i18n ("Use the menus to choose other actions"));
    qs->setButtonWhatsThis (KDialog::User2,
            i18n ("Before playing, use the menus to choose other actions, "
                 "such as loading a saved game or changing the theme"));

    // Add the KGoldrunner application icon to the dialog box.
    QLabel * logo = new QLabel();
    qs->setMainWidget (logo);
    logo->setPixmap (kapp->windowIcon().pixmap (240));
    logo->setAlignment (Qt::AlignTop | Qt::AlignHCenter);

    connect (qs, SIGNAL (okClicked()),     this, SLOT (quickStartPlay()));
    connect (qs, SIGNAL (user1Clicked()),  this, SLOT (quickStartNewGame()));
    connect (qs, SIGNAL (user2Clicked()),  this, SLOT (quickStartUseMenu()));
    connect (qs, SIGNAL (cancelClicked()), this, SLOT (quickStartQuit()));

    qs->show();
}

void KGrGame::quickStartPlay()
{
    // KDialog calls QDialog::accept() after the OK slot, so must hide it
    // now, to avoid interference with any tutorial messages there may be.
    qs->hide();
    freeze (ProgramPause, false);
    showTutorialMessages (level);	// Level has already been loaded.
}

void KGrGame::quickStartNewGame()
{
    qs->accept();
    freeze (ProgramPause, false);

    int selectedGame  = gameIndex;
    int selectedLevel = level;
    if (modeSwitch (NEW, selectedGame, selectedLevel)) {
        newGame (selectedLevel, selectedGame);
    }
    showTutorialMessages (selectedLevel);
}

void KGrGame::quickStartUseMenu()
{
    qs->accept();
    freeze (ProgramPause, false);
    freeze (UserPause, true);
    KGrMessage::information (view, i18n ("Game Paused"),
            i18n ("The game is halted. You will need to press the Pause key "
                  "(default P or Esc) when you are ready to play."));

    if (levelPlayer) {
        levelPlayer->prepareToPlay();
    }
}

void KGrGame::quickStartQuit()
{
    emit quitGame();
}

/******************************************************************************/
/*************************  LEVEL SELECTION PROCEDURE  ************************/
/******************************************************************************/

bool KGrGame::selectGame (const SelectAction slAction,
                          int & selectedGame, int & selectedLevel)
{
    // Halt the game during the dialog.
    freeze (ProgramPause, true);

    // Run the game and level selection dialog.
    KGrSLDialog * sl = new KGrSLDialog (slAction, selectedLevel, selectedGame,
                                        gameList, systemDataDir, userDataDir,
                                        view);
    bool selected = sl->selectLevel (selectedGame, selectedLevel);
    delete sl;

    kDebug() << "After dialog - programFreeze" << programFreeze;
    kDebug() << "selected" << selected << "gameFrozen" << gameFrozen;
    kDebug() << "selectedGame" << selectedGame
             << "prefix" << gameList.at(selectedGame)->prefix
             << "selectedLevel" << selectedLevel;
    // Unfreeze the game, but only if it was previously unfrozen.
    freeze (ProgramPause, false);
    return selected;
}

void KGrGame::runReplay (const int action,
                         const int selectedGame, const int selectedLevel)
{
    if (action == SOLVE) {
        setPlayback (true);
        demoType = SOLVE;
        if (! startDemo
            (SYSTEM, gameList.at (selectedGame)->prefix, selectedLevel)) {
            KGrMessage::information (view, i18n ("Show A Solution"),
                i18n ("Sorry, although all levels of KGoldrunner can be "
                      "solved, no solution has been recorded yet for the "
                      "level you selected."), "Show_noSolutionRecorded");
        }
    }
    else if (action == REPLAY_ANY) {
        setPlayback (true);
        demoType = REPLAY_ANY;
        if (! startDemo
            (USER,  gameList.at (selectedGame)->prefix, selectedLevel)) {
            KGrMessage::information (view, i18n ("Replay Any Level"),
                i18n ("Sorry, you do not seem to have played and recorded "
                      "the selected level before."), "Show_noReplay");
        }
    }
}

/******************************************************************************/
/***************************  MAIN GAME PROCEDURES  ***************************/
/******************************************************************************/

void KGrGame::newGame (const int lev, const int newGameIndex)
{
    view->goToBlack();

    KGrGameData * gameData = gameList.at (newGameIndex);
    level     = lev;
    gameIndex = newGameIndex;
    owner     = gameData->owner;
    prefix    = gameData->prefix;
    levelMax  = gameData->nLevels;

    lives = 5;				// Start with 5 lives.
    score = 0;
    startScore = 0;

    emit showLives (lives);
    emit showScore (score);
    emit showLevel (level);

    playLevel (owner, prefix, level, NewLevel);
}

bool KGrGame::playLevel (const Owner fileOwner, const QString & prefix,
                         const int levelNo, const bool newLevel)
{
    // If the game-editor is active, terminate it.
    if (editor) {
        emit setEditMenu (false);	// Disable edit menu items and toolbar.
        delete editor;
        editor = 0;
    }

    // If there is a level being played, kill it, with no win/lose result.
    if (levelPlayer) {
        endLevel (NORMAL);
    }

    // Clean up any sprites remaining from a previous level.  This is done late,
    // so that the player has a little time to observe how the level ended.
    view->deleteAllSprites();

    // Set up to record or play back: load either level-data or recording-data.
    if (! initRecordingData (fileOwner, prefix, levelNo)) {
        return false;
    }

    view->setLevel (levelNo);		// Switch and render background if reqd.
    view->fadeIn();			// Then run the fade-in animation.
    startScore = score;			// The score we will save, if asked.

    // Create a level player, initialised and ready for play or replay to start.
    setupLevelPlayer();

    levelName = recording->levelName;
    levelHint = recording->hint;

    // Indicate on the menus whether there is a hint for this level.
    emit hintAvailable (levelHint.length() > 0);

    // Re-draw the playfield frame, level title and figures.
    view->setTitle (getTitle());

    // If we are starting a new level, save it in the player's config file.
    if (newLevel && (level != 0)) {	// But do not save the "ENDE" level.
        KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
        gameGroup.writeEntry ("GamePrefix", prefix);
        gameGroup.writeEntry ("Level_" + prefix, level);
        gameGroup.sync();		// Ensure that the entry goes to disk.
    }

    return true;
}

void KGrGame::setupLevelPlayer()
{
    levelPlayer = new KGrLevelPlayer (this, randomGen);

    levelPlayer->init (view, recording, playback, gameFrozen);
    levelPlayer->setTimeScale (recording->speed);

    // Use queued connections here, to ensure that levelPlayer has finished
    // executing and can be deleted when control goes to the relevant slot.
    connect (levelPlayer, SIGNAL (endLevel(int)),
             this,        SLOT   (endLevel(int)), Qt::QueuedConnection);
    if (playback) {
        connect (levelPlayer, SIGNAL (interruptDemo()),
                 this,        SLOT   (interruptDemo()),  Qt::QueuedConnection);
    }
}

void KGrGame::incScore (const int n)
{
    score = score + n;		// SCORING: trap enemy 75, kill enemy 75,
    emit showScore (score);	// collect gold 250, complete the level 1500.
}

void KGrGame::playSound (const int n, const bool onOff)
{
#ifdef KGAUDIO_BACKEND_OPENAL
        if (! effects) {
            return;            // Sound is off and not yet loaded.
        }
        static int fallToken = -1;
        if (onOff) {
        int token = -1;
            if (stepsOn || ((n != StepSound) && (n != ClimbSound))) {
                token = effects->play (fx [n]);
            }
            if (n == FallSound) {
                fallToken = token;
            }
        }
        // Only the falling sound can get individually turned off.
        else if ((n == FallSound) && (fallToken >= 0)) {
        effects->stop (fallToken);
        fallToken = -1;
        }
#endif
}

void KGrGame::endLevel (const int result)
{
    dbk << "Return to KGrGame, result:" << result;

#ifdef KGAUDIO_BACKEND_OPENAL
    if (effects) {			// If sounds have been loaded, cut off
        effects->stopAllSounds();	// all sounds that are in progress.
    }
#endif

    if (! levelPlayer) {
        return;			// Not playing a level.
    }

    if (playback && (result == UNEXPECTED_END)) {
        if (demoType == INSTANT_REPLAY) {
            interruptDemo();	// Reached end of recording in instant replay.
        }
        else {
            runNextDemoLevel();	// Finished replay unexpectedly.  Error?
        }
        return;
    }

    dbk << "delete levelPlayer";
    // Delete the level-player, hero, enemies, grid, rule-book, etc.
    // Delete sprites in the view later: the user may need to see them briefly.
    delete levelPlayer;
    levelPlayer = 0;

    // If the player finished the level (won or lost), save the recording.
    if ((! playback) && ((result == WON_LEVEL) || (result == DEAD))) {
        dbk << "saveRecording()";
        saveRecording();
    }

    if (result == WON_LEVEL) {
        dbk << "Won level";
        levelCompleted();
    }
    else if (result == DEAD) {
        dbk << "Lost level";
        herosDead();
    }
}

void KGrGame::herosDead()
{
    if ((level < 1) || (lives <= 0)) {
        return;			// Game over: we are in the "ENDE" screen.
    }

    // Lose a life.
    if ((--lives > 0) || playback) {
        // Demo mode or still some life left.
        emit showLives (lives);

        // Freeze the animation and let the player see what happened.
        freeze (ProgramPause, true);
        playSound (DeathSound);

        dyingTimer->setSingleShot (true);
        dyingTimer->start (1000);
    }
    else {
        // Game over.
        emit showLives (lives);

        freeze (ProgramPause, true);
        playSound (GameOverSound);

        checkHighScore();	// Check if there is a high score for this game.

        // Offer the player a chance to start this level again with 5 new lives.
        QString gameOver = i18n ("<NOBR><B>GAME OVER !!!</B></NOBR><P>"
                                 "Would you like to try this level again?</P>");
        switch (KGrMessage::warning (view, i18n ("Game Over"), gameOver,
                            i18n ("&Try Again"), i18n ("&Finish"))) {
        case 0:
            freeze (ProgramPause, false);		// Offer accepted.
            newGame (level, gameIndex);
            showTutorialMessages (level);
            return;
            break;
        case 1:
            break;				// Offer rejected.
        }

        // Game completely over.
        freeze (ProgramPause, false);		// Unfreeze.
        level = 0;				// Display the "ENDE" screen.
        if (playLevel (SYSTEM, "ende", level, (! NewLevel))) {
            levelPlayer->prepareToPlay();	// Activate the animation.
        }
    }
}

void KGrGame::finalBreath()
{
    dbk << "Connecting fadeFinished()";
    connect (view, SIGNAL (fadeFinished()), this, SLOT (repeatLevel()));
    dbk << "Calling view->fadeOut()";
    view->fadeOut();
}

void KGrGame::repeatLevel()
{
    disconnect (view, SIGNAL (fadeFinished()), this, SLOT (repeatLevel()));

    // Avoid re-starting if the player selected edit before the time was up.
    if (! editor) {
        if (playback) {
            runNextDemoLevel();
        }
        else if (playLevel (owner, prefix, level, (! NewLevel))) {
            levelPlayer->prepareToPlay();
        }
    }
    freeze (ProgramPause, false);	// Unfreeze, but don't move yet.
}

void KGrGame::levelCompleted()
{
    playSound (CompletedSound);

    dbk << "Connecting fadeFinished()";
    connect (view, SIGNAL (fadeFinished()), this, SLOT (goUpOneLevel()));
    dbk << "Calling view->fadeOut()";
    view->fadeOut();
}

void KGrGame::goUpOneLevel()
{
    disconnect (view, SIGNAL (fadeFinished()), this, SLOT (goUpOneLevel()));

    lives++;			// Level completed: gain another life.
    emit showLives (lives);
    incScore (1500);

    if (playback) {
        runNextDemoLevel();
        return;
    }
    if (level >= levelMax) {
        KGrGameData * gameData = gameList.at (gameIndex);
        freeze (ProgramPause, true);
        playSound (VictorySound);

        KGrMessage::information (view, gameData->name,
            i18n ("<b>CONGRATULATIONS !!!!</b>"
            "<p>You have conquered the last level in the "
            "<b>\"%1\"</b> game !!</p>", gameData->name));
        checkHighScore();	// Check if there is a high score for this game.

        freeze (ProgramPause, false);
        level = 0;		// Game completed: display the "ENDE" screen.
    }
    else {
        level++;		// Go up one level.
        emit showLevel (level);
    }

    if (playLevel (owner, prefix, level, NewLevel)) {
        showTutorialMessages (level);
    }
}

void KGrGame::setControlMode (const int mode)
{
    // Enable/disable keyboard-mode options.
    bool enableDisable = (mode == KEYBOARD);
    emit setAvail ("click_key", enableDisable);
    emit setAvail ("hold_key",  enableDisable);

    controlMode = mode;
    if (levelPlayer && (! playback)) {
        // Change control during play, but not during a demo or replay.
        levelPlayer->setControlMode (mode);
    }
}

void KGrGame::setHoldKeyOption (const int option)
{
    holdKeyOption = option;
    if (levelPlayer && (! playback)) {
        // Change key-option during play, but not during a demo or replay.
        levelPlayer->setHoldKeyOption (option);
    }
}

void KGrGame::setTimeScale (const int action)
{
    switch (action) {
    case NORMAL_SPEED:
        timeScale = 10;
        break;
    case BEGINNER_SPEED:
        timeScale = 5;
        break;
    case CHAMPION_SPEED:
        timeScale = 15;
        break;
    case INC_SPEED:
        timeScale = (timeScale < 20) ? timeScale + 1 : 20;
        break;
    case DEC_SPEED:
        timeScale = (timeScale > 2)  ? timeScale - 1 : 2;
        break;
    default:
        break;
    }

    if (levelPlayer && (! playback)) {
        // Change speed during play, but not during a demo or replay.
        kDebug() << "setTimeScale" << (timeScale);
        levelPlayer->setTimeScale (timeScale);
    }
}

bool KGrGame::inEditMode()
{
    return (editor != 0);	// Return true if the game-editor is active.
}

void KGrGame::toggleSoundsOnOff (const int action)
{
    const char * setting = (action == PLAY_SOUNDS) ? "Sound" : "StepSounds";
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    bool soundOnOff = gameGroup.readEntry (setting, false);
    soundOnOff = (! soundOnOff);
    gameGroup.writeEntry (setting, soundOnOff);
    if (action == PLAY_SOUNDS) {
        soundOn = soundOnOff;
    }
    else {
        stepsOn = soundOnOff;
    }

#ifdef KGAUDIO_BACKEND_OPENAL
    if (action == PLAY_SOUNDS) {
        if (soundOn && (effects == 0)) {
            loadSounds();	// Sounds were not loaded when the game started.
        }
        effects->setMuted (! soundOn);
    }
#endif
}

void KGrGame::freeze (const bool userAction, const bool on_off)
{
    QString type = userAction ? "UserAction" : "ProgramAction";
    kDebug() << "PAUSE:" << type << on_off;
    kDebug() << "gameFrozen" << gameFrozen << "programFreeze" << programFreeze;

#ifdef KGAUDIO_BACKEND_OPENAL
    if (on_off && effects) {		// If pausing and sounds are loaded, cut
        effects->stopAllSounds();	// off all sounds that are in progress.
    }
#endif

    if (! userAction) {
        // The program needs to freeze the game during a message, dialog, etc.
        if (on_off) {
            if (gameFrozen) {
                if (programFreeze) {
                    kDebug() << "P: The program has already frozen the game.";
                }
                else {
                    kDebug() << "P: The user has already frozen the game.";
                }
                return;			// The game is already frozen.
            }
            programFreeze = false;
        }
        else if (! programFreeze) {
            if (gameFrozen) {
                kDebug() << "P: The user will keep the game frozen.";
            }
            else {
                kDebug() << "P: The game is NOT frozen.";
            }
            return;			// The user will keep the game frozen.
        }
        // The program will succeed in freezing or unfreezing the game.
        programFreeze = on_off;
    }
    else if (programFreeze) {
        // If the user breaks through a program freeze somehow, take no action.
        kDebug() << "U: THE USER HAS BROKEN THROUGH SOMEHOW.";
        return;
    }
    else {
        // The user is freezing or unfreezing the game.  Do visual feedback.
        emit gameFreeze (on_off);
    }

    gameFrozen = on_off;
    if (levelPlayer) {
        levelPlayer->pause (on_off);
    }
    kDebug() << "RESULT: gameFrozen" << gameFrozen
             << "programFreeze" << programFreeze;
}

void KGrGame::showHint()
{
    // Put out a hint for this level.
    QString caption = i18n ("Hint");

    if (levelHint.length() > 0) {
	freeze (ProgramPause, true);
	// TODO - IDW. Check if a solution exists BEFORE showing the extra button.
	switch (KGrMessage::warning (view, caption, levelHint,
			    i18n ("&OK"), i18n ("&Show A Solution"))) {
	case 0:
	    freeze (ProgramPause, false);	// No replay requested.
	    break;
	case 1:
	    freeze (ProgramPause, false);	// Replay a solution.
	    // This deletes current KGrLevelPlayer and play, but no life is lost.
            runReplay (SOLVE, gameIndex, level);
	    break;
	}
    }
    else
        myMessage (view, caption,
                        i18n ("Sorry, there is no hint for this level."));
}

void KGrGame::showTutorialMessages (int levelNo)
{
    // Halt the game during message displays and mouse pointer moves.
    freeze (ProgramPause, true);

    // Check if this is a tutorial collection and not on the "ENDE" screen.
    if ((prefix.left (4) == "tute") && (levelNo != 0)) {

        // At the start of a tutorial, put out an introduction.
        if (levelNo == 1) {
            KGrMessage::information (view, gameList.at (gameIndex)->name,
                        i18n (gameList.at (gameIndex)->about.constData()));
        }
        // Put out an explanation of this level.
        KGrMessage::information (view, getTitle(), levelHint);
    }

    if (levelPlayer) {
        levelPlayer->prepareToPlay();
    }
    freeze (ProgramPause, false);		// Let the level begin.
}

void KGrGame::setPlayback (const bool onOff)
{
    if (playback != onOff) {
        // Disable high scores, kill hero and some settings during demo/replay.
        bool enableDisable = (! onOff);
        emit setAvail  ("game_highscores", enableDisable);
        emit setAvail  ("kill_hero",       enableDisable);

        emit setAvail  ("mouse_mode",      enableDisable);
        emit setAvail  ("keyboard_mode",   enableDisable);
        emit setAvail  ("laptop_mode",     enableDisable);

        emit setAvail  ("click_key",       enableDisable);
        emit setAvail  ("hold_key",        enableDisable);

        emit setAvail  ("normal_speed",    enableDisable);
        emit setAvail  ("beginner_speed",  enableDisable);
        emit setAvail  ("champion_speed",  enableDisable);
        emit setAvail  ("increase_speed",  enableDisable);
        emit setAvail  ("decrease_speed",  enableDisable);
    }
    view->showReplayMessage (onOff);
    playback = onOff;
}

QString KGrGame::getDirectory (Owner o)
{
    return ((o == SYSTEM) ? systemDataDir : userDataDir);
}

QString KGrGame::getTitle()
{
    int lev = (playback) ? recording->level : level;
    KGrGameData * gameData = gameList.at (gameIndex);
    if (lev == 0) {
        // Generate a special title for end of game.
        return (i18n ("T H E   E N D"));
    }

    // Set title string to "Game-name - NNN" or "Game-name - NNN - Level-name".
    QString gameName = (playback) ? recording->gameName : gameData->name;
    QString levelNumber = QString::number(lev).rightJustified(3,'0');

    QString levelTitle = (levelName.length() <= 0)
                    ?
                    i18nc ("Game name - level number.",
                           "%1 - %2",      gameName, levelNumber)
                    :
                    i18nc ("Game name - level number - level name.",
                           "%1 - %2 - %3", gameName, levelNumber, levelName);
    return (levelTitle);
}

void KGrGame::kbControl (const int dirn, const bool pressed)
{
    dbk2 << "Keystroke setting direction" << dirn << "pressed" << pressed;

    if (editor) {
        return;
    }
    if (playback) {
        levelPlayer->interruptPlayback();	// Will emit interruptDemo().
        return;
    }

    // Using keyboard control can automatically disable mouse control.
    if (pressed && ((controlMode == MOUSE) ||
        ((controlMode == LAPTOP) && (dirn != DIG_RIGHT) && (dirn != DIG_LEFT))))
        {
        // Halt the game while a message is displayed.
        freeze (ProgramPause, true);

        switch (KMessageBox::questionYesNo (view, 
                i18n ("You have pressed a key that can be used to control the "
                "Hero. Do you want to switch automatically to keyboard "
                "control? Pointer control is easier to use in the long term "
                "- like riding a bike rather than walking!"),
                i18n ("Switch to Keyboard Mode"),
                KGuiItem (i18n ("Switch to &Keyboard Mode")),
                KGuiItem (i18n ("Stay in &Mouse Mode")),
                i18n ("Keyboard Mode")))
        {
        case KMessageBox::Yes: 
            settings (KEYBOARD);
            emit setToggle ("keyboard_mode", true);	// Adjust Settings menu.
            break;
        case KMessageBox::No: 
            break;
        }

        // Unfreeze the game, but only if it was previously unfrozen.
        freeze (ProgramPause, false);

        if (controlMode != KEYBOARD) {
            return;                    		// Stay in Mouse or Laptop Mode.
        }
    }

    // Accept keystroke to set next direction, even when the game is frozen.
    if (levelPlayer) {
        levelPlayer->setDirectionByKey ((Direction) dirn, pressed);
    }
}

/******************************************************************************/
/**************************  SAVE AND RE-LOAD GAMES  **************************/
/******************************************************************************/

void KGrGame::saveGame()		// Save game ID, score and level.
{
    if (editor) {
        myMessage (view, i18n ("Save Game"),
        i18n ("Sorry, you cannot save your game play while you are editing. "
        "Please try menu item \"%1\".",
        i18n ("&Save Edits...")));
        return;
    }
    if (playback) {
        return;				//  Avoid saving in playback mode.
    }

    QDate today = QDate::currentDate();
    QTime now =   QTime::currentTime();
    QString saved;
    QString day;
    day = today.shortDayName (today.dayOfWeek());
    saved = saved.sprintf
                ("%-6s %03d %03ld %7ld    %s %04d-%02d-%02d %02d:%02d\n",
                qPrintable(prefix), level, lives, startScore,
                qPrintable(day),
                today.year(), today.month(), today.day(),
                now.hour(), now.minute());

    QFile file1 (userDataDir + "savegame.dat");
    QFile file2 (userDataDir + "savegame.tmp");

    if (! file2.open (QIODevice::WriteOnly)) {
        KGrMessage::information (view, i18n ("Save Game"),
                i18n ("Cannot open file '%1' for output.",
                 userDataDir + "savegame.tmp"));
        return;
    }
    QTextStream text2 (&file2);
    text2 << saved;

    if (file1.exists()) {
        if (! file1.open (QIODevice::ReadOnly)) {
            KGrMessage::information (view, i18n ("Save Game"),
                i18n ("Cannot open file '%1' for read-only.",
                 userDataDir + "savegame.dat"));
            return;
        }

        QTextStream text1 (&file1);
        int n = 30;			// Limit the file to the last 30 saves.
        while ((! text1.atEnd()) && (--n > 0)) {
            saved = text1.readLine() + '\n';
            text2 << saved;
        }
        file1.close();
    }

    file2.close();

    if (KGrGameIO::safeRename (view, userDataDir+"savegame.tmp",
                               userDataDir+"savegame.dat")) {
        KGrMessage::information (view, i18n ("Save Game"),
            i18n ("Please note: for reasons of simplicity, your saved game "
            "position and score will be as they were at the start of this "
            "level, not as they are now."));
    }
    else {
        KGrMessage::information (view, i18n ("Save Game"),
                                i18n ("Error: Failed to save your game."));
    }
}

bool KGrGame::selectSavedGame (int & selectedGame, int & selectedLevel)
{
    selectedGame  = 0;
    selectedLevel = 1;

    QFile savedGames (userDataDir + "savegame.dat");
    if (! savedGames.exists()) {
        // Use myMessage() because it stops the game while the message appears.
        myMessage (view, i18n ("Load Game"),
                         i18n ("Sorry, there are no saved games."));
        return false;
    }

    if (! savedGames.open (QIODevice::ReadOnly)) {
        myMessage (view, i18n ("Load Game"),
                         i18n ("Cannot open file '%1' for read-only.",
                         userDataDir + "savegame.dat"));
        return false;
    }

    // Halt the game during the loadGame() dialog.
    freeze (ProgramPause, true);

    bool result = false;

    loadedData = "";
    KGrLGDialog * lg = new KGrLGDialog (&savedGames, gameList, view);
    if (lg->exec() == QDialog::Accepted) {
        loadedData = lg->getCurrentText();
    }
    delete lg;

    QString pr;
    int index = -1;

    selectedLevel = 0;
    if (! loadedData.isEmpty()) {
        pr = loadedData.mid (21, 7);			// Get the game prefix.
        pr = pr.left (pr.indexOf (" ", 0, Qt::CaseInsensitive));

        for (int i = 0; i < gameList.count(); i++) {	// Find the game.
            if (gameList.at (i)->prefix == pr) {
                index = i;
                break;
            }
        }
        if (index >= 0) {
            selectedGame  = index;
            selectedLevel = loadedData.mid (28, 3).toInt();
            result = true;
        }
        else {
            KGrMessage::information (view, i18n ("Load Game"),
                i18n ("Cannot find the game with prefix '%1'.", pr));
        }
    }

    // Unfreeze the game, but only if it was previously unfrozen.
    freeze (ProgramPause, false);

    return result;
}

void KGrGame::loadGame (const int game, const int lev)
{
    newGame (lev, game);			// Re-start the selected game.
    showTutorialMessages (level);
    lives = loadedData.mid (32, 3).toLong();	// Update the lives.
    emit showLives (lives);
    score = loadedData.mid (36, 7).toLong();	// Update the score.
    emit showScore (score);
}

bool KGrGame::saveOK()
{
    return (editor ? (editor->saveOK()) : true);
}

/******************************************************************************/
/**************************  HIGH-SCORE PROCEDURES  ***************************/
/******************************************************************************/

void KGrGame::checkHighScore()
{
    // Don't keep high scores for tutorial games.
    if ((prefix.left (4) == "tute") || (playback)) {
        return;
    }

    if (score <= 0) {
        return;
    }

#ifdef USE_KSCOREDIALOG
    KScoreDialog scoreDialog (
            KScoreDialog::Name | KScoreDialog::Level | 
            KScoreDialog::Date | KScoreDialog::Score, 
            view);
    scoreDialog.setConfigGroup (prefix);
    KScoreDialog::FieldInfo scoreInfo;
    scoreInfo[KScoreDialog::Level].setNum (level);
    scoreInfo[KScoreDialog::Score].setNum (score);
    QDate today = QDate::currentDate();
    scoreInfo[KScoreDialog::Date] = today.toString ("ddd yyyy MM dd");
    if (scoreDialog.addScore (scoreInfo)) {
        scoreDialog.exec();
    }
#else
    bool	prevHigh  = true;
    qint16	prevLevel = 0;
    qint32	prevScore = 0;
    QString	thisUser  = i18n ("Unknown");
    int		highCount = 0;

    // Look for user's high-score file or for a released high-score file.
    QFile high1 (userDataDir + "hi_" + prefix + ".dat");
    QDataStream s1;

    if (! high1.exists()) {
        high1.setFileName (systemDataDir + "hi_" + prefix + ".dat");
        if (! high1.exists()) {
            prevHigh = false;
        }
    }

    // If a previous high score file exists, check the current score against it.
    if (prevHigh) {
        if (! high1.open (QIODevice::ReadOnly)) {
            QString high1_name = high1.fileName();
            KGrMessage::information (view, i18n ("Check for High Score"),
                i18n ("Cannot open file '%1' for read-only.", high1_name));
            return;
        }

        // Read previous users, levels and scores from the high score file.
        s1.setDevice (&high1);
        bool found = false;
        highCount = 0;
        while (! s1.atEnd()) {
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
                found = true;			// We have a high score.
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

    QFile high2 (userDataDir + "hi_" + prefix + ".tmp");
    QDataStream s2;

    if (! high2.open (QIODevice::WriteOnly)) {
        KGrMessage::information (view, i18n ("Check for High Score"),
                i18n ("Cannot open file '%1' for output.",
                 userDataDir + "hi_" + prefix + ".tmp"));
        return;
    }

    // Dialog to ask the user to enter their name.
    QDialog *		hsn = new QDialog (view,
                        Qt::WindowTitleHint);
    hsn->setObjectName ( QLatin1String("hsNameDialog" ));

    int margin = 10;
    int spacing = 10;
    QVBoxLayout *	mainLayout = new QVBoxLayout (hsn);
    mainLayout->setSpacing (spacing);
    mainLayout->setMargin (margin);

    QLabel *		hsnMessage  = new QLabel (
                        i18n ("<html><b>Congratulations !!!</b><br>"
                        "You have achieved a high score in this game.<br>"
                        "Please enter your name so that it may be enshrined<br>"
                        "in the KGoldrunner Hall of Fame.</html>"),
                        hsn);
    QLineEdit *		hsnUser = new QLineEdit (hsn);
    QPushButton *	OK = new KPushButton (KStandardGuiItem::ok(), hsn);

    mainLayout->	addWidget (hsnMessage);
    mainLayout->	addWidget (hsnUser);
    mainLayout->	addWidget (OK);

    hsn->		setWindowTitle (i18n ("Save High Score"));

    QPoint		p = view->mapToGlobal (QPoint (0,0));
    hsn->		move (p.x() + 50, p.y() + 50);

    OK->		setShortcut (Qt::Key_Return);
    hsnUser->		setFocus();		// Set the keyboard input on.

    connect	(hsnUser, SIGNAL (returnPressed()), hsn, SLOT (accept()));
    connect	(OK,      SIGNAL (clicked()),       hsn, SLOT (accept()));

    // Run the dialog to get the player's name.  Use "-" if nothing is entered.
    hsn->exec();
    thisUser = hsnUser->text();
    if (thisUser.length() <= 0)
        thisUser = QChar('-');
    delete hsn;

    QDate today = QDate::currentDate();
    QString hsDate;
    QString day = today.shortDayName (today.dayOfWeek());
    hsDate = hsDate.sprintf
                ("%s %04d-%02d-%02d",
                qPrintable(day),
                today.year(), today.month(), today.day());

    s2.setDevice (&high2);

    if (prevHigh) {
        high1.reset();
        bool scoreRecorded = false;
        highCount = 0;
        while ((! s1.atEnd()) && (highCount < 10)) {
            char * prevUser;
            char * prevDate;
            s1 >> prevUser;
            s1 >> prevLevel;
            s1 >> prevScore;
            s1 >> prevDate;
            if ((! scoreRecorded) && (score > prevScore)) {
                highCount++;
                // Recode the user's name as UTF-8, in case it contains
                // non-ASCII chars (e.g. "Krüger" is encoded as "KrÃ¼ger").
                s2 << thisUser.toUtf8().constData();
                s2 << (qint16) level;
                s2 << (qint32) score;
                s2 << qPrintable(hsDate);
                scoreRecorded = true;
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
            // Recode the user's name as UTF-8, in case it contains
            // non-ASCII chars (e.g. "Krüger" is encoded as "KrÃ¼ger").
            s2 << thisUser.toUtf8().constData();
            s2 << (qint16) level;
            s2 << (qint32) score;
            s2 << qPrintable(hsDate);
        }
        high1.close();
    }
    else {
        // Recode the user's name as UTF-8, in case it contains
        // non-ASCII chars (e.g. "Krüger" is encoded as "KrÃ¼ger").
        s2 << thisUser.toUtf8().constData();
        s2 << (qint16) level;
        s2 << (qint32) score;
        s2 << qPrintable(hsDate);
    }

    high2.close();

    if (KGrGameIO::safeRename (view, high2.fileName(),
                userDataDir + "hi_" + prefix + ".dat")) {
        // Remove a redundant popup message.
        // KGrMessage::information (view, i18n ("Save High Score"),
                                // i18n ("Your high score has been saved."));
    }
    else {
        KGrMessage::information (view, i18n ("Save High Score"),
                            i18n ("Error: Failed to save your high score."));
    }

    showHighScores();
    return;
#endif
}

void KGrGame::showHighScores()
{
    // Don't keep high scores for tutorial games.
    if (prefix.left (4) == "tute") {
        KGrMessage::information (view, i18n ("Show High Scores"),
                i18n ("Sorry, we do not keep high scores for tutorial games."));
        return;
    }

#ifdef USE_KSCOREDIALOG
    KScoreDialog scoreDialog (
            KScoreDialog::Name | KScoreDialog::Level | 
            KScoreDialog::Date | KScoreDialog::Score, 
            view);
    scoreDialog.exec();
#else
    qint16	prevLevel = 0;
    qint32	prevScore = 0;
    int		n = 0;

    // Look for user's high-score file or for a released high-score file.
    QFile high1 (userDataDir + "hi_" + prefix + ".dat");
    QDataStream s1;

    if (! high1.exists()) {
        high1.setFileName (systemDataDir + "hi_" + prefix + ".dat");
        if (! high1.exists()) {
            KGrMessage::information (view, i18n ("Show High Scores"),
                i18n("Sorry, there are no high scores for the \"%1\" game yet.",
                         gameList.at (gameIndex)->name));
            return;
        }
    }

    if (! high1.open (QIODevice::ReadOnly)) {
        QString high1_name = high1.fileName();
        KGrMessage::information (view, i18n ("Show High Scores"),
            i18n ("Cannot open file '%1' for read-only.", high1_name));
        return;
    }

    QDialog *		hs = new QDialog (view,
                        Qt::WindowTitleHint);
    hs->setObjectName ( QLatin1String("hsDialog" ));

    int margin = 10;
    int spacing = 10;
    QVBoxLayout *	mainLayout = new QVBoxLayout (hs);
    mainLayout->setSpacing (spacing);
    mainLayout->setMargin (margin);

    QLabel *		hsHeader = new QLabel (i18n (
                            "<center><h2>KGoldrunner Hall of Fame</h2></center>"
                            "<center><h3>\"%1\" Game</h3></center>",
                            gameList.at (gameIndex)->name),
                            hs);
    mainLayout->addWidget (hsHeader, 10);

    QTreeWidget * scores = new QTreeWidget (hs);
    mainLayout->addWidget (scores, 50);
    scores->setColumnCount (5);
    scores->setHeaderLabels (QStringList() <<
                            i18nc ("1, 2, 3 etc.", "Rank") <<
                            i18nc ("Person", "Name") <<
                            i18nc ("Game level reached", "Level") <<
                            i18n ("Score") <<
                            i18n ("Date"));
    scores->setRootIsDecorated (false);

    hs->		setWindowTitle (i18n ("High Scores"));

    // Read and display the users, levels and scores from the high score file.
    scores->clear();
    s1.setDevice (&high1);
    n = 0;
    while ((! s1.atEnd()) && (n < 10)) {
        char * prevUser;
        char * prevDate;
        s1 >> prevUser;
        s1 >> prevLevel;
        s1 >> prevScore;
        s1 >> prevDate;

        // prevUser has been saved on file as UTF-8 to allow non=ASCII chars
        // in the user's name (e.g. "Krüger" is encoded as "KrÃ¼ger" in UTF-8).
        QStringList data;
        data << QString().setNum (n+1)
            << QString().fromUtf8 (prevUser)
            << QString().setNum (prevLevel)
            << QString().setNum (prevScore)
            << QString().fromUtf8 (prevDate);
        QTreeWidgetItem * score = new QTreeWidgetItem (data);
        score->setTextAlignment (0, Qt::AlignRight);	// Rank.
        score->setTextAlignment (1, Qt::AlignLeft);	// Name.
        score->setTextAlignment (2, Qt::AlignRight);	// Level.
        score->setTextAlignment (3, Qt::AlignRight);	// Score.
        score->setTextAlignment (4, Qt::AlignLeft);	// Date.
        if (prevScore > 0) {			// Skip score 0 (bad file-data).
            scores->addTopLevelItem (score);	// Show score > 0.
            if (n == 0) {
                scores->setCurrentItem (score);	// Highlight the highest score.
            }
            n++;
        }

        delete prevUser;
        delete prevDate;
    }

    // Adjust the columns to fit the data.
    scores->header()->setResizeMode (0, QHeaderView::ResizeToContents);
    scores->header()->setResizeMode (1, QHeaderView::ResizeToContents);
    scores->header()->setResizeMode (2, QHeaderView::ResizeToContents);
    scores->header()->setResizeMode (3, QHeaderView::ResizeToContents);
    scores->header()->setResizeMode (4, QHeaderView::ResizeToContents);
    scores->header()->setMinimumSectionSize (-1);	// Font metrics size.

    QFrame * separator = new QFrame (hs);
    separator->setFrameStyle (QFrame::HLine + QFrame::Sunken);
    mainLayout->addWidget (separator);

    QHBoxLayout *hboxLayout1 = new QHBoxLayout();
    hboxLayout1->setSpacing (spacing);
    QSpacerItem * spacerItem = new QSpacerItem (40, 20, QSizePolicy::Expanding,
                                                QSizePolicy::Minimum);
    hboxLayout1->addItem (spacerItem);
    QPushButton *	OK = new KPushButton (KStandardGuiItem::close(), hs);
    OK->		setShortcut (Qt::Key_Return);
    OK->		setMaximumWidth (100);
    hboxLayout1->addWidget (OK);
    mainLayout->	addLayout (hboxLayout1, 5);
    int w =		(view->size().width()*4)/10;
    hs->		setMinimumSize (w, w);

    QPoint		p = view->mapToGlobal (QPoint (0,0));
    hs->		move (p.x() + 50, p.y() + 50);

    // Start up the dialog box.
    connect		(OK, SIGNAL (clicked()), hs, SLOT (accept()));
    hs->		exec();

    delete hs;
#endif
}

/******************************************************************************/
/**************************  AUTHORS' DEBUGGING AIDS **************************/
/******************************************************************************/

void KGrGame::dbgControl (const int code)
{
    if (playback) {
        levelPlayer->interruptPlayback();	// Will emit interruptDemo().
        return;
    }
    // kDebug() << "Debug code =" << code;
    if (levelPlayer && gameFrozen) {
        levelPlayer->dbgControl (code);
    }
}

bool KGrGame::initGameLists()
{
    // Initialise the lists of games (i.e. collections of levels).

    // System games are the ones distributed with KDE Games.  They cannot be
    // edited or deleted, but they can be copied, edited and saved into a user's
    // game.  Users' games and levels can be freely created, edited and deleted.

    owner = SYSTEM;				// Use system levels initially.
    if (! loadGameData (SYSTEM))		// Load list of system games.
        return (false);				// If no system games, abort.
    loadGameData (USER);			// Load user's list of games.
                                                // If none, don't worry.
    for (int i = 0; i < gameList.count(); i++) {
        dbk1 << i << gameList.at(i)->prefix << gameList.at(i)->name;
    }
    return (true);
}

bool KGrGame::loadGameData (Owner o)
{
    KGrGameIO io (view);
    QList<KGrGameData *> gList;
    QString filePath;
    IOStatus status = io.fetchGameListData
                         (o, getDirectory (o), gList, filePath);

    bool result = false;
    switch (status) {
    case NotFound:
        // If the user has not yet created a collection, don't worry.
        if (o == SYSTEM) {
            KGrMessage::information (view, i18n ("Load Game Info"),
                i18n ("Cannot find game info file '%1'.", filePath));
        }
        break;
    case NoRead:
    case NoWrite:
        KGrMessage::information (view, i18n ("Load Game Info"),
            i18n ("Cannot open file '%1' for read-only.", filePath));
        break;
    case UnexpectedEOF:
        KGrMessage::information (view, i18n ("Load Game Info"),
            i18n ("Reached end of file '%1' before finding end of game-data.",
                filePath));
        break;
    case OK:
        // Append this owner's list of games to the main list.
        gameList += gList;
        result = true;
        break;
    }

    return (result);
}

bool KGrGame::initRecordingData (const Owner fileOwner, const QString & prefix,
                                 const int levelNo)
{
    // Initialise the recording.
    delete recording;
    recording = new KGrRecording;
    recording->content.fill (0, 4000);
    recording->draws.fill   (0, 400);

    // If system game or ENDE, choose system dir, else choose user dir.
    const QString dir = ((fileOwner == SYSTEM) || (levelNo == 0)) ?
                        systemDataDir : userDataDir;
    if (playback) {
        kDebug() << "loadRecording" << dir << prefix << levelNo;
        if (! loadRecording (dir, prefix, levelNo)) {
            return false;
        }
    }
    else {
        KGrGameIO    io (view);
        KGrLevelData levelData;

        // Read the level data.
        if (! io.readLevelData (dir, prefix, levelNo, levelData)) {
            return false;
        }

        recording->dateTime    = QDateTime::currentDateTime()
                                              .toUTC()
                                              .toString (Qt::ISODate);
        kDebug() << "Recording at" << recording->dateTime;

        KGrGameData * gameData = gameList.at (gameIndex);
        recording->owner       = gameData->owner;
        recording->rules       = gameData->rules;
        recording->prefix      = gameData->prefix;
        recording->gameName    = gameData->name;

        recording->level       = levelNo;
        recording->width       = levelData.width;
        recording->height      = levelData.height;
        recording->layout      = levelData.layout;

        // If there is a name or hint, translate the UTF-8 code right now.
        recording->levelName   = (levelData.name.size() > 0) ?
                                 i18n (levelData.name.constData()) : "";
        recording->hint        = (levelData.hint.size() > 0) ?
                                 i18n (levelData.hint.constData()) : "";

        recording->lives       = lives;
        recording->score       = score;
        recording->speed       = timeScale;
        recording->controlMode = controlMode;
        recording->keyOption   = holdKeyOption;
        recording->content [0] = 0xff;
    }
    return true;
}

void KGrGame::saveRecording()
{
    QString filename = userDataDir + "rec_" + prefix + ".txt";
    QString groupName = prefix + QString::number(level).rightJustified(3,'0');
    kDebug() << filename << groupName;

    KConfig config (filename, KConfig::SimpleConfig);
    KConfigGroup configGroup = config.group (groupName);
    configGroup.writeEntry ("DateTime", recording->dateTime);
    configGroup.writeEntry ("Owner",    (int) recording->owner);
    configGroup.writeEntry ("Rules",    (int) recording->rules);
    configGroup.writeEntry ("Prefix",   recording->prefix);
    configGroup.writeEntry ("GameName", recording->gameName);
    configGroup.writeEntry ("Level",    recording->level);
    configGroup.writeEntry ("Width",    recording->width);
    configGroup.writeEntry ("Height",   recording->height);
    configGroup.writeEntry ("Layout",   recording->layout);
    configGroup.writeEntry ("Name",     recording->levelName);
    configGroup.writeEntry ("Hint",     recording->hint);
    configGroup.writeEntry ("Lives",    (int) recording->lives);
    configGroup.writeEntry ("Score",    (int) recording->score);
    configGroup.writeEntry ("Speed",    (int) recording->speed);
    configGroup.writeEntry ("Mode",     (int) recording->controlMode);
    configGroup.writeEntry ("KeyOption", (int)recording->keyOption);

    QList<int> bytes;
    int ch = 0;
    int n  = recording->content.size();
    for (int i = 0; i < n; i++) {
        ch = (uchar)(recording->content.at(i));
        bytes.append (ch);
        if (ch == 0)
            break;
    }
    configGroup.writeEntry ("Content", bytes);

    bytes.clear();
    ch = 0;
    n = recording->draws.size();
    for (int i = 0; i < n; i++) {
        ch = (uchar)(recording->draws.at(i));
        bytes.append (ch);
        if (ch == 0)
            break;
    }
    configGroup.writeEntry ("Draws", bytes);

    configGroup.sync();			// Ensure that the entry goes to disk.

    // Save the game and level, for use in the REPLAY_LAST action.
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    gameGroup.writeEntry ("LastGamePrefix", prefix);
    gameGroup.writeEntry ("LastLevel",      level);
    gameGroup.sync();			// Ensure that the entry goes to disk.
}

bool KGrGame::loadRecording (const QString & dir, const QString & prefix,
                                                  const int levelNo)
{
    kDebug() << prefix << levelNo;
    QString filename  = dir + "rec_" + prefix + ".txt";
    QString groupName = prefix + QString::number(levelNo).rightJustified(3,'0');
    kDebug() << filename << groupName;

    KConfig config (filename, KConfig::SimpleConfig);
    if (! config.hasGroup (groupName)) {
        kDebug() << "Group" << groupName << "NOT FOUND";
        return false;
    }

    KConfigGroup configGroup    = config.group (groupName);
    QByteArray blank ("");
    recording->dateTime         = configGroup.readEntry ("DateTime", "");
    recording->owner            = (Owner)(configGroup.readEntry
                                                        ("Owner", (int)(USER)));
    recording->rules            = configGroup.readEntry ("Rules", (int)('T'));
    recording->prefix           = configGroup.readEntry ("Prefix", "");
    recording->gameName         = configGroup.readEntry ("GameName", blank);
    recording->level            = configGroup.readEntry ("Level",  1);
    recording->width            = configGroup.readEntry ("Width",  FIELDWIDTH);
    recording->height           = configGroup.readEntry ("Height", FIELDHEIGHT);
    recording->layout           = configGroup.readEntry ("Layout", blank);
    recording->levelName        = configGroup.readEntry ("Name",   blank);
    recording->hint             = configGroup.readEntry ("Hint",   blank);
    recording->lives            = configGroup.readEntry ("Lives",  5);
    recording->score            = configGroup.readEntry ("Score",  0);
    recording->speed            = configGroup.readEntry ("Speed",  10);
    recording->controlMode      = configGroup.readEntry ("Mode",   (int)MOUSE);
    recording->keyOption        = configGroup.readEntry ("KeyOption",
                                                                (int)CLICK_KEY);

    // If demoType is DEMO or SOLVE, get the TRANSLATED gameName, levelName and
    // hint from current data (other recordings have been translated already).
    if ((demoType == DEMO) || (demoType == SOLVE)) {
        int index = -1;
        for (int i = 0; i < gameList.count(); i++) {	// Find the game.
            if (gameList.at (i)->prefix == recording->prefix) {
                index = i;
                break;
            }
        }
        if (index >= 0) {
            // Get the current translation of the name of the game.
            recording->gameName = gameList.at (index)->name;

            // Read the current level data.
            KGrGameIO    io (view);
            KGrLevelData levelData;

            if (io.readLevelData (dir, recording->prefix, recording->level,
                                  levelData)) {
                // If there is a level name or hint, translate it.
                recording->levelName   = (levelData.name.size() > 0) ?
                                         i18n (levelData.name.constData()) : "";
                recording->hint        = (levelData.hint.size() > 0) ?
                                         i18n (levelData.hint.constData()) : "";
            }
        }
    }

    QList<int> bytes = configGroup.readEntry ("Content", QList<int>());
    int n  = bytes.count();
    recording->content.fill (0, n + 1);
    for (int i = 0; i < n; i++) {
        recording->content [i] = bytes.at (i);
    }

    bytes.clear();
    bytes = configGroup.readEntry ("Draws", QList<int>());
    n  = bytes.count();
    recording->draws.fill (0, n + 1);
    for (int i = 0; i < n; i++) {
        recording->draws [i] = bytes.at (i);
    }

    // Set up and display the starting score and lives.
    lives = recording->lives;
    emit showLives (lives);
    score = recording->score;
    emit showScore (score);
    return true;
}

void KGrGame::loadSounds()
{
#ifdef KGAUDIO_BACKEND_OPENAL
        const qreal volumes [NumSounds] = {0.6, 0.3, 0.3, 0.6, 0.6, 1.8, 1.0, 1.0, 1.0, 1.0};
        effects = new KGrSounds();
        effects->setParent (this);        // Delete at end of KGrGame.

        fx[GoldSound]      = effects->loadSound (KStandardDirs::locate ("appdata",
                             "themes/default/gold.ogg"));
        fx[StepSound]      = effects->loadSound (KStandardDirs::locate ("appdata",
                             "themes/default/step.wav"));
        fx[ClimbSound]     = effects->loadSound (KStandardDirs::locate ("appdata",
                             "themes/default/climb.wav"));
        fx[FallSound]      = effects->loadSound (KStandardDirs::locate ("appdata",
                             "themes/default/falling.ogg"));
        fx[DigSound]       = effects->loadSound (KStandardDirs::locate ("appdata",
                             "themes/default/dig.ogg"));
        fx[LadderSound]    = effects->loadSound (KStandardDirs::locate ("appdata",
                             "themes/default/ladder.ogg"));
        fx[CompletedSound] = effects->loadSound (KStandardDirs::locate ("appdata",
                             "themes/default/completed.ogg"));
        fx[DeathSound]     = effects->loadSound (KStandardDirs::locate ("appdata",
                             "themes/default/death.ogg"));
        fx[GameOverSound]  = effects->loadSound (KStandardDirs::locate ("appdata",
                             "themes/default/gameover.ogg"));
        fx[VictorySound]   = effects->loadSound (KStandardDirs::locate ("appdata",
                             "themes/default/victory.ogg"));

        // Gold and dig sounds are timed and are allowed to play for at least one
        // second, so that rapid sequences of those sounds are heard as overlapping.
        effects->setTimedSound (fx[GoldSound]);
        effects->setTimedSound (fx[DigSound]);

        // Adjust the relative volumes of sounds to improve the overall balance.
        for (int i = 0; i < NumSounds; i++) {
            effects->setVolume (fx [i], volumes [i]);
        }
#endif
}

/******************************************************************************/
/**********************    MESSAGE BOX WITH FREEZE    *************************/
/******************************************************************************/

void KGrGame::myMessage (QWidget * parent, const QString &title, const QString &contents)
{
    // Halt the game while the message is displayed, if not already halted.
    freeze (ProgramPause, true);

    KGrMessage::information (parent, title, contents);

    // Unfreeze the game, but only if it was previously unfrozen.
    freeze (ProgramPause, false);
}

#include "kgrgame.moc"
// vi: set sw=4 :
