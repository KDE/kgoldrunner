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

#include "kgrconsts.h"
// #include "kgrobject.h"
// #include "kgrfigure.h" // OBSOLESCENT - 09/1/09
// #include "kgrrunner.h" // OBSOLESCENT - 18/1/09
#include "kgrcanvas.h"
#include "kgrdialog.h"
#include "kgrsoundbank.h"
#include "kgreditor.h"
#include "kgrlevelplayer.h" // TESTING - 1/1/09

// Obsolete - #include <iostream.h>
#include <iostream>
#include <stdlib.h>
#include <ctype.h>
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
        view (theView),
        systemDataDir (theSystemDir),
        userDataDir (theUserDir),
        level (0),
        fx (NumSounds),
        editor (0)
{
    // Set the game-editor OFF, but available.
    editMode = false;
    paintEditObj = false;
    paintAltObj = false;
    editObj  = BRICK;
    shouldSave = false;

    // OBSOLESCENT - 9/1/09
    // hero = new KGrHero (view, 0, 0);	// The hero is born ... Yay !!!
    // hero->setParent (this);		// Delete hero when KGrGame is deleted.
    // hero->setPlayfield (&playfield);

    setBlankLevel (true);		// Fill the playfield with blank walls.

    // OBSOLESCENT - 18/1/09 enemy = NULL;
    newLevel = true;			// Next level will be a new one.
    loading  = true;			// Stop input until it is loaded.

    gameFrozen = true;
    modalFreeze = false;
    messageFreeze = false;

    gameLogging = false;
    bugFixed = false;

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

    // REPLACE - 9/1/09  Hero's game connections.
    // connect (hero, SIGNAL (gotNugget (int)),  SLOT (incScore (int)));
    // connect (hero, SIGNAL (caughtHero()),     SLOT (herosDead()));
    // connect (hero, SIGNAL (haveAllNuggets()), SLOT (showHiddenLadders()));
    // connect (hero, SIGNAL (leaveLevel()),     SLOT (levelCompleted()));

    dyingTimer = new QTimer (this);
    connect (dyingTimer, SIGNAL (timeout()),  SLOT (finalBreath()));

    // Get the mouse position every 40 msec.  It is used to steer the hero.
    mouseSampler = new QTimer (this);
    connect (mouseSampler, SIGNAL (timeout()), SLOT (readMousePos()));
    mouseSampler->start (40);

    srand (time (0)); 			// Initialise random number generator.
}

KGrGame::~KGrGame()
{
    while (! gameList.isEmpty())
        delete gameList.takeFirst();
    // OBSOLESCENT - 9/1/09
    //release collections
    while (!collections.isEmpty())
        delete collections.takeFirst();
}

