/***************************************************************************
 *    Copyright 2003 Marco Krüger <grisuji@gmx.de>                         *
 *    Copyright 2003 Ian Wadham <ianw2@optusnet.com.au>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kgrgame.h"

#include "kgrglobals.h"
#include "kgrcanvas.h"
#include "kgrselector.h"
#include "kgrsoundbank.h"
#include "kgreditor.h"
#include "kgrlevelplayer.h"
#include "kgrdialog.h"

#include <iostream>
#include <stdlib.h>
// #include <ctype.h>
#include <time.h>

#include <kpushbutton.h>
#include <KStandardGuiItem>
#include <KStandardDirs>
#include <KApplication>
#include <kdebug.h>

// Do NOT change KGoldrunner over to KScoreDialog until we have found a way
// to preserve high-score data pre-existing from the KGr high-score methods.
// #define USE_KSCOREDIALOG 1 // IDW - 11 Aug 07.

#ifdef USE_KSCOREDIALOG
#include <KScoreDialog>
#include <QDate>
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

#include "kgrrulebook.h"

/******************************************************************************/
/***********************    KGOLDRUNNER GAME CLASS    *************************/
/******************************************************************************/

KGrGame::KGrGame (KGrCanvas * theView, 
                  const QString & theSystemDir, const QString & theUserDir)
        : 
	QObject (theView),	// Make sure game is destroyed when view closes.
        levelPlayer (0),
        view (theView),
        systemDataDir (theSystemDir),
        userDataDir (theUserDir),
        level (0),
        fx (NumSounds),
        editor (0)
{
    newLevel = true;			// Next level will be a new one.
    loading  = true;			// Stop input until it is loaded.
    settings (NORMAL_SPEED);

    gameFrozen = true;
    modalFreeze = false;
    messageFreeze = false;

#ifdef ENABLE_SOUND_SUPPORT
    effects = new KGrSoundBank(8);
    effects->setParent (this);		// Delete at end of KGrGame.

    fx[GoldSound] = effects->loadSound (KStandardDirs::locate ("appdata", "themes/default/gold.wav"));
    fx[StepSound] = effects->loadSound (KStandardDirs::locate ("appdata", "themes/default/step.wav"));
    fx[ClimbSound] = effects->loadSound (KStandardDirs::locate ("appdata", "themes/default/climb.wav"));
    fx[FallSound] = effects->loadSound (KStandardDirs::locate ("appdata", "themes/default/falling.wav"));
    fx[DigSound] = effects->loadSound (KStandardDirs::locate ("appdata", "themes/default/dig.wav"));
    fx[LadderSound] = effects->loadSound (KStandardDirs::locate ("appdata", "themes/default/ladder.wav"));
    fx[CompletedSound] = effects->loadSound (KStandardDirs::locate ("appdata", "themes/default/completed.wav"));
    fx[DeathSound] = effects->loadSound (KStandardDirs::locate ("appdata", "themes/default/death.wav"));
    fx[GameOverSound] = effects->loadSound (KStandardDirs::locate ("appdata", "themes/default/gameover.wav"));

    // REPLACE - 9/1/09  Hero's sound connections.
    // connect(hero, SIGNAL (stepDone (bool)), this, SLOT (heroStep (bool)));
    // connect(hero, SIGNAL (falling (bool)), this, SLOT (heroFalls (bool)));
    // connect(hero, SIGNAL (digs()), this, SLOT (heroDigs()));

#endif

    dyingTimer = new QTimer (this);
    connect (dyingTimer, SIGNAL (timeout()),  SLOT (finalBreath()));

    srand (time (0)); 			// Initialise random number generator.
}

KGrGame::~KGrGame()
{
    while (! gameList.isEmpty())
        delete gameList.takeFirst();
}

// Flags to control author's debugging aids.
bool KGrGame::bugFix  = false;	// Start game with dynamic bug-fix OFF.
bool KGrGame::logging = false;	// Start game with dynamic logging OFF.

void KGrGame::gameActions (int action)
{
    switch (action) {
    case NEW:
	kDebug() << "NEW signal:" << action;
        startAnyLevel();
        break;
    case LOAD:
	kDebug() << "LOAD signal:" << action;
        loadGame();
        break;
    case SAVE_GAME:
	kDebug() << "SAVE_GAME signal:" << action;
        saveGame();
        break;
    case PAUSE:
	kDebug() << "PAUSE signal:" << action;
        if (! gameFrozen) {
            freeze();
        }
        else {
            unfreeze();
        }
        break;
    case HIGH_SCORE:
	kDebug() << "HIGH_SCORE signal:" << action;
        showHighScores();
        break;
    case HINT:
	kDebug() << "HINT signal:" << action;
	showHint();
	break;
    case KILL_HERO:
	kDebug() << "KILL_HERO signal:" << action;
        endLevel (DEAD);
	break;
    default:
	break;
    }
}