void KGrGame::gameActions (int action)
{
    switch (action) {
    case HINT:
	kDebug() << "HINT signal:" << action;
	showHint();
	break;
    case KILL_HERO:
	kDebug() << "KILL_HERO signal:" << action;
	herosDead();
	break;
    default:
	break;
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
	    break;
        case FREE:
        case ENEMY:
        case HERO:
        case BETON:
        case BRICK:
        case FBRICK:
        case HLADDER:
        case LADDER:
        case NUGGET:
        case POLE:
            // Set the next object to be painted in the level-layout.
	    editor->setEditObj (action);
	    break;
        default:
	    break;
        }
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
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame"); // Get prev game.
    QString prevGamePrefix = gameGroup.readEntry ("GamePrefix", "tute");
    int prevLevel = gameGroup.readEntry ("Level_" + prevGamePrefix, 1);
    kDebug()<< "Config() Game and Level" << prevGamePrefix << prevLevel;

    // Use that collection and level, if it is among the current collections.
    int n = 0;
    collnIndex = -1;
    foreach (collection, collections) {
        if (collection->prefix == prevGamePrefix) {
            collnIndex = n;
            level = prevLevel;
            break;
        }
        n++;
    }

    // If not found, set the first collection in the list and level 1.
    if (collnIndex < 0) {
        collnIndex = 0;
        level = 1;
    }

    collection = collections.at (collnIndex);

    kDebug() << "Calling the first view->changeTheme() ...";
    view->changeTheme (initialThemeFilepath);

    emit markRuleType (collection->settings);
    newGame (level, collnIndex);
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
    showTutorialMessages (level);
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
    // Force compile IDW if (! saveOK (false)) {				// Check unsaved work.
        // Force compile IDW return;
    // Force compile IDW }
    // Use dialog box to select game and level: startingAt = ID_FIRST or ID_ANY.
    int selectedLevel = selectLevel (startingAt, requestedLevel);
    if (selectedLevel > 0) {	// If OK, start the selected game and level.
        newGame (selectedLevel, selectedGame);
        showTutorialMessages (level);
    } else {
        level = 0;
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

void KGrGame::herosDead()
{
    if ((level < 1) || (lives <= 0))
        return;			// Game over: we are in the "ENDE" screen.

    // Lose a life.
    if (--lives > 0) {
	effects->play (fx[DeathSound]);
        // Still some life left, so PAUSE and then re-start the level.
        emit showLives (lives);
        gameFrozen = true;	// Freeze the animation and let
        dyingTimer->setSingleShot (true);
        dyingTimer->start (1500);	// the player see what happened.
        view->fadeOut();
    }
    else {
        // Game over.
	effects->play (fx[GameOverSound]);
        emit showLives (lives);
        freeze();
        QString gameOver = "<NOBR><B>" + i18n ("GAME OVER !!!") + "</B></NOBR>";
        KGrMessage::information (view, collection->name, gameOver);
        checkHighScore();	// Check if there is a high score for this game.

        // Offer the player a chance to start this level again with 5 new lives.
        switch (KGrMessage::warning (view, i18n ("Retry Level?"),
                            i18n ("Would you like to try this level again?"),
                            i18n ("&Try Again"), i18n ("&Finish"))) {
        case 0:
            unfreeze();			// Offer accepted.
            newGame (level, collnIndex);
            showTutorialMessages (level);
            return;
            break;
        case 1:
            break;			// Offer rejected.
        }

        // Game completely over: display the "ENDE" screen.
        // OBSOLESCENT - 18/1/09 enemyCount = 0;
        //todo enemies.clear();	// Stop the enemies catching the hero again ...
        // while (!enemies.isEmpty())
                // delete enemies.takeFirst(); // OBSOLETE - 9/1/09

        view->deleteEnemySprites();
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
    if (! editMode) {
        // OBSOLESCENT - 18/1/09 enemyCount = 0;		// Hero is dead: re-start the level.
        loadLevel (level);
    }
    gameFrozen = false;	// Unfreeze the game, but don't move yet.
}

void KGrGame::showHiddenLadders()
{
    effects->play (fx[LadderSound]);

    int i, j;
    for (i = 1; i < 21; i++)
        for (j = 1; j < 29; j++)
        ; // OBSOLESCENT - 20/1/09 Need to compile after kgrobject.cpp removed.
            // if (playfield[j][i]->whatIam() == HLADDER)
                // ((KGrHladder *)playfield[j][i])->showLadder();
    initSearchMatrix();
}
        
void KGrGame::levelCompleted()
{
    effects->play (fx[CompletedSound]);
    connect (view, SIGNAL (fadeFinished()), this, SLOT (goUpOneLevel()));
    view->fadeOut();
}

// 
void KGrGame::goUpOneLevel()
{
    disconnect (view, SIGNAL (fadeFinished()), this, SLOT (goUpOneLevel()));
    lives++;			// Level completed: gain another life.
    emit showLives (lives);
    incScore (1500);

    if (level >= collection->nLevels) {
        freeze();
        KGrMessage::information (view, collection->name,
            i18n ("<b>CONGRATULATIONS !!!!</b>"
            "<p>You have conquered the last level in the "
            "<b>\"%1\"</b> game !!</p>", collection->name));
        checkHighScore();	// Check if there is a high score for this game.

        unfreeze();
        level = 0;		// Game completed: display the "ENDE" screen.
    }
    else {
        level++;		// Go up one level.
        emit showLevel (level);
    }

    // OBSOLESCENT - 18/1/09 enemyCount = 0;
    //enemies.clear();
    // while (!enemies.isEmpty()) // OBSOLETE - 9/1/09
        // delete enemies.takeFirst();

    view->deleteEnemySprites();
    newLevel = true;
    loadLevel (level);
    showTutorialMessages (level);
    newLevel = false;
}

void KGrGame::loseNugget()
{
    // OBSOLESCENT - 9/1/09 hero->loseNugget();		// Enemy trapped/dead and holding a nugget.
}

// OBSOLESCENT - 9/1/09
// KGrHero * KGrGame::getHero()
// {
    // return (hero);		// Return a pointer to the hero.
// }

void KGrGame::setMouseMode (bool on_off)
{
    mouseMode = on_off;		// Set mouse mode on or off.
}

bool KGrGame::inMouseMode()
{
    return (mouseMode);		// Return true if game is under mouse control.
}

bool KGrGame::inEditMode()
{
    return (editMode);		// Return true if the game-editor is active.
}

bool KGrGame::isLoading()
{
    return (loading);		// Return true if a level is being loaded.
}

void KGrGame::setPlaySounds (bool on_off)
{
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    gameGroup.writeEntry ("Sound", on_off);
#ifdef ENABLE_SOUND_SUPPORT
    effects->setMuted (!on_off);
#endif
}

void KGrGame::freeze()
{
    if ((! modalFreeze) && (! messageFreeze)) {
        emit gameFreeze (true);	// Do visual feedback in the GUI.
    }
    gameFrozen = true;	// Halt the game, by blocking all timer events.
}

void KGrGame::unfreeze()
{
    if ((! modalFreeze) && (! messageFreeze)) {
        emit gameFreeze (false);// Do visual feedback in the GUI.
    }
    gameFrozen = false;	// Restart the game.  Because frozen == false,
    restart();			// the game goes on running after the next step.
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

void KGrGame::setBlankLevel (bool playable)
{
    // OBSOLESCENT - 20/1/09 KGrLevelGrid replaces playfield[][].
    return; // Don't reference playfield[][].

    for (int j = 0; j < 20; j++)
        for (int i = 0; i < 28; i++) {
            if (playable) {
                // playfield[i+1][j+1] = new KGrFree (FREE, i+1, j+1, view);
            }
            else {
                // playfield[i+1][j+1] = new KGrEditable (FREE);
                view->paintCell (i+1, j+1, FREE);
            }
	    // playfield[i+1][j+1]->setParent (this); // Delete if KGrGame dies.
            editObjArray[i+1][j+1] = FREE;
        }
    for (int j = 0; j < 30; j++) {
        // playfield[j][0] = new KGrObject (BETON);
	// playfield[j][0]->setParent (this);	// Delete at end of KGrGame.
        editObjArray[j][0] = BETON;

        // playfield[j][21] = new KGrObject (BETON);
	// playfield[j][21]->setParent (this);	// Delete at end of KGrGame.
        editObjArray[j][21] = BETON;
    }
    for (int i = 0; i < 22; i++) {
        // playfield[0][i] = new KGrObject (BETON);
	// playfield[0][i]->setParent (this);	// Delete at end of KGrGame.
        editObjArray[0][i] = BETON;

        // playfield[29][i] = new KGrObject (BETON);
	// playfield[29][i]->setParent (this);	// Delete at end of KGrGame.
        editObjArray[29][i] = BETON;
    }
}
    
void KGrGame::newGame (const int lev, const int gameIndex)
{
    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = true;		// "loadLevel (level)" will reset it.

    view->goToBlack();
    if (editMode) {
        emit setEditMenu (false);	// Disable edit menu items and toolbar.

        editMode = false;
        paintEditObj = false;
        paintAltObj = false;
        editObj = BRICK;

        view->setHeroVisible (true);
    }

    newLevel = true;

    level = lev;
    collnIndex = gameIndex;
    collection = collections.at (collnIndex);
    owner = collection->owner;

    lives = 5;				// Start with 5 lives.
    score = 0;
    startScore = 0;

    emit showLives (lives);
    emit showScore (score);
    emit showLevel (level);

    // OBSOLESCENT - 18/1/09 enemyCount = 0;

    //enemies.clear();
    // OBSOLESCENT - 9/1/09 while (!enemies.isEmpty())
        // OBSOLESCENT - 9/1/09 delete enemies.takeFirst();

    view->deleteEnemySprites();

    newLevel = true;;
    loadLevel (level);
    newLevel = false;
}

void KGrGame::startTutorial()
{
    // Force compile IDW if (! saveOK (false)) {				// Check unsaved work.
        // Force compile IDW return;
    // Force compile IDW }

    int i, index;
    int imax = collections.count();
    bool found = false;

    index = 0;
    for (i = 0; i < imax; i++) {
        index = i;			// Index within owner.
        if (collections.at (i)->prefix == "tute") {
            found = true;
            break;
        }
    }
    if (found) {
        // Start the tutorial.
        collection = collections.at (index);
        owner = collection->owner;
        emit markRuleType (collection->settings);
        collnIndex = index;
        level = 1;
        newGame (level, collnIndex);
        showTutorialMessages (level);
    }
    else {
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
    KGrLevelData d;
    if (! readLevelData (levelNo, d)) {
        loading = false;
        return 0;
    }

    view->setLevel (levelNo);		// Switch and render background if reqd.
    view->fadeIn();			// Then run the fade-in animation.
    nuggets = 0;
    // OBSOLESCENT - 18/1/09 enemyCount=0;
    startScore = score;			// The score we will save, if asked.

    // TESTING
    kDebug() << "Prefix" << collection->prefix << "index" << collnIndex << "of" << gameList.count();
    KGrGameData * testGame = gameList.at (collnIndex);
    // KGrGameData  * testGame = new KGrGameData;
    // testGame->owner   = collection->owner;
    // testGame->nLevels = collection->nLevels;
    // testGame->rules   = collection->settings;
    // testGame->prefix  = collection->prefix;
    // testGame->skill   = collection->skill;
    // testGame->width   = FIELDWIDTH;
    // testGame->height  = FIELDHEIGHT;
    // testGame->name    = ""; // collection->name;
    // testGame->about   = ""; // collection->about;

    KGrLevelData * testData = new KGrLevelData;
    testData->level   = levelNo;
    testData->width   = FIELDWIDTH;
    testData->height  = FIELDHEIGHT;
    testData->layout  = d.layout;
    testData->name    = d.name;
    testData->hint    = d.hint;

    levelPlayer = new KGrLevelPlayer (this, testGame, testData); // TESTING

    levelPlayer->init (view);

    // OBSOLESCENT - 9/1/09
    int i, j;
    // Load the level-layout, hero and enemies.
    for (j = 1; j <= FIELDHEIGHT; j++) {
        for (i = 1; i <= FIELDWIDTH; i++) {
            changeObject (d.layout.at ((j - 1) * FIELDWIDTH + (i - 1)), i , j);
        }
    }
    // END ...

    // If there is a name, translate the UTF-8 coded QByteArray right now.
    levelName = (d.name.size() > 0) ? i18n ((const char *) d.name) : "";

    // Indicate on the menus whether there is a hint for this level.
    int len = d.hint.length();
    emit hintAvailable (len > 0);

    // If there is a hint, translate it right now.
    levelHint = (len > 0) ? i18n ((const char *) d.hint) : "";

    // Disconnect edit-mode slots from signals from "view".
    disconnect (view, SIGNAL (mouseClick (int)), 0, 0);
    disconnect (view, SIGNAL (mouseLetGo (int)), 0, 0);

    if (newLevel) {
        // OBSOLESCENT - 9/1/09 hero->setEnemyList (&enemies);
        // QListIterator<KGrEnemy *> i (enemies);
        // while (i.hasNext()) {
            // OBSOLESCENT - 9/1/09 KGrEnemy * enemy = i.next();
            // OBSOLESCENT - 9/1/09 enemy->setEnemyList (&enemies);
        // }
    }

    // OBSOLESCENT - 9/1/09 hero->setNuggets (nuggets);
    setTimings();

    // Make a new sequence of all possible x co-ordinates for enemy rebirth.
    // OBSOLESCENT - 9/1/09
    // if (KGrFigure::reappearAtTop && (enemies.count() > 0)) {
        // KGrEnemy::makeReappearanceSequence();
    // }

    // Set direction-flags to use during enemy searches.
    initSearchMatrix();

    // Re-draw the playfield frame, level title and figures.
    view->setTitle (getTitle());

    // If in mouse mode, not keyboard mode, put the mouse pointer on the hero.
    if (mouseMode) {
        view->setMousePos (startI, startJ);
    }

    // If we are starting a new level, save it in the player's config file.
    if (newLevel) {
        KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
        gameGroup.writeEntry ("GamePrefix", collection->prefix);
        gameGroup.writeEntry ("Level_" + collection->prefix, level);
        gameGroup.sync();		// Ensure that the entry goes to disk.
    }

    // Connect play-mode slot to signal from "view".
    connect (view, SIGNAL (mouseClick (int)), SLOT (doDig (int)));

    // Re-enable player input.
    loading = false;

    return 1;
}

void KGrGame::showTutorialMessages (int levelNo)
{
    // Halt the game during message displays and mouse pointer moves.
    setMessageFreeze (true);

    // Check if this is a tutorial collection and not on the "ENDE" screen.
    if ((collection->prefix.left (4) == "tute") && (levelNo != 0)) {

        // At the start of a tutorial, put out an introduction.
        if (levelNo == 1) {
            KGrMessage::information (view, collection->name,
                        i18n (collection->about.toUtf8().constData()));
        }
        // Put out an explanation of this level.
        KGrMessage::information (view, getTitle(), levelHint);
    }

    // If in mouse mode, make sure the mouse pointer is back on the hero.
    if (mouseMode) {
        view->setMousePos (startI, startJ);
    }
    setMessageFreeze (false);	// Let the level begin.
}

bool KGrGame::readLevelData (int levelNo, KGrLevelData & d)
{
    KGrGameIO io;
    // If system game or ENDE screen, choose system dir, else choose user dir.
    const QString dir = ((owner == SYSTEM) || (levelNo == 0)) ?
                                        systemDataDir : userDataDir;
    QString filePath;
    IOStatus stat = io.fetchLevelData (dir, collection->prefix, levelNo,
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

void KGrGame::changeObject (unsigned char kind, int i, int j)
{
    // OBSOLESCENT - 20/1/09 Done in KGrLevelGrid now.
    // delete playfield[i][j];
    switch (kind) {
    case FREE:    break; // createObject (new KGrFree (FREE,i,j,view),FREE,i,j);break;
    case LADDER:  break; // createObject (new KGrObject (LADDER),LADDER,i,j);break;
    case HLADDER: break; // createObject (new KGrHladder (HLADDER,i,j,view),FREE,i,j);break;
    case BRICK:   break; // createObject (new KGrBrick (BRICK,i,j,view),BRICK,i,j);break;
    case BETON:   break; // createObject (new KGrObject (BETON),BETON,i,j);break;
    case FBRICK:  break; // createObject (new KGrObject (FBRICK),BRICK,i,j);break;
    case POLE:    break; // createObject (new KGrObject (POLE),POLE,i,j);break;
    case NUGGET:  break; // createObject (new KGrFree (NUGGET,i,j,view),NUGGET,i,j);
                                  nuggets++;break;
    case HERO:    // createObject (new KGrFree (FREE,i,j,view),FREE,i,j);
        // OBSOLESCENT - 9/1/09 hero->init (i,j);
        startI = i; startJ = j;
        // OBSOLESCENT - 9/1/09 hero->started = false;
        // OBSOLESCENT - 9/1/09 hero->showFigure();
        break;
    case ENEMY:   // createObject (new KGrFree (FREE,i,j,view),FREE,i,j);
        if (newLevel) {
            // OBSOLESCENT - 9/1/09
            // // Starting a level for the first time.
            // enemy = new KGrEnemy (view, i, j);
	    // enemy->setParent (this);	// Delete enemy when KGrGame is deleted.
            // enemy->setPlayfield (&playfield);
            // enemy->enemyId = enemyCount++;
            // enemies.append (enemy);
            // connect (enemy, SIGNAL (lostNugget()), SLOT (loseNugget()));
            // connect (enemy, SIGNAL (trapped (int)), SLOT (incScore (int)));
            // connect (enemy, SIGNAL (killed (int)),  SLOT (incScore (int)));
        }
        else {
            // Starting a level again after losing.
            // enemy=enemies.at (enemyCount);
            // enemy->enemyId=enemyCount++;
            // enemy->setNuggets (0);
            // enemy->init (i,j);	// Re-initialise the enemy's state information.
        }
        // enemy->showFigure();
        break;
    default :  break; // createObject (new KGrBrick (BRICK,i,j,view),BRICK,i,j);break;
    }
}

// OBSOLESCENT - 20/1/09 void KGrGame::createObject (KGrObject *o, char picType, int x, int y)
// {
    // playfield[x][y] = o;
    // o->setParent (this);		// Delete cell if KGrGame is deleted.
    // view->paintCell (x, y, picType);		// Pic maybe not same as object.
// }

void KGrGame::setTimings()
{
    // OBSOLESCENT - 9/1/09
    // Timing *	timing;
    // int		c = -1;

    // OBSOLESCENT - 9/1/09
    // if (KGrFigure::variableTiming) {
        // c = enemies.count();			// Timing based on enemy count.
        // c = (c > 5) ? 5 : c;
        // timing = &(KGrFigure::varTiming[c]);
    // }
    // else {
        // timing = &(KGrFigure::fixedTiming);	// Fixed timing.
    // }

    // KGrHero::WALKDELAY		= timing->hwalk;
    // KGrHero::FALLDELAY		= timing->hfall;
    // KGrEnemy::WALKDELAY		= timing->ewalk;
    // KGrEnemy::FALLDELAY		= timing->efall;
    // KGrEnemy::CAPTIVEDELAY	= timing->ecaptive;
    // KGrBrick::HOLETIME		= timing->hole;
}

void KGrGame::initSearchMatrix()
{
    // OBSOLESCENT - 20/1/09 Should be replaced by KGrLevelGrid object.
    // // Called at start of level and also when hidden ladders appear.
    // int i, j;
// 
    // for (i = 1; i < 21; i++) {
        // for (j = 1; j < 29; j++) {
            // // If on ladder, can walk L, R, U or D.
            // if (playfield[j][i]->whatIam() == LADDER)
                // playfield[j][i]->searchValue = CANWALKLEFT + CANWALKRIGHT +
                                              // CANWALKUP + CANWALKDOWN;
            // else
                // // If on solid ground, can walk L or R.
                // if ((playfield[j][i+1]->whatIam() == BRICK) ||
                    // (playfield[j][i+1]->whatIam() == HOLE) ||
                    // (playfield[j][i+1]->whatIam() == USEDHOLE) ||
                    // (playfield[j][i+1]->whatIam() == BETON))
                    // playfield[j][i]->searchValue = CANWALKLEFT + CANWALKRIGHT;
                // else
                    // // If on pole or top of ladder, can walk L, R or D.
                    // if ((playfield[j][i]->whatIam() == POLE) ||
                        // (playfield[j][i+1]->whatIam() == LADDER))
                        // playfield[j][i]->searchValue = CANWALKLEFT +
                                              // CANWALKRIGHT + CANWALKDOWN;
                    // else
                        // // Otherwise, gravity takes over ...
                        // playfield[j][i]->searchValue = CANWALKDOWN;
     //  
            // // Clear corresponding bits if there are solids to L, R, U or D.
            // if (playfield[j][i-1]->blocker)
                // playfield[j][i]->searchValue &= ~CANWALKUP;
            // if (playfield[j-1][i]->blocker)
                // playfield[j][i]->searchValue &= ~CANWALKLEFT;
            // if (playfield[j+1][i]->blocker)
                // playfield[j][i]->searchValue &= ~CANWALKRIGHT;
            // if (playfield[j][i+1]->blocker)
                // playfield[j][i]->searchValue &= ~CANWALKDOWN;
        // }
    // }
}
        
void KGrGame::startPlaying() {
    // OBSOLESCENT - 9/1/09
    // if (! hero->started) {
        // // Start the enemies and the hero.
        // for (--enemyCount; enemyCount>=0; --enemyCount) {
            // enemy=enemies.at (enemyCount);
            // enemy->startSearching();
        // }
        // hero->start();
    // }
}

QString KGrGame::getDirectory (Owner o)
{
    return ((o == SYSTEM) ? systemDataDir : userDataDir);
}

QString KGrGame::getFilePath (Owner o, KGrCollection * colln, int lev)
{
    QString filePath;

    if (lev == 0) {
        // End of game: show the "ENDE" screen.
        o = SYSTEM;
        filePath = "level000.grl";
    }
    else {
        filePath.setNum (lev);		// Convert INT -> QString.
        filePath = filePath.rightJustified (3,'0'); // Add 0-2 zeros at left.
        filePath.append (".grl");	// Add KGoldrunner level-suffix.
        filePath.prepend (colln->prefix);	// Add collection file-prefix.
    }

    filePath.prepend (((o == SYSTEM)? systemDataDir : userDataDir) + "levels/");

    return (filePath);
}

QString KGrGame::getTitle()
{
    QString levelTitle;
    QString levelNumber;
    if (level == 0) {
        // Generate a special title: end of game or creating a new level.
        if (! editMode)
            levelTitle = i18n ("T H E   E N D");
        else
            levelTitle = i18n ("New Level");
    }
    else {
        // Generate title string "Collection-name - NNN - Level-name".
        levelNumber.setNum (level);
        levelNumber = levelNumber.rightJustified (3,'0');
        if (levelName.length() <= 0) {
            levelTitle = i18nc ("Game name - level number.",
                    "%1 - %2", collection->name, levelNumber);
        }
        else {
            levelTitle = i18nc ("Game name - level number - level name.",
                    "%1 - %2 - %3", collection->name, levelNumber, levelName);
        }
    }
    return (levelTitle);
}

void KGrGame::readMousePos()
{
    QPoint p;
    int i, j;

    // If loading a level for play or editing, ignore mouse-position input.
    if (loading) return;

    // If game control is currently by keyboard, ignore the mouse.
    if ((! mouseMode) && (! editMode)) {
        if (gameFrozen) return;	// If game is stopped, do nothing.
        levelPlayer->tick();
        return;
    }

    p = view->getMousePos();
    i = p.x(); j = p.y();

    if (editMode) {
        // Editing - check if we are in paint mode and have moved the mouse.
        if (paintEditObj && ((i != oldI) || (j != oldJ))) {
            // Force compile IDW insertEditObj (i, j, editObj);
            oldI = i;
            oldJ = j;
        }
        if (paintAltObj && ((i != oldI) || (j != oldJ))) {
            // Force compile IDW insertEditObj (i, j, FREE);
            oldI = i;
            oldJ = j;
        }
        // Highlight the cursor position
	
    }
    else {
        // Playing - if  the level has started, control the hero.
        if (gameFrozen) return;	// If game is stopped, do nothing.

        if (levelPlayer) { // OBSOLESCENT - 7/1/09 - Should be sure it exists.
            levelPlayer->setTarget (i, j);
        }
    }

    if (gameFrozen) return;	// If game is stopped, do nothing.
    levelPlayer->tick();
}

void KGrGame::doDig (int button) {

    // If game control is currently by keyboard, ignore the mouse.
    if (editMode) return;
    if (! mouseMode) return;

    // If loading a level for play or editing, ignore mouse-button input.
    if ((! loading) && (! gameFrozen)) {
        // OBSOLESCENT - 7/1/09 }
        // if (! hero->started) {
            // startPlaying();	// If first player-input, start playing.
        // }
        switch (button) {
        // OBSOLESCENT - 7/1/09
        case Qt::LeftButton:	break; // hero->digLeft(); break;
        case Qt::RightButton:	break; // hero->digRight(); break;
        default:		break;
        }
    }
}

void KGrGame::kbControl (int dirn)
{
    kDebug() << "Keystroke setting direction" << dirn;
    if (editMode) return;

    // Using keyboard control can automatically disable mouse control.
    if (mouseMode) {
        // Halt the game while a message is displayed.
        setMessageFreeze (true);

        switch (KMessageBox::questionYesNo (view, 
                i18n ("You have pressed a key that can be used to move the "
                "Hero. Do you want to switch automatically to keyboard "
                "control? Mouse control is easier to use in the long term "
                "- like riding a bike rather than walking!"),
                i18n ("Switch to Keyboard Mode"),
                KGuiItem (i18n ("Switch to &Keyboard Mode")),
                KGuiItem (i18n ("Stay in &Mouse Mode")),
                i18n ("Keyboard Mode")))
        {
        case KMessageBox::Yes: 
            mouseMode = false;				// Set mouse mode OFF.
            // TODO - Connect these signals in kgoldrunner.cpp somewhere.
            emit setToggle ("mouse_mode", false);	// Adjust Settings menu.
            emit setToggle ("keyboard_mode", true);
            break;
        case KMessageBox::No: 
            break;
        }

        // Unfreeze the game, but only if it was previously unfrozen.
        setMessageFreeze (false);

        if (mouseMode)
            return;                    		// Stay in Mouse Mode.
    }

    // if (level != 0)
    // {
        // OBSOLESCENT - 9/1/09 if (! hero->started)	// Start when first movement
        // startPlaying();			// key is pressed ...
        // TODO - Pass direction to the hero heroAction (movement);
    // }
    if (levelPlayer) {
        levelPlayer->setDirection ((Direction) dirn);
    }
}

void KGrGame::heroAction (KBAction movement)
{
    // OBSOLESCENT - 7/1/09
    switch (movement) {
    case KB_UP:		break; // hero->setKey (UP); break;
    case KB_DOWN:	break; // hero->setKey (DOWN); break;
    case KB_LEFT:	break; // hero->setKey (LEFT); break;
    case KB_RIGHT:	break; // hero->setKey (RIGHT); break;
    case KB_STOP:	break; // hero->setKey (STAND); break;
    case KB_DIGLEFT:	break; // hero->setKey (STAND); hero->digLeft(); break;
    case KB_DIGRIGHT:	break; // hero->setKey (STAND); hero->digRight(); break;
    }
}

/******************************************************************************/
/**************************  SAVE AND RE-LOAD GAMES  **************************/
/******************************************************************************/

void KGrGame::saveGame()		// Save game ID, score and level.
{
    if (editMode) {myMessage (view, i18n ("Save Game"),
        i18n ("Sorry, you cannot save your game play while you are editing. "
        "Please try menu item \"%1\".",
        i18n ("&Save Edits...")));
        return;
    }
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
                collection->prefix.myStr(), level, lives, startScore,
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
        while ((! text1.endData()) && (--n > 0)) {
            saved = text1.readLine() + '\n';
            text2 << saved;
        }
        file1.close();
    }

    file2.close();

    if (safeRename (userDataDir+"savegame.tmp", userDataDir+"savegame.dat")) {
        KGrMessage::information (view, i18n ("Save Game"),
                                i18n ("Your game has been saved."));
    }
    else {
        KGrMessage::information (view, i18n ("Save Game"),
                                i18n ("Error: Failed to save your game."));
    }
}

bool KGrGame::safeRename (const QString & oldName, const QString & newName)
{
    QFile newFile (newName);
    if (newFile.exists()) {
        // On some file systems we cannot rename if a file with the new name
        // already exists.  We must delete the existing file, otherwise the
        // upcoming QFile::rename will fail, according to Qt4 docs.  This
        // seems to be true with reiserfs at least.
        if (! newFile.remove()) {
            KGrMessage::information (view, i18n ("Rename File"),
                i18n ("Cannot delete previous version of file '%1'.", newName));
            return false;
        }
    }
    QFile oldFile (oldName);
    if (! oldFile.rename (newName)) {
        KGrMessage::information (view, i18n ("Rename File"),
            i18n ("Cannot rename file '%1' to '%2'.", oldName, newName));
        return false;
    }
    return true;
}

void KGrGame::loadGame()		// Re-load game, score and level.
{
    // Force compile IDW if (! saveOK (false)) {				// Check unsaved work.
        // Force compile IDW return;
    // Force compile IDW }

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

    KGrLGDialog * lg = new KGrLGDialog (&savedGames, collections, view);

    if (lg->exec() == QDialog::Accepted) {
        s = lg->getCurrentText();
    }

    bool found = false;
    QString pr;
    int  lev;
    int i;
    int imax = collections.count();

    if (! s.isNull()) {
        pr = s.mid (21, 7);			// Get the collection prefix.
        pr = pr.left (pr.indexOf (" ", 0, Qt::CaseInsensitive));

        for (i = 0; i < imax; i++) {		// Find the collection.
            if (collections.at (i)->prefix == pr) {
                collection = collections.at (i);
                collnIndex  = i;
                owner = collections.at (i)->owner;
                found = true;
                break;
            }
        }
        if (found) {
            // Set the rules for the selected game.
            emit markRuleType (collection->settings);
            lev   = s.mid (28, 3).toInt();
            newGame (lev, collnIndex);		// Re-start the selected game.
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

/******************************************************************************/
/**************************  HIGH-SCORE PROCEDURES  ***************************/
/******************************************************************************/

void KGrGame::checkHighScore()
{
    // Don't keep high scores for tutorial games.
    if (collection->prefix.left (4) == "tute")
        return;

    if (score <= 0)
        return;

#ifdef USE_KSCOREDIALOG
    KScoreDialog scoreDialog (
            KScoreDialog::Name | KScoreDialog::Level | 
            KScoreDialog::Date | KScoreDialog::Score, 
            view);
    scoreDialog.setConfigGroup (collection->prefix);
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
    QFile high1 (userDataDir + "hi_" + collection->prefix + ".dat");
    QDataStream s1;

    if (! high1.exists()) {
        high1.setFileName (systemDataDir + "hi_" + collection->prefix + ".dat");
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

    QFile high2 (userDataDir + "hi_" + collection->prefix + ".tmp");
    QDataStream s2;

    if (! high2.open (QIODevice::WriteOnly)) {
        KGrMessage::information (view, i18n ("Check for High Score"),
                i18n ("Cannot open file '%1' for output.",
                 userDataDir + "hi_" + collection->prefix + ".tmp"));
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
        while ((! s1.endData()) && (highCount < 10)) {
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

    if (safeRename (high2.fileName(),
                userDataDir + "hi_" + collection->prefix + ".dat")) {
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
    if (collection->prefix.left (4) == "tute") {
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
    QFile high1 (userDataDir + "hi_" + collection->prefix + ".dat");
    QDataStream s1;

    if (! high1.exists()) {
        high1.setFileName (systemDataDir + "hi_" + collection->prefix + ".dat");
        if (! high1.exists()) {
            KGrMessage::information (view, i18n ("Show High Scores"),
                i18n("Sorry, there are no high scores for the \"%1\" game yet.",
                         collection->name));
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
                            collection->name),
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
    while ((! s1.endData()) && (n < 10)) {
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

void KGrGame::doStep()
{
    if (gameFrozen) {	// The game must have been halted.
        restart();		// Do one step and halt again.
    }
}

void KGrGame::restart()
{
    bool temp;
    // int i,j;

    if (editMode)		// Can't move figures when in Edit Mode.
        return;

    temp = gameFrozen;

    gameFrozen = false;	// Temporarily restart the game, by re-running
                                // any timer events that have been blocked.

    // OBSOLESCENT - 7/1/09
    // readMousePos();		// Set hero's direction.
    // hero->doStep();		// Move the hero one step.

    // OBSOLESCENT - 7/1/09
    // j = enemies.count();	// Move each enemy one step.
    // for (i = 0; i < j; i++) {
        // enemy = enemies.at (i);	// Need to use an index because called methods
        // enemy->doStep();	// change the "current()" of the "enemies" list.
    // }

    // OBSOLESCENT - 20/1/09 Need to compile after kgrobject.cpp removed.
    // for (i = 1; i <= 28; i++)
        // for (j = 1; j <= 20; j++) {
            // if ((playfield[i][j]->whatIam() == HOLE) ||
                // (playfield[i][j]->whatIam() == USEDHOLE) ||
                // (playfield[i][j]->whatIam() == BRICK))
                // ((KGrBrick *)playfield[i][j])->doStep();
        // }

    gameFrozen = temp;	// If frozen was true, halt again, which gives a
                                // single-step effect, otherwise go on running.
}

void KGrGame::showFigurePositions()
{
    // OBSOLESCENT - 7/1/09
    if (gameFrozen) {
        // hero->showState ('p');
        // QListIterator<KGrEnemy *> i (enemies);
        // while (i.hasNext()) {
            // KGrEnemy * enemy = i.next();
            // enemy->showState ('p');
        // }
    }
}

void KGrGame::showHeroState()
{
    // OBSOLESCENT - 7/1/09
    if (gameFrozen) {
        // hero->showState ('s');
    }
}

void KGrGame::showEnemyState (int /* enemyId */)
{
    // OBSOLESCENT - 7/1/09
    if (gameFrozen) {
        // QListIterator<KGrEnemy *> i (enemies);
        // while (i.hasNext()) {
                // KGrEnemy * enemy = i.next();
                // if (enemy->enemyId == enemyId) enemy->showState ('s');
        // }
    }
}

void KGrGame::showObjectState()
{
    QPoint p;
    int i, j;
    // OBSOLESCENT - 20/1/09 KGrObject * myObject;

    if (gameFrozen) {
        p = view->getMousePos();
        i = p.x(); j = p.y();
        // OBSOLESCENT - 20/1/09 Need to compile after kgrobject.cpp removed.
        // myObject = playfield[i][j];
        // switch (myObject->whatIam()) {
            // case BRICK:
            // case HOLE:
            // case USEDHOLE:
                // ((KGrBrick *)myObject)->showState (i, j); break;
            // default: myObject->showState (i, j); break;
        // }
    }
}

void KGrGame::bugFix()
{
    if (gameFrozen) {		// Toggle a bug fix on/off dynamically.
        bugFixed = (bugFixed) ? false : true;
        printf ("%s", (bugFixed) ? "\n" : "");
        printf (">>> Bug fix is %s\n", (bugFixed) ? "ON" : "OFF\n");
    }
}

void KGrGame::startLogging()
{
    if (gameFrozen) {		// Toggle logging on/off dynamically.
        gameLogging = (gameLogging) ? false : true;
        printf ("%s", (gameLogging) ? "\n" : "");
        printf (">>> Logging is %s\n", (gameLogging) ? "ON" : "OFF\n");
    }
}

/******************************************************************************/
/**********************    LEVEL SELECTION DIALOG BOX    **********************/
/******************************************************************************/

int KGrGame::selectLevel (int action, int requestedLevel)
{
    int selectedLevel = 0;		// 0 = no selection (Cancel) or invalid.

    // Halt the game during the dialog.
    modalFreeze = false;
    if (! gameFrozen) {
        modalFreeze = true;
        freeze();
    }

    // Create and run a modal dialog box to select a game and level.
    KGrSLDialog * sl = new KGrSLDialog (action, requestedLevel, collnIndex,
                                        collections, this, view);
    while (sl->exec() == QDialog::Accepted) {
        selectedGame = sl->selectedGame();
        selectedLevel = 0;	// In case the selection is invalid.
        if (collections.at (selectedGame)->owner == SYSTEM) {
            switch (action) {
            case SL_CREATE:	// Can save only in a USER collection.
            case SL_SAVE:
            case SL_MOVE:
                KGrMessage::information (view, i18n ("Select Level"),
                        i18n ("Sorry, you can only save or move "
                        "into one of your own games."));
                continue;			// Re-run the dialog box.
                break;
            case SL_DELETE:	// Can delete only in a USER collection.
                KGrMessage::information (view, i18n ("Select Level"),
                        i18n ("Sorry, you can only delete a level "
                        "from one of your own games."));
                continue;			// Re-run the dialog box.
                break;
            case SL_UPD_GAME:	// Can edit info only in a USER collection.
                KGrMessage::information (view, i18n ("Edit Game Info"),
                        i18n ("Sorry, you can only edit the game "
                        "information on your own games."));
                continue;			// Re-run the dialog box.
                break;
            default:
                break;
            }
        }

        selectedLevel = sl->selectedLevel();
        if ((selectedLevel > collections.at (selectedGame)->nLevels) &&
            (action != SL_CREATE) && (action != SL_SAVE) &&
            (action != SL_MOVE) && (action != SL_UPD_GAME)) {
            KGrMessage::information (view, i18n ("Select Level"),
                i18n ("There is no level %1 in %2, "
                "so you cannot play or edit it.",
                 selectedLevel,
                 "\"" + collections.at (selectedGame)->name + "\""));
            selectedLevel = 0;			// Set an invalid selection.
            continue;				// Re-run the dialog box.
        }

        // If "OK", set the results.
        collection = collections.at (selectedGame);
        owner = collection->owner;
        collnIndex = selectedGame;
        // Set default rules for selected game.
        emit markRuleType (collection->settings);
        break;
    }

    // Unfreeze the game, but only if it was previously unfrozen.
    if (modalFreeze) {
        unfreeze();
        modalFreeze = false;
    }

    delete sl;
    return (selectedLevel);			// 0 = canceled or invalid.
}

bool KGrGame::ownerOK (Owner o)
{
    // Check that this owner has at least one collection.
    bool OK = false;

    QListIterator<KGrCollection *> i (collections);
    while (i.hasNext()) {
        KGrCollection * c = i.next();
        if (c->owner == o) {
            OK = true;
            break;
        }		// Pit is blocked.  Find another way.
    }

    return (OK);
}

/******************************************************************************/
/**********************    CLASS TO DISPLAY THUMBNAIL   ***********************/
/******************************************************************************/

KGrThumbNail::KGrThumbNail (QWidget * parent, const char * name)
                        : QFrame (parent)
{
    setObjectName (name);
    // Let the parent do all the work.  We need a class here so that
    // QFrame::drawContents (QPainter *) can be re-implemented and
    // the thumbnail can be automatically re-painted when required.
}

void KGrThumbNail::setLevelData (const QString& dir, const QString& prefix, int level,
                                        QLabel * sln)
{
    KGrGameIO io;
    KGrLevelData d;
    QString filePath;

    IOStatus stat = io.fetchLevelData (dir, prefix, level, d, filePath);
    if (stat == OK) {
        // Keep a safe copy of the layout.  Translate and display the name.
        levelLayout = d.layout;
        sln->setText ((d.name.size() > 0) ? i18n ((const char *) d.name) : "");
    }
    else {
        // Level-data inaccessible or not found.
        levelLayout = "";
        sln->setText ("");
    }
}

// This was previously a Q3Frame event
//     void KGrThumbNail::drawContents (QPainter * p)
//
// In Qt4 there is no longer such a method for QFrame.  The
// workaround is to reimplement paintEvent for this widget.

void KGrThumbNail::paintEvent (QPaintEvent * /* event (unused) */)
{
    QPainter    p (this);
    QFile	openFile;
    QPen	pen = p.pen();
    char	obj = FREE;
    int		fw = 1;				// Set frame width.
    int		n = width() / FIELDWIDTH;	// Set thumbnail cell-size.

    QColor backgroundColor = QColor ("#000038"); // Midnight blue.
    QColor brickColor =      QColor ("#9c0f0f"); // Oxygen's brick-red.
    QColor concreteColor =   QColor ("#585858"); // Dark grey.
    QColor ladderColor =     QColor ("#a0a0a0"); // Steely grey.
    QColor poleColor =       QColor ("#a0a0a0"); // Steely grey.
    QColor heroColor =       QColor ("#00ff00"); // Green.
    QColor enemyColor =      QColor ("#0080ff"); // Bright blue.
    QColor gold;
    gold.setNamedColor ("gold");		 // Gold.

    pen.setColor (backgroundColor);
    p.setPen (pen);

    if (levelLayout.isEmpty()) {
        // There is no file, so fill the thumbnail with "FREE" cells.
        p.drawRect (QRect (fw, fw, FIELDWIDTH*n, FIELDHEIGHT*n));
        return;
    }

    for (int j = 0; j < FIELDHEIGHT; j++)
    for (int i = 0; i < FIELDWIDTH; i++) {

        obj = levelLayout.at (j*FIELDWIDTH + i);

        // Set the colour of each object.
        switch (obj) {
        case BRICK:
        case FBRICK:
            pen.setColor (brickColor); p.setPen (pen); break;
        case BETON:
            pen.setColor (concreteColor); p.setPen (pen); break;
        case LADDER:
            pen.setColor (ladderColor); p.setPen (pen); break;
        case POLE:
            pen.setColor (poleColor); p.setPen (pen); break;
        case HERO:
            pen.setColor (heroColor); p.setPen (pen); break;
        case ENEMY:
            pen.setColor (enemyColor); p.setPen (pen); break;
        default:
            // Set the background colour for FREE, HLADDER and NUGGET.
            pen.setColor (backgroundColor); p.setPen (pen); break;
        }

        // Draw n x n pixels as n lines of length n.
        p.drawLine (i*n+fw, j*n+fw, i*n+(n-1)+fw, j*n+fw);
        if (obj == POLE) {
            // For a pole, only the top line is drawn in white.
            pen.setColor (backgroundColor);
            p.setPen (pen);
        }
        for (int k = 1; k < n; k++) {
            p.drawLine (i*n+fw, j*n+k+fw, i*n+(n-1)+fw, j*n+k+fw);
        }

        // For a nugget, add just a vertical touch  of yellow (2-3 pixels).
        if (obj == NUGGET) {
            int k = (n/2)+fw;
            pen.setColor (gold);	// Gold.
            p.setPen (pen);
            p.drawLine (i*n+k, j*n+k, i*n+k, j*n+(n-1)+fw);
            p.drawLine (i*n+k+1, j*n+k, i*n+k+1, j*n+(n-1)+fw);
        }
    }

    // Finally, draw a small black border around the outside of the thumbnail.
    pen.setColor (Qt::black); 
    p.setPen (pen);
    p.drawRect (rect().left(), rect().top(), rect().right(), rect().bottom());
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

/******************************************************************************/
/*************************   COLLECTIONS HANDLING   ***************************/
/******************************************************************************/

// NOTE: Macros "myStr" and "myChar", defined in "kgrgame.h", are used
//       to smooth out differences between Qt 1 and Qt2 QString classes.

bool KGrGame::initCollections()
{
    // Initialise the list of collections of levels (i.e. the list of games).
    //collections.setAutoDelete (true);
    owner = SYSTEM;				// Use system levels initially.
    if (! loadCollections (SYSTEM))		// Load system collections list.
        return (false);				// If no collections, abort.
    loadCollections (USER);			// Load user collections list.
                                                // If none, don't worry.

    // DISABLED by IDW  mapCollections();	// Check ".grl" file integrity.

    return (true);
}

void KGrGame::mapCollections()
{
    QDir		d;
    KGrCollection *	colln;
    QString		d_path;
    QString		fileName1;
    QString		fileName2;

    // Find KGoldrunner level files, sorted by name (same as numerical order).
    QListIterator<KGrCollection *> i (collections);
    while (i.hasNext()) {
        colln = i.next();
        if (colln->owner == SYSTEM) {
            // Skip these checks now we are using KGoldrunner 3 file formats.
            continue;
        }
        d.setPath (getDirectory (colln->owner) + "levels/");
        d_path = d.path();
        if (! d.exists()) {
            // There is no "levels" sub-directory: OK if game has no levels yet.
            if (colln->nLevels > 0) {
                KGrMessage::information (view, i18n ("Check Games & Levels"),
                        i18n ("There is no folder '%1' to hold levels for"
                        " the '%2' game.", d_path, colln->name));
            }
            continue;
        }

        const QFileInfoList files = d.entryInfoList
                        (QStringList (colln->prefix + "???.grl"),
                         QDir::Files, QDir::Name);

        if ((files.count() <= 0) && (colln->nLevels > 0)) {
            KGrMessage::information (view, i18n ("Check Games & Levels"),
                i18n ("There are no files '%1/%2???.grl' for the %3 game.",
                 d_path,
                 colln->prefix,
                 "\"" + colln->name + "\""));
            continue;
        }

        // If the prefix is "level", the first file is the "ENDE" screen.
        int lev = (colln->prefix == "level") ? 0 : 1;

        foreach (const QFileInfo& file, files) {
            // Get the name of the file found on disk.
            fileName1 = file.fileName();

            while (true) {
                // Work out what the file name should be, based on the level no.
                fileName2.setNum (lev);			// Convert to QString.
                fileName2 = fileName2.rightJustified (3,'0'); // Add zeros.
                fileName2.append (".grl");		// Add level-suffix.
                fileName2.prepend (colln->prefix);	// Add colln. prefix.

                if (lev > colln->nLevels) {
                    KGrMessage::information (view,
                        i18n ("Check Games & Levels"),
                        i18n ("File '%1' is beyond the highest level for "
                        "the %2 game and cannot be played.",
                         fileName1,
                         "\"" + colln->name + "\""));
                    break;
                }
                else if (fileName1 == fileName2) {
                    lev++;
                    break;
                }
                else if (fileName1.myStr() < fileName2.myStr()) {
                    KGrMessage::information (view,
                        i18n ("Check Games & Levels"),
                        i18n ("File '%1' is before the lowest level for "
                        "the %2 game and cannot be played.",
                         fileName1,
                         "\"" + colln->name + "\""));
                    break;
                }
                else {
                    KGrMessage::information (view,
                        i18n ("Check Games & Levels"),
                        i18n ("Cannot find file '%1' for the %2 game.",
                         fileName2,
                         "\"" + colln->name + "\""));
                    lev++;
                }
            }
        }
    }
}

bool KGrGame::loadCollections (Owner o)
{
    KGrGameIO io;
    QList<KGrGameData *> gList;
    QString filePath;
    IOStatus status = io.fetchGameListData
                         (getDirectory (o), gList, filePath);

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
        // OBSOLESCENT - 9/1/09
        foreach (KGrGameData * g, gList) {
            collections.append (new KGrCollection
                    (o, i18n ((const char *) g->name), // Translate now.
                        g->prefix, g->rules, g->nLevels,
                        QString::fromUtf8 ((const char *) g->about), g->skill));
        }
        result = true;
        break;
    }

    return (result);
}

bool KGrGame::saveCollections (Owner o)
{
    QString	filePath;

    if (o != USER) {
        KGrMessage::information (view, i18n ("Save Game Info"),
            i18n ("You can only modify user games."));
        return false;
    }

    filePath = userDataDir + "games.dat";

    QFile c (filePath);

    // Open the output file.
    if (! c.open (QIODevice::WriteOnly)) {
        KGrMessage::information (view, i18n ("Save Game Info"),
                i18n ("Cannot open file '%1' for output.", filePath));
        return (false);
    }

    // Save the collections.
    KGrCollection *	colln;
    QString		line;
    int			i, len;
    char		ch;

    QListIterator<KGrCollection *> it (collections);
    while (it.hasNext()) {
        colln = it.next();
        if (colln->owner == o) {
            line.sprintf ("%03d %c %s %s\n", colln->nLevels, colln->settings,
                                colln->prefix.myStr(),
                                colln->name.toUtf8().constData());
            len = line.length();
            for (i = 0; i < len; i++)
                        c.putChar (line.toUtf8()[i]);

            len = colln->about.length();
            if (len > 0) {
                QByteArray aboutC = colln->about.toUtf8();
                len = aboutC.length();		// Might be longer now.
                for (i = 0; i < len; i++) {
                    ch = aboutC[i];
                    if (ch != '\n') {
                        c.putChar (ch);		// Copy the character.
                    }
                    else {
                        c.putChar ('\\');	// Change newline to \ and n.
                        c.putChar ('n');
                    }
                }
                c.putChar ('\n');		// Add a real newline.
            }
        }
    }

    c.close();
    return (true);
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


/******************************************************************************/
/***********************    COLLECTION DATA CLASS    **************************/
/******************************************************************************/

KGrCollection::KGrCollection (Owner o, const QString & n, const QString & p,
               const char s, int nl, const QString & a, const char sk = 'N')
{
    // Holds information about a collection of KGoldrunner levels (i.e. a game).
    owner = o; name = n; prefix = p; settings = s; nLevels = nl;
    about = a; skill = sk;
}

#include "kgrgame.moc"
// vi: set sw=4 :