void KGrGame::editActions (int action)
{
    if (! editor) {
        if (action == SAVE_EDITS) {
            KGrMessage::information (view, i18n ("Save Level"),
                i18n ("Inappropriate action: you are not editing a level."));
            return;
        }

        // If there is a level being played, kill it, with no win/lose result.
        if (levelPlayer) {
            endLevel (NORMAL);
            view->deleteAllSprites();
        }

        // If there is no editor running, start one.
        emit setEditMenu (true);	// Enable edit menu items and toolbar.
        emit defaultEditObj();		// Set default edit-toolbar button.
        editor = new KGrEditor (view, systemDataDir, userDataDir, gameList);
        emit showLives (0);
        emit showScore (0);
        // Pass the editor's showLevel signal on to the KGoldrunner GUI object.
        connect (editor, SIGNAL (showLevel (int)),
                 this,   SIGNAL (showLevel (int)));
    }

    switch (action) {
    case CREATE_LEVEL:
	kDebug() << "CREATE_LEVEL signal:" << action;
	editor->createLevel (gameIndex);
	break;
    case EDIT_ANY:
	kDebug() << "EDIT_ANY signal:" << action;
	editor->updateLevel (gameIndex, level);
	break;
    case SAVE_EDITS:
	kDebug() << "SAVE_EDITS signal:" << action;
	editor->saveLevelFile ();
	break;
    case MOVE_LEVEL:
	kDebug() << "MOVE_LEVEL signal:" << action;
	editor->moveLevelFile (gameIndex, level);
	break;
    case DELETE_LEVEL:
	kDebug() << "DELETE_LEVEL signal:" << action;
	editor->deleteLevelFile (gameIndex, level);
	break;
    case CREATE_GAME:
	kDebug() << "CREATE_GAME signal:" << action;
	editor->editGame (-1);
	break;
    case EDIT_GAME:
	kDebug() << "EDIT_GAME signal:" << action;
	editor->editGame (gameIndex);
	break;
    default:
	break;
    }

    int game = gameIndex, lev = level;
    editor->getGameAndLevel (game, lev);
    kDebug() << "Game used" << gameList.at(gameIndex)->name << "level" << level;
    kDebug() << "Editor used" << gameList.at(game)->name << "level" << lev;
    if (((game != gameIndex) || (lev != level)) && (lev != 0)) {
        gameIndex = game;
        level     = lev;
        gameData  = gameList.at (gameIndex);
        kDebug() << "Saving to KConfigGroup";

        KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
        gameGroup.writeEntry ("GamePrefix", gameData->prefix);
        gameGroup.writeEntry ("Level_" + gameData->prefix, level);
        gameGroup.sync();		// Ensure that the entry goes to disk.
    }
}

void KGrGame::editToolbarActions (int action)
{
    // If game-editor is inactive or action-code is not recognised, do nothing.
    if (editor) {
        switch (action) {
        case EDIT_HINT:
            // Edit the level-name or hint.
	    kDebug() << "EDIT_HINT signal:" << action;
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

void KGrGame::settings (int action)
{
    switch (action) {
    case PLAY_SOUNDS:
	kDebug() << "PLAY_SOUNDS signal:" << action;
        toggleSoundsOnOff();
        break;
    case MOUSE:
    case KEYBOARD:
    case LAPTOP:
	kDebug() << "Control-mode signal:" << action;
        setControlMode (action);
        break;
    case NORMAL_SPEED:
    case BEGINNER_SPEED:
    case CHAMPION_SPEED:
    case INC_SPEED:
    case DEC_SPEED:
	kDebug() << "Speed-setting signal:" << action;
        setTimeScale (action);
        break;
    default:
        break;
    }
}

void KGrGame::setInitialTheme (const QString & themeFilepath)
{
    initialThemeFilepath = themeFilepath;
}

void KGrGame::initGame()
{
    kDebug() << "Entered, draw the initial graphics now ...";

    // Get the most recent collection and level that was played by this user.
    // If he/she has never played before, set it to Tutorial, level 1.
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    QString prevGamePrefix = gameGroup.readEntry ("GamePrefix", "tute");
    int prevLevel = gameGroup.readEntry ("Level_" + prevGamePrefix, 1);

    kDebug()<< "Config() Game and Level" << prevGamePrefix << prevLevel;

    // Use that game and level, if it is among the current games.
    int n = 0;
    gameIndex = -1;
    foreach (gameData, gameList) {
        if (gameData->prefix == prevGamePrefix) {
            gameIndex = n;
            level = prevLevel;
            break;
        }
        n++;
    }

    // If not found, use the first game in the list and level 1.
    if (gameIndex < 0) {
        gameIndex = 0;
        level = 1;
    }

    gameData = gameList.at (gameIndex);
    kDebug() << "Owner" << gameData->owner << gameData->name << level;

    kDebug() << "Calling the first view->changeTheme() ...";
    view->changeTheme (initialThemeFilepath);

    newGame (level, gameIndex);
}

/******************************************************************************/
/**********************  QUICK-START DIALOG AND SLOTS  ************************/
/******************************************************************************/

void KGrGame::quickStartDialog()
{
    // Make sure the game will not start during the Quick Start dialog.
    freeze();

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
    showTutorialMessages (level);	// Level has already been loaded.
    kDebug() << "gameFrozen?" << gameFrozen;
    unfreeze();
}

void KGrGame::quickStartNewGame()
{
    qs->accept();
    unfreeze();
    startAnyLevel();
}

void KGrGame::quickStartUseMenu()
{
    qs->accept();
    myMessage (view, i18n ("Game Paused"),
            i18n ("The game is halted. You will need to press the Pause key "
                 "(default P or Esc) when you are ready to play."));
}

void KGrGame::quickStartQuit()
{
    emit quitGame();
}

/******************************************************************************/
/*************************  GAME SELECTION PROCEDURES  ************************/
/******************************************************************************/

void KGrGame::startLevelOne()
{
    startLevel (SL_START, 1);
}

void KGrGame::startAnyLevel()
{
    startLevel (SL_ANY, level);
}

void KGrGame::startNextLevel()
{
    startLevel (SL_ANY, level + 1);
}

void KGrGame::startLevel (int startingAt, int requestedLevel)
{
    kDebug() << "Game" << gameList.at(gameIndex)->name << "level" << requestedLevel;
    if (! saveOK()) {		// Check for unsaved edits.
        return;
    }
    kDebug() << "Game" << gameList.at(gameIndex)->name << "level" << requestedLevel;
    // Use dialog box to select game and level: startingAt = ID_FIRST or ID_ANY.
    // int selectedLevel = selectLevel (startingAt, requestedLevel);
    int selectedLevel = requestedLevel;

    // Halt the game during the dialog.
    modalFreeze = false;
    if (! gameFrozen) {
        modalFreeze = true;
        freeze();
    }

    // Run the game and level selection dialog.
    KGrSLDialog * sl = new KGrSLDialog (startingAt, requestedLevel, gameIndex,
                                        gameList, systemDataDir, userDataDir,
                                        view);
    bool selected = sl->selectLevel (selectedGame, selectedLevel);
    delete sl;

    // Unfreeze the game, but only if it was previously unfrozen.
    if (modalFreeze) {
        unfreeze();
        modalFreeze = false;
    }

    if (selected) {		// If OK, start the selected game and level.
        newGame (selectedLevel, selectedGame);
        showTutorialMessages (level);
    }
    else {
        // TODO - At startup, choose New Game, but don't select a game.
        // TODO - Then the user's previous game, from Config, becomes FROZEN.
        // TODO - No amount of hitting Esc or P can UNFREEZE it ...
        level = requestedLevel; // IDW = 0;
    }
}

/******************************************************************************/
/************************  MAIN GAME EVENT PROCEDURES  ************************/
/******************************************************************************/

void KGrGame::incScore (int n)
{
#ifdef ENABLE_SOUND_SUPPORT
    // I don't think this is the right place, but it's just for testing...
    switch (n) {
    case 250: 
	effects->play (fx[GoldSound]);
	break;
    default:
	break;
    }
#endif
    score = score + n;		// SCORING: trap enemy 75, kill enemy 75,
    emit showScore (score);	// collect gold 250, complete the level 1500.
}

void KGrGame::endLevel (const int result)
{
    if (levelPlayer) {
        // Delete the level-player, hero, enemies, grid, rule-book, etc.
        // Delete sprites in KGrCanvas later, so they stay visible for a while.
        delete levelPlayer;
        levelPlayer = 0;
    }
    else {
        return;			// Not playing a level.
    }

    if (result == WON_LEVEL) {
        levelCompleted();
    }
    else if (result == DEAD) {
        herosDead();
    }
}

void KGrGame::herosDead()
{
    if ((level < 1) || (lives <= 0)) {
        return;			// Game over: we are in the "ENDE" screen.
    }

    // Lose a life.
    if (--lives > 0) {
#ifdef ENABLE_SOUND_SUPPORT
	effects->play (fx[DeathSound]);
#endif
        // Still some life left, so PAUSE and then re-start the level.
        emit showLives (lives);
        gameFrozen = true;	// Freeze the animation and let
        dyingTimer->setSingleShot (true);
        dyingTimer->start (1500);	// the player see what happened.
        view->fadeOut();
    }
    else {
        // Game over.
#ifdef ENABLE_SOUND_SUPPORT
	effects->play (fx[GameOverSound]);
#endif
        emit showLives (lives);
        freeze();
        QString gameOver = "<NOBR><B>" + i18n ("GAME OVER !!!") + "</B></NOBR>";
        KGrMessage::information (view, gameData->name, gameOver);
        checkHighScore();	// Check if there is a high score for this game.

        // Offer the player a chance to start this level again with 5 new lives.
        switch (KGrMessage::warning (view, i18n ("Retry Level?"),
                            i18n ("Would you like to try this level again?"),
                            i18n ("&Try Again"), i18n ("&Finish"))) {
        case 0:
            unfreeze();			// Offer accepted.
            newGame (level, gameIndex);
            showTutorialMessages (level);
            return;
            break;
        case 1:
            break;			// Offer rejected.
        }

        // Game completely over: display the "ENDE" screen.
        // TODO - The "ENDE" screen is displayed but it is frozen.
        // TODO - If we restart the game on the "ENDE" screen then use the New
        //        Game dialog, the new level comes up but animation is haywire.
        unfreeze();		//    ... NOW we can unfreeze.
        newLevel = true;
        level = 0;
        loadLevel (level);	// Display the "ENDE" screen.
        newLevel = false;
    }
}

void KGrGame::finalBreath()
{
    // Fix bug 95202:	Avoid re-starting if the player selected
    //			edit mode before the 1.5 seconds were up.
    if (! editor) {
        loadLevel (level);
        if (levelPlayer) {
            levelPlayer->prepareToPlay();
        }
    }
    gameFrozen = false;	// Unfreeze the game, but don't move yet.
}

void KGrGame::showHiddenLadders()
{
    // TODO - Move this line to KGrLevelPlayer.
    effects->play (fx[LadderSound]);
}
        
void KGrGame::levelCompleted()
{
#ifdef ENABLE_SOUND_SUPPORT
    effects->play (fx[CompletedSound]);
#endif
    connect (view, SIGNAL (fadeFinished()), this, SLOT (goUpOneLevel()));
    view->fadeOut();
}

void KGrGame::goUpOneLevel()
{
    disconnect (view, SIGNAL (fadeFinished()), this, SLOT (goUpOneLevel()));
    lives++;			// Level completed: gain another life.
    emit showLives (lives);
    incScore (1500);

    if (level >= gameData->nLevels) {
        freeze();
        KGrMessage::information (view, gameData->name,
            i18n ("<b>CONGRATULATIONS !!!!</b>"
            "<p>You have conquered the last level in the "
            "<b>\"%1\"</b> game !!</p>", gameData->name.constData()));
        checkHighScore();	// Check if there is a high score for this game.

        unfreeze();
        level = 0;		// Game completed: display the "ENDE" screen.
    }
    else {
        level++;		// Go up one level.
        emit showLevel (level);
    }

    newLevel = true;
    loadLevel (level);
    showTutorialMessages (level);
    newLevel = false;
}

void KGrGame::setControlMode (const int mode)
{
    controlMode = mode;
    if (levelPlayer) {
        levelPlayer->setControlMode (mode);
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

    if (levelPlayer) {
        kDebug() << "setTimeScale" << (timeScale * 0.1);
        levelPlayer->setTimeScale (timeScale * 0.1);
    }
}

bool KGrGame::inMouseMode()
{
    return (controlMode == MOUSE);
}

bool KGrGame::inEditMode()
{
    return (editor != 0);	// Return true if the game-editor is active.
}

bool KGrGame::isLoading()
{
    return (loading);		// Return true if a level is being loaded.
}

void KGrGame::toggleSoundsOnOff()
{
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    bool soundOnOff = gameGroup.readEntry ("Sound", false);
    soundOnOff = (! soundOnOff);
    gameGroup.writeEntry ("Sound", soundOnOff);
#ifdef ENABLE_SOUND_SUPPORT
    effects->setMuted (! soundOnOff);
#endif
}

void KGrGame::freeze()
{
    if ((! modalFreeze) && (! messageFreeze)) {
        emit gameFreeze (true);	// Do visual feedback in the GUI.
    }
    gameFrozen = true;	// Halt the game, by blocking all timer events.
    if (levelPlayer) {
        levelPlayer->pause (true);
    }
}

void KGrGame::unfreeze()
{
    if ((! modalFreeze) && (! messageFreeze)) {
        emit gameFreeze (false);// Do visual feedback in the GUI.
    }
    gameFrozen = false;		// Restart the game after the next tick().
    if (levelPlayer) {
        levelPlayer->pause (false);
    }
}

void KGrGame::setMessageFreeze (bool on_off)
{
    if (on_off) {		// Freeze the game action during a message.
        messageFreeze = false;
        if (! gameFrozen) {
            messageFreeze = true;
            freeze();
        }
    }
    else {			// Unfreeze the game action after a message.
        if (messageFreeze) {
            unfreeze();
            messageFreeze = false;
        }
    }
}

void KGrGame::newGame (const int lev, const int newGameIndex)
{
    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = true;		// "loadLevel (level)" will reset it.

    view->goToBlack();
    if (editor) {
        emit setEditMenu (false);	// Disable edit menu items and toolbar.

        delete editor;
        editor = 0;
    }

    newLevel = true;

    level     = lev;
    gameIndex = newGameIndex;
    gameData  = gameList.at (gameIndex);
    owner     = gameData->owner;

    lives = 5;				// Start with 5 lives.
    score = 0;
    startScore = 0;

    emit showLives (lives);
    emit showScore (score);
    emit showLevel (level);

    newLevel = true;;
    loadLevel (level);
    newLevel = false;
}

void KGrGame::startTutorial()
{
    if (! saveOK()) {		// Check for unsaved edits.
        return;
    }

    int i, index;
    int imax = gameList.count();
    bool found = false;

    index = 0;
    for (i = 0; i < imax; i++) {
        index = i;			// Index within owner.
        if (gameList.at (i)->prefix == "tute") {
            found = true;
            break;
        }
    }
    if (found) {
        // Start the tutorial.
        gameData = gameList.at (index);
        owner = gameData->owner;
        gameIndex = index;
        level = 1;
        newGame (level, gameIndex);
        showTutorialMessages (level);
    }
    else {
        // TODO - This message should refer to the "game_tute.txt" file.
        KGrMessage::information (view, i18n ("Start Tutorial"),
            i18n ("Cannot find the tutorial game (file-prefix '%1') in "
            "the '%2' files.",
            QString ("tute"), QString ("games.dat")));
    }
}

void KGrGame::showHint()
{
    // Put out a hint for this level.
    QString caption = i18n ("Hint");

    if (levelHint.length() > 0)
        myMessage (view, caption, levelHint);
    else
        myMessage (view, caption,
                        i18n ("Sorry, there is no hint for this level."));
}

int KGrGame::loadLevel (int levelNo)
{
    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = true;

    // Read the level data.
    KGrLevelData levelData;
    if (! readLevelData (levelNo, levelData)) {
        loading = false;
        return 0;
    }

    // Clean up any sprites remaining from a previous level.  This is done late,
    // so that the end-state of the sprites will be visible for a short while.
    view->deleteAllSprites();

    view->setLevel (levelNo);		// Switch and render background if reqd.
    view->fadeIn();			// Then run the fade-in animation.
    startScore = score;			// The score we will save, if asked.

    kDebug() << "Prefix" << gameData->prefix << "index" << gameIndex
             << "of" << gameList.count();

    levelPlayer = new KGrLevelPlayer (this);

    char rulesCode = gameList.at(gameIndex)->rules;
    levelPlayer->init (view, controlMode, rulesCode, &levelData);
    kDebug() << "setTimeScale" << (timeScale * 0.1);
    levelPlayer->setTimeScale (timeScale * 0.1);

    // If there is a name, translate the UTF-8 coded QByteArray right now.
    levelName = (levelData.name.size() > 0) ?
                      i18n ((const char *) levelData.name) : "";

    // Indicate on the menus whether there is a hint for this level.
    int len = levelData.hint.length();
    emit hintAvailable (len > 0);

    // If there is a hint, translate it right now.
    levelHint = (len > 0) ? i18n ((const char *) levelData.hint) : "";

    // TODO - Handle editor connections inside the editor.
    // Disconnect edit-mode slots from signals from "view".
    // disconnect (view, SIGNAL (mouseClick (int)), 0, 0);
    // disconnect (view, SIGNAL (mouseLetGo (int)), 0, 0);

    // Re-draw the playfield frame, level title and figures.
    view->setTitle (getTitle());

    // If we are starting a new level, save it in the player's config file.
    if (newLevel) {
        KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
        gameGroup.writeEntry ("GamePrefix", gameData->prefix);
        gameGroup.writeEntry ("Level_" + gameData->prefix, level);
        gameGroup.sync();		// Ensure that the entry goes to disk.
    }

    // Use a queued connection here, to ensure that levelPlayer has finished
    // executing when control goes to the endLevel (const int heroStatus) slot.
    connect (levelPlayer, SIGNAL (endLevel (const int)),
             this,        SLOT   (endLevel (const int)), Qt::QueuedConnection);

    // Re-enable player input.
    loading = false;

    return 1;
}

void KGrGame::showTutorialMessages (int levelNo)
{
    // Halt the game during message displays and mouse pointer moves.
    setMessageFreeze (true);

    // Check if this is a tutorial collection and not on the "ENDE" screen.
    if ((gameData->prefix.left (4) == "tute") && (levelNo != 0)) {

        // At the start of a tutorial, put out an introduction.
        if (levelNo == 1) {
            KGrMessage::information (view, gameData->name,
                        i18n (gameData->about.constData()));
        }
        // Put out an explanation of this level.
        KGrMessage::information (view, getTitle(), levelHint);
    }

    if (levelPlayer) {
        levelPlayer->prepareToPlay();
    }
    setMessageFreeze (false);	// Let the level begin.
}

bool KGrGame::readLevelData (int levelNo, KGrLevelData & d)
{
    KGrGameIO io;
    // If system game or ENDE screen, choose system dir, else choose user dir.
    const QString dir = ((owner == SYSTEM) || (levelNo == 0)) ?
                                        systemDataDir : userDataDir;
    kDebug() << "Owner" << owner << "dir" << dir << "Level" << gameData->prefix << levelNo;
    QString filePath;
    IOStatus stat = io.fetchLevelData (dir, gameData->prefix, levelNo,
                                       d, filePath);

    switch (stat) {
    case NotFound:
        KGrMessage::information (view, i18n ("Read Level Data"),
            i18n ("Cannot find file '%1'.", filePath));
        break;
    case NoRead:
    case NoWrite:
        KGrMessage::information (view, i18n ("Read Level Data"),
            i18n ("Cannot open file '%1' for read-only.", filePath));
        break;
    case UnexpectedEOF:
        KGrMessage::information (view, i18n ("Read Level Data"),
            i18n ("Reached end of file '%1' without finding level data.",
            filePath));
        break;
    case OK:
        break;
    }

    return (stat == OK);
}

    // TODO - Connect these somewhere else in the code, e.g. in KGrLevelPlayer.
    // connect (enemy, SIGNAL (trapped (int)), SLOT (incScore (int)));
    // connect (enemy, SIGNAL (killed (int)),  SLOT (incScore (int)));

QString KGrGame::getDirectory (Owner o)
{
    return ((o == SYSTEM) ? systemDataDir : userDataDir);
}

QString KGrGame::getTitle()
{
    if (level == 0) {
        // Generate a special title for end of game.
        return (i18n ("T H E   E N D"));
    }

    // Set title string to "Game-name - NNN" or "Game-name - NNN - Level-name".
    QString levelNumber;
    levelNumber.setNum (level);
    levelNumber = levelNumber.rightJustified (3,'0');

    // TODO - Make sure gameData->name.constData() results in Unicode display
    //        and not some weird UTF8 character stuff.
    QString levelTitle = (levelName.length() <= 0)
                    ?
                    i18nc ("Game name - level number.",
                           "%1 - %2",
                           gameData->name.constData(), levelNumber)
                    :
                    i18nc ("Game name - level number - level name.",
                           "%1 - %2 - %3",
                           gameData->name.constData(), levelNumber, levelName);
    return (levelTitle);
}

void KGrGame::kbControl (int dirn)
{
    kDebug() << "Keystroke setting direction" << dirn;
    if (editor) return;

    // Using keyboard control can automatically disable mouse control.
    if ((controlMode == MOUSE) ||
        ((controlMode == LAPTOP) && (dirn != DIG_RIGHT) && (dirn != DIG_LEFT)))
        {
        // Halt the game while a message is displayed.
        setMessageFreeze (true);

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
            controlMode = KEYBOARD;
            // TODO - Connect these signals in kgoldrunner.cpp somewhere.
            emit setToggle ("keyboard_mode", true);	// Adjust Settings menu.
            break;
        case KMessageBox::No: 
            break;
        }

        // Unfreeze the game, but only if it was previously unfrozen.
        setMessageFreeze (false);

        if (controlMode != KEYBOARD) {
            return;                    		// Stay in Mouse or Laptop Mode.
        }
    }

    // Accept keystroke to set next direction, even when the game is frozen.
    // TODO - But what if it is a DIG key?
    if (levelPlayer) {
        levelPlayer->setDirectionByKey ((Direction) dirn);
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
    // TODO - Smart save ...
    // OBSOLESCENT - 7/1/09
    // if (hero->started) {myMessage (view, i18n ("Save Game"),
        // i18n ("Please note: for reasons of simplicity, your saved game "
        // "position and score will be as they were at the start of this "
        // "level, not as they are now."));
    // }

    QDate today = QDate::currentDate();
    QTime now =   QTime::currentTime();
    QString saved;
    QString day;
    day = today.shortDayName (today.dayOfWeek());
    saved = saved.sprintf
                ("%-6s %03d %03ld %7ld    %s %04d-%02d-%02d %02d:%02d\n",
                gameData->prefix.myStr(), level, lives, startScore,
                day.myStr(),
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

    if (KGrGameIO::safeRename (userDataDir+"savegame.tmp",
                               userDataDir+"savegame.dat")) {
        KGrMessage::information (view, i18n ("Save Game"),
                                i18n ("Your game has been saved."));
    }
    else {
        KGrMessage::information (view, i18n ("Save Game"),
                                i18n ("Error: Failed to save your game."));
    }
}

void KGrGame::loadGame()		// Re-load game, score and level.
{
    if (! saveOK()) {			// Check for unsaved edits.
        return;
    }

    QFile savedGames (userDataDir + "savegame.dat");
    if (! savedGames.exists()) {
        // Use myMessage() because it stops the game while the message appears.
        myMessage (view, i18n ("Load Game"),
                        i18n ("Sorry, there are no saved games."));
        return;
    }

    if (! savedGames.open (QIODevice::ReadOnly)) {
        KGrMessage::information (view, i18n ("Load Game"),
            i18n ("Cannot open file '%1' for read-only.",
             userDataDir + "savegame.dat"));
        return;
    }

    // Halt the game during the loadGame() dialog.
    modalFreeze = false;
    if (!gameFrozen) {
        modalFreeze = true;
        freeze();
    }

    QString s;

    KGrLGDialog * lg = new KGrLGDialog (&savedGames, gameList, view);

    if (lg->exec() == QDialog::Accepted) {
        s = lg->getCurrentText();
    }

    bool found = false;
    QString pr;
    int  lev;
    int i;
    int imax = gameList.count();

    if (! s.isNull()) {
        pr = s.mid (21, 7);			// Get the collection prefix.
        pr = pr.left (pr.indexOf (" ", 0, Qt::CaseInsensitive));

        for (i = 0; i < imax; i++) {		// Find the collection.
            if (gameList.at (i)->prefix == pr) {
                gameData = gameList.at (i);
                gameIndex  = i;
                owner = gameList.at (i)->owner;
                found = true;
                break;
            }
        }
        if (found) {
            // Set the rules for the selected game.
            lev   = s.mid (28, 3).toInt();
            newGame (lev, gameIndex);		// Re-start the selected game.
            showTutorialMessages (level);
            lives = s.mid (32, 3).toLong();	// Update the lives.
            emit showLives (lives);
            score = s.mid (36, 7).toLong();	// Update the score.
            emit showScore (score);
        }
        else {
            KGrMessage::information (view, i18n ("Load Game"),
                i18n ("Cannot find the game with prefix '%1'.", pr));
        }
    }

    // Unfreeze the game, but only if it was previously unfrozen.
    if (modalFreeze) {
        unfreeze();
        modalFreeze = false;
    }

    delete lg;
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
    if (gameData->prefix.left (4) == "tute")
        return;

    if (score <= 0)
        return;

#ifdef USE_KSCOREDIALOG
    KScoreDialog scoreDialog (
            KScoreDialog::Name | KScoreDialog::Level | 
            KScoreDialog::Date | KScoreDialog::Score, 
            view);
    scoreDialog.setConfigGroup (gameData->prefix);
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
    QFile high1 (userDataDir + "hi_" + gameData->prefix + ".dat");
    QDataStream s1;

    if (! high1.exists()) {
        high1.setFileName (systemDataDir + "hi_" + gameData->prefix + ".dat");
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

    QFile high2 (userDataDir + "hi_" + gameData->prefix + ".tmp");
    QDataStream s2;

    if (! high2.open (QIODevice::WriteOnly)) {
        KGrMessage::information (view, i18n ("Check for High Score"),
                i18n ("Cannot open file '%1' for output.",
                 userDataDir + "hi_" + gameData->prefix + ".tmp"));
        return;
    }

    // Dialog to ask the user to enter their name.
    QDialog *		hsn = new QDialog (view,
                        Qt::WindowTitleHint);
    hsn->setObjectName ("hsNameDialog");

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
        thisUser = "-";
    delete hsn;

    QDate today = QDate::currentDate();
    QString hsDate;
    QString day = today.shortDayName (today.dayOfWeek());
    hsDate = hsDate.sprintf
                ("%s %04d-%02d-%02d",
                day.myStr(),
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
                s2 << hsDate.myStr();
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
            s2 << hsDate.myStr();
        }
        high1.close();
    }
    else {
        // Recode the user's name as UTF-8, in case it contains
        // non-ASCII chars (e.g. "Krüger" is encoded as "KrÃ¼ger").
        s2 << thisUser.toUtf8().constData();
        s2 << (qint16) level;
        s2 << (qint32) score;
        s2 << hsDate.myStr();
    }

    high2.close();

    if (KGrGameIO::safeRename (high2.fileName(),
                userDataDir + "hi_" + gameData->prefix + ".dat")) {
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
    if (gameData->prefix.left (4) == "tute") {
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
    QFile high1 (userDataDir + "hi_" + gameData->prefix + ".dat");
    QDataStream s1;

    if (! high1.exists()) {
        high1.setFileName (systemDataDir + "hi_" + gameData->prefix + ".dat");
        if (! high1.exists()) {
            KGrMessage::information (view, i18n ("Show High Scores"),
                i18n("Sorry, there are no high scores for the \"%1\" game yet.",
                         gameData->name.constData()));
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
    hs->setObjectName ("hsDialog");

    int margin = 10;
    int spacing = 10;
    QVBoxLayout *	mainLayout = new QVBoxLayout (hs);
    mainLayout->setSpacing (spacing);
    mainLayout->setMargin (margin);

    QLabel *		hsHeader = new QLabel (i18n (
                            "<center><h2>KGoldrunner Hall of Fame</h2></center>"
                            "<center><h3>\"%1\" Game</h3></center>",
                            gameData->name.constData()),
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

void KGrGame::dbgControl (int code)
{
    // kDebug() << "Debug code =" << code;
    if (levelPlayer && gameFrozen) {
        levelPlayer->dbgControl (code);
    }
}

/******************************************************************************/
/****************************  MISC SOUND HANDLING  ***************************/
/******************************************************************************/

void KGrGame::heroDigs()
{
    effects->play (fx[DigSound]);
}

void KGrGame::heroStep (bool climbing)
{
    if (climbing) {
	effects->play (fx[ClimbSound]);
    }
    else {
	effects->play (fx[StepSound]);
    }
}

void KGrGame::heroFalls (bool starting)
{
    static int token = -1;
    if (starting) {
	token = effects->play (fx[FallSound]);
    }
    else {
	effects->stop (token);
	token = -1;
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
    return (true);
}

bool KGrGame::loadGameData (Owner o)
{
    KGrGameIO io;
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

/******************************************************************************/
/**********************    MESSAGE BOX WITH FREEZE    *************************/
/******************************************************************************/

void KGrGame::myMessage (QWidget * parent, const QString &title, const QString &contents)
{
    // Halt the game while the message is displayed.
    setMessageFreeze (true);

    KGrMessage::information (parent, title, contents);

    // Unfreeze the game, but only if it was previously unfrozen.
    setMessageFreeze (false);
}

#include "kgrgame.moc"
// vi: set sw=4 :
