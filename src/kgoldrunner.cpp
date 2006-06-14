/*
    Copyright 2002 Marco Krüger
    Copyright 2002 Ian Wadham <ianw@netspace.net.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <QPixmap>
#include <QDesktopWidget>

#include <kglobal.h>
#include <kstatusbar.h>
#include <kkeydialog.h>

#include <kconfig.h>

#include <ktoolbar.h>

#include <kaction.h>
#include <ktoggleaction.h>
#include <kstdaction.h>
#include <kstdgameaction.h>

#include "kgrconsts.h"
#include "kgrobject.h"
#include "kgrfigure.h"
#include "kgrcanvas.h"
#include "kgrdialog.h"
#include "kgrgame.h"
#include "kgoldrunner.h"

KGoldrunner::KGoldrunner()
    : KMainWindow (0, "KGoldrunner"),
      view (new KGrCanvas (this))
{
/******************************************************************************/
/*************  FIND WHERE THE GAMES DATA AND HANDBOOK SHOULD BE  *************/
/******************************************************************************/

    // Avoid "saveOK()" check if an error-exit occurs during the file checks.
    startupOK = true;

    // Get directory paths for the system levels, user levels and manual.
    if (! getDirectories()) {
	fprintf (stderr, "getDirectories() FAILED\n");
	startupOK = false;
	return;				// If games directory not found, abort.
    }

    // This message is to help diagnose distribution or installation problems.
    printf
    ("The games data and handbook should be in the following locations:\n");
    printf ("System games: %s\nUser data:    %s\nHandbook:     %s\n",
	systemDataDir.myStr(), userDataDir.myStr(), systemHTMLDir.myStr());

/******************************************************************************/
/************************  SET PLAYFIELD AND GAME DATA  ***********************/
/******************************************************************************/

    game = new KGrGame (view, systemDataDir, userDataDir);

    // Initialise the collections of levels (i.e. the list of games).
    if (! game->initCollections()) {
	startupOK = false;
	return;				// If no game files, abort.
    }

    hero = game->getHero();		// Get a pointer to the hero.

/******************************************************************************/
/*************************  SET UP THE USER INTERFACE  ************************/
/******************************************************************************/

    // Get catalog for translation
    KGlobal::locale()->insertCatalog("libkdegames");

    // Tell the KMainWindow that this is the main widget
    setCentralWidget (view);

    // Set up our actions (menu, toolbar and keystrokes) ...
    setupActions();

    // and a status bar.
    initStatusBar();

    // Connect the game actions to the menu and toolbar displays.
    connect(game, SIGNAL (setEditMenu (bool)),	SLOT (setEditMenu (bool)));
    connect(game, SIGNAL (markRuleType (char)), SLOT (markRuleType (char)));
    connect(game, SIGNAL (hintAvailable(bool)),	SLOT (adjustHintAction(bool)));
    connect(game, SIGNAL (defaultEditObj()),	SLOT (defaultEditObj()));

    // Apply the saved mainwindow settings, if any, and ask the mainwindow
    // to automatically save settings if changed: window size, toolbar
    // position, icon size, etc.
    setAutoSaveSettings();

    // explicitly hide an edit toolbar - we need it in edit mode only
    toolBar("editToolbar")->hide();
    toolBar("editToolbar")->setAllowedAreas(Qt::TopToolBarArea);
#ifdef QT3
    // Base size of playing-area and widgets on the monitor resolution.
    int dw = KApplication::desktop()->width();
    if (dw > 800) {				// More than 800x600.
	view->changeSize (+1);		// Scale 1.25:1.
    }
    if (dw > 1024) {			// More than 1024x768.
	view->changeSize (+1);
	view->changeSize (+1);		// Scale 1.75:1.
    }
    view->setBaseScale();		// Set scale for level-names.
#endif
    setFixedSize (view->size());


    // Set mouse control of the hero as the default.
    game->setMouseMode (true);

    // Paint the main widget (title, menu, status bar, blank playfield).
    show();

    // Force the main widget to appear before the "Start Game" dialog does.
    qApp->processEvents();

    // Call the "Start Game" function and pop up the "Start Game" dialog.
    game->startLevelOne();
}

KGoldrunner::~KGoldrunner()
{
    delete editToolbar;
}

void KGoldrunner::setupActions()
{
    /**************************************************************************/
    /******************************   GAME MENU  ******************************/
    /**************************************************************************/

    // New Game...
    // Load Saved Game...
    // Play Any Level...
    // Play Next Level...
    // Tutorial
    // --------------------------

    KAction * newAction =	KStdGameAction::
				gameNew (
				game,
				SLOT(startLevelOne()), actionCollection());
    newAction->			setText (i18n("&New Game..."));
    KAction * loadGame =	KStdGameAction::
				load (
				game, SLOT(loadGame()), actionCollection());
    loadGame->			setText (i18n("&Load Saved Game..."));

    KAction* playAnyAct = 	new KAction (
				i18n("&Play Any Level..."),
				actionCollection(),
				"play_any");
    connect(playAnyAct, SIGNAL(triggered(bool)), game, SLOT(startAnyLevel()));

    KAction* playNextAct =	new KAction (
				i18n("Play &Next Level..."),
				actionCollection(),
				"play_next");
    connect(playNextAct, SIGNAL(triggered(bool)), game, SLOT(startNextLevel()));

    // Save Game...
    // Save Edits... (extra copy)
    // --------------------------

    saveGame =			KStdGameAction::
				save (
				game, SLOT(saveGame()), actionCollection());
    saveGame->			setText (i18n("&Save Game..."));
    saveGame->			setShortcut (Qt::Key_S); // Alternate key.

    // Pause
    // Show High Scores
    // Get a Hint
    // Kill the Hero
    // --------------------------

    myPause =			KStdGameAction::
				pause (
				this, SLOT(stopStart()), actionCollection());
    myPause->			setShortcut (Qt::Key_Escape); // Alternate key.
    highScore =			KStdGameAction::
				highscores (
				game, SLOT(showHighScores()), actionCollection());
    hintAction =		new KAction ( KIcon("ktip"),
				i18n("&Get Hint"),
				actionCollection(),
				"get_hint");
    connect( hintAction, SIGNAL(triggered(bool)), game, SLOT(showHint()));
    killHero =			new KAction (
				i18n("&Kill Hero"),
				actionCollection(),
				"kill_hero");
    killHero->setShortcut( Qt::Key_Q );
    connect( killHero, SIGNAL(triggered(bool)), game, SLOT(herosDead()));

    // Quit
    // --------------------------

    (void)			KStdGameAction::
				quit (
				this, SLOT(close()), actionCollection());

    /**************************************************************************/
    /***************************   GAME EDITOR MENU  **************************/
    /**************************************************************************/

    // Create a Level
    // Edit Any Level...
    // Edit Next Level...
    // --------------------------

    KAction* createAct = 	new KAction ( KIcon("filenew"),
				i18n("&Create Level"),
				actionCollection(),
				"create_level");
    connect( createAct, SIGNAL(triggered(bool)), game, SLOT(createLevel()));

    KAction* editAnyAct	=	new KAction ( KIcon("fileopen"),
				i18n("&Edit Any Level..."),
				actionCollection(),
				"edit_any");
    connect( editAnyAct, SIGNAL(triggered(bool)), game, SLOT(updateLevel()));

    KAction* editNextAct =	new KAction (
				i18n("Edit &Next Level..."),
				actionCollection(),
				"edit_next");
    connect( editNextAct, SIGNAL(triggered(bool)), game, SLOT(updateNext()));

    // Save Edits...
    // Move Level...
    // Delete Level...
    // --------------------------

    saveEdits =			new KAction ( KIcon("filesave"),
				i18n("&Save Edits..."),
				actionCollection(),
				"save_edits");
    connect( saveEdits, SIGNAL(triggered(bool)), game, SLOT(saveLevelFile()));
    saveEdits->setEnabled (false);			// Nothing to save, yet.

    KAction* moveLevel =	new KAction (
				i18n("&Move Level..."),
				actionCollection(),
				"move_level");
    connect( moveLevel, SIGNAL(triggered(bool)), game, SLOT(moveLevelFile()));

    KAction* deleteLevel =	new KAction (
				i18n("&Delete Level..."),
				actionCollection(),
				"delete_level");
    connect( deleteLevel, SIGNAL(triggered(bool)), game, SLOT(deleteLevelFile()));

    // Create a Game
    // Edit Game Info...
    // --------------------------

    KAction* createGame	=	new KAction (
				i18n("Create Game..."),
				actionCollection(),
				"create_game");
    connect( createGame, SIGNAL(triggered(bool)), this, SLOT(createGame()));

    KAction* editGame =		new KAction (
				i18n("Edit Game Info..."),
				actionCollection(),
				"edit_game");
    connect( editGame, SIGNAL(triggered(bool)), this, SLOT(editGameInfo()));

    /**************************************************************************/
    /***************************   LANDSCAPES MENU  ***************************/
    /**************************************************************************/

    // Default shortcut keys are set by "kgoldrunnerui.rc".

    // Default Shift+G
    setKGoldrunner =		new KToggleAction (
				"K&Goldrunner",
				actionCollection(),
				"kgoldrunner");
    connect( setKGoldrunner, SIGNAL(triggered(bool)), this, SLOT(lsKGoldrunner()));

    // Default Shift+A
    setAppleII =		new KToggleAction (
				"&Apple II",
				actionCollection(),
				"apple_2");
    connect( setAppleII, SIGNAL(triggered(bool)), this, SLOT(lsApple2()));

    // Default Shift+I
    setIceCave =		new KToggleAction (
				i18n("&Ice Cave"),
				actionCollection(),
				"ice_cave");
    connect( setIceCave, SIGNAL(triggered(bool)), this, SLOT(lsIceCave()));

    // Default Shift+M
    setMidnight =		new KToggleAction (
				i18n("&Midnight"),
				actionCollection(),
				"midnight");
    connect( setMidnight, SIGNAL(triggered(bool)), this, SLOT(lsMidnight()));

    // Default Shift+K
    setKDEKool =		new KToggleAction (
				i18n("&KDE Kool"),
				actionCollection(),
				"kde_kool");
    connect( setKDEKool, SIGNAL(triggered(bool)), this, SLOT(lsKDEKool()));

    QActionGroup* landscapesGrp = new QActionGroup(this);
    landscapesGrp->addAction(setKGoldrunner);
    landscapesGrp->addAction(setAppleII);
    landscapesGrp->addAction(setIceCave);
    landscapesGrp->addAction(setMidnight);
    landscapesGrp->addAction(setKDEKool);
    landscapesGrp->setExclusive(true);

    setKGoldrunner->setChecked(true);

    /**************************************************************************/
    /****************************   SETTINGS MENU  ****************************/
    /**************************************************************************/

    // Mouse Controls Hero
    // Keyboard Controls Hero
    // --------------------------

    setMouse =			new KToggleAction (
				i18n("&Mouse Controls Hero"),
				actionCollection(),
				"mouse_mode");
    connect( setMouse, SIGNAL(triggered(bool)), this, SLOT(setMouseMode()));

    setKeyboard =		new KToggleAction (
				i18n("&Keyboard Controls Hero"),
				actionCollection(),
				"keyboard_mode");
    connect( setKeyboard, SIGNAL(triggered(bool)), this, SLOT(setKeyBoardMode()));

    QActionGroup* controlGrp = new QActionGroup(this);
    controlGrp->addAction(setMouse);
    controlGrp->addAction(setKeyboard);
    controlGrp->setExclusive(true);
    setMouse->setChecked(true);

    // Normal Speed
    // Beginner Speed
    // Champion Speed
    // Increase Speed
    // Decrease Speed
    // --------------------------

    KToggleAction * nSpeed =	new KToggleAction (
				i18n("Normal Speed"),
				actionCollection(),
				"normal_speed");
    connect( nSpeed, SIGNAL(triggered(bool)), this, SLOT(normalSpeed()));

    KToggleAction * bSpeed =	new KToggleAction (
				i18n("Beginner Speed"),
				actionCollection(),
				"beginner_speed");
    connect( bSpeed, SIGNAL(triggered(bool)), this, SLOT(beginSpeed()));

    KToggleAction * cSpeed =	new KToggleAction (
				i18n("Champion Speed"),
				actionCollection(),
				"champion_speed");
    connect( cSpeed, SIGNAL(triggered(bool)), this, SLOT(champSpeed()));

    KToggleAction * iSpeed =	new KToggleAction (
				i18n("Increase Speed"),
				actionCollection(),
				"increase_speed");
    iSpeed->setShortcut( Qt::Key_Plus );
    connect( iSpeed, SIGNAL(triggered(bool)), this, SLOT(incSpeed()));

    KToggleAction * dSpeed =	new KToggleAction (
				i18n("Decrease Speed"),
				actionCollection(),
				"decrease_speed");
    iSpeed->setShortcut( Qt::Key_Minus );
    connect( dSpeed, SIGNAL(triggered(bool)), this, SLOT(decSpeed()));

    QActionGroup* speedGrp = new QActionGroup(this);
    speedGrp->addAction(nSpeed);
    speedGrp->addAction(bSpeed);
    speedGrp->addAction(cSpeed);
    speedGrp->addAction(iSpeed);
    speedGrp->addAction(dSpeed);
    nSpeed->setChecked(true);

    // Traditional Rules
    // KGoldrunner Rules
    // --------------------------

    tradRules =			new KToggleAction (
				i18n("&Traditional Rules"),
				actionCollection(),
				"trad_rules");
    connect( tradRules, SIGNAL(triggered(bool)), this, SLOT(setTradRules()));

    kgrRules =			new KToggleAction (
				i18n("K&Goldrunner Rules"),
				actionCollection(),
				"kgr_rules");
    connect( kgrRules, SIGNAL(triggered(bool)), this, SLOT(setKGrRules()));

    QActionGroup* rulesGrp = new QActionGroup(this);
    rulesGrp->addAction(tradRules);
    rulesGrp->addAction(kgrRules);
    tradRules->setChecked (true);

    // Larger Playing Area
    // Smaller Playing Area
    // --------------------------

    KAction* largerArea =	new KAction (
				i18n("Larger Playing Area"),
				actionCollection(),
				"larger_area");
    connect( largerArea, SIGNAL(triggered(bool)), this, SLOT(makeLarger()));

    KAction* smallerArea =	new KAction (
				i18n("Smaller Playing Area"),
				actionCollection(),
				"smaller_area");
    connect( smallerArea, SIGNAL(triggered(bool)), this, SLOT(makeSmaller()));

    // Configure Shortcuts...
    // Configure Toolbars...
    // --------------------------

    KStdAction::keyBindings (
				this, SLOT(optionsConfigureKeys()),
				actionCollection());
    // KStdAction::configureToolbars (
				// this, SLOT(optionsConfigureToolbars()),
				// actionCollection());

    /**************************************************************************/
    /**************************   KEYSTROKE ACTIONS  **************************/
    /**************************************************************************/

    // Two-handed KB controls and alternate one-handed controls for the hero.

    KAction* moveUp = new KAction (i18n("Move Up"), actionCollection(), "move_up");
    moveUp->setShortcut( Qt::Key_Up );
    connect( moveUp, SIGNAL(triggered(bool)), this, SLOT(goUp()));

    KAction* moveRight = new KAction (i18n("Move Right"), actionCollection(), "move_right");
    moveRight->setShortcut( Qt::Key_Right );
    connect( moveRight, SIGNAL(triggered(bool)), this, SLOT(goR()));

    KAction* moveDown = new KAction (i18n("Move Down"), actionCollection(), "move_down");
    moveDown->setShortcut( Qt::Key_Down );
    connect( moveDown, SIGNAL(triggered(bool)), this, SLOT(goDown()));

    KAction* moveLeft = new KAction (i18n("Move Left"), actionCollection(), "move_left");
    moveLeft->setShortcut( Qt::Key_Left );
    connect( moveLeft, SIGNAL(triggered(bool)), this, SLOT(goL()));

    KAction* stop = new KAction (i18n("Stop"), actionCollection(), "stop");
    stop->setShortcut( Qt::Key_Space );
    connect( stop, SIGNAL(triggered(bool)), this, SLOT(stop()));

    KAction* digRight = new KAction (i18n("Dig Right"), actionCollection(), "dig_right");
    digRight->setShortcut( Qt::Key_C );
    connect( digRight, SIGNAL(triggered(bool)), this, SLOT(digR()));

    KAction* digLeft = new KAction (i18n("Dig Left"), actionCollection(), "dig_left");
    digLeft->setShortcut( Qt::Key_Z );
    connect( digLeft, SIGNAL(triggered(bool)), this, SLOT(digL()));

    setupEditToolbarActions();			// Uses pixmaps from "view".

    // Alternate one-handed controls.  Set up in "kgoldrunnerui.rc".

    // Key_I, "move_up"
    // Key_L, "move_right"
    // Key_K, "move_down"
    // Key_J, "move_left"
    // Key_Space, "stop" (as above)
    // Key_O, "dig_right"
    // Key_U, "dig_left"

#ifdef KGR_DEBUG
    // Authors' debugging aids.

    KAction* step = new KAction (i18n("Step"), actionCollection(), "do_step");
    step->setShortcut( Qt::Key_Period );
    connect( step, SIGNAL(triggered(bool)), game, SLOT(doStep()) );

    KAction* bugFix = new KAction (i18n("Test Bug Fix"), actionCollection(), "bug_fix");
    bugFix->setShortcut( Qt::Key_B );
    connect( bugFix, SIGNAL(triggered(bool)), game, SLOT(bugFix()) );

    KAction* showPos = new KAction (i18n("Show Positions"), actionCollection(), "step");
    showPos->setShortcut( Qt::Key_D );
    connect( showPos, SIGNAL(triggered(bool)), game, SLOT(showFigurePositions()) );

    KAction* startLog = new KAction (i18n("Start Logging"), actionCollection(), "logging");
    startLog->setShortcut( Qt::Key_G );
    connect( startLog, SIGNAL(triggered(bool)), game, SLOT(startLogging()) );

    KAction* showHero = new KAction (i18n("Show Hero"), actionCollection(), "show_hero");
    showHero->setShortcut( Qt::Key_H );
    connect( showHero, SIGNAL(triggered(bool)), game, SLOT(showHeroState()) );

    KAction* showObj = new KAction (i18n("Show Object"), actionCollection(), "show_obj");
    showObj->setShortcut( Qt::Key_Question );
    connect( showObj, SIGNAL(triggered(bool)), game, SLOT(showObjectState()) );

    KAction* showEnemy0 = new KAction (i18n("Show Enemy") + "0", actionCollection(), "show_enemy_0");
    showEnemy0->setShortcut( Qt::Key_0 );
    connect( showEnemy0, SIGNAL(triggered(bool)), game, SLOT(showEnemy0()) );

    KAction* showEnemy1 = new KAction (i18n("Show Enemy") + "1", actionCollection(), "show_enemy_1");
    showEnemy1->setShortcut( Qt::Key_1 );
    connect( showEnemy1, SIGNAL(triggered(bool)), game, SLOT(showEnemy1()) );

    KAction* showEnemy2 = new KAction (i18n("Show Enemy") + "2", actionCollection(), "show_enemy_2");
    showEnemy2->setShortcut( Qt::Key_2 );
    connect( showEnemy2, SIGNAL(triggered(bool)), game, SLOT(showEnemy2()) );

    KAction* showEnemy3 = new KAction (i18n("Show Enemy") + "3", actionCollection(), "show_enemy_3");
    showEnemy3->setShortcut( Qt::Key_3 );
    connect( showEnemy3, SIGNAL(triggered(bool)), game, SLOT(showEnemy3()) );

    KAction* showEnemy4 = new KAction (i18n("Show Enemy") + "4", actionCollection(), "show_enemy_4");
    showEnemy4->setShortcut( Qt::Key_4 );
    connect( showEnemy4, SIGNAL(triggered(bool)), game, SLOT(showEnemy4()) );

    KAction* showEnemy5 = new KAction (i18n("Show Enemy") + "5", actionCollection(), "show_enemy_5");
    showEnemy5->setShortcut( Qt::Key_5 );
    connect( showEnemy5, SIGNAL(triggered(bool)), game, SLOT(showEnemy5()) );

    KAction* showEnemy6 = new KAction (i18n("Show Enemy") + "6", actionCollection(), "show_enemy_6");
    showEnemy6->setShortcut( Qt::Key_6 );
    connect( showEnemy6, SIGNAL(triggered(bool)), game, SLOT(showEnemy6()) );

#endif

    /**************************************************************************/
    /**************************   NOW SET IT ALL UP  **************************/
    /**************************************************************************/

    createGUI();
}

/******************************************************************************/
/**********************  SLOTS FOR STATUS BAR UPDATES  ************************/
/******************************************************************************/

void KGoldrunner::initStatusBar()
{
    QString s = statusBar()->fontInfo().family();	// Set bold font.
    int i = statusBar()->fontInfo().pointSize();
    statusBar()->setFont (QFont (s, i, QFont::Bold));

    statusBar()->setSizeGripEnabled (false);		// Use Settings menu ...

    statusBar()->insertItem ("", ID_LIVES);
    statusBar()->insertItem ("", ID_SCORE);
    statusBar()->insertItem ("", ID_LEVEL);
    statusBar()->insertItem ("", ID_HINTAVL);
    statusBar()->insertItem ("", ID_MSG, QSizePolicy::Horizontally);

    showLives (5);					// Start with 5 lives.
    showScore (0);
    showLevel (0);
    adjustHintAction (false);

    // Set the PAUSE/RESUME key-names into the status bar message.
    pauseKeys = myPause->shortcut().toString();
    pauseKeys = pauseKeys.replace (';', "\" " + i18n("or") + " \"");
    gameFreeze (false);

    statusBar()->setItemFixed (ID_LIVES, -1);		// Fix current sizes.
    statusBar()->setItemFixed (ID_SCORE, -1);
    statusBar()->setItemFixed (ID_LEVEL, -1);

    connect(game, SIGNAL (showLives (long)),	SLOT (showLives (long)));
    connect(game, SIGNAL (showScore (long)),	SLOT (showScore (long)));
    connect(game, SIGNAL (showLevel (int)),	SLOT (showLevel (int)));
    connect(game, SIGNAL (gameFreeze (bool)),	SLOT (gameFreeze (bool)));
}

void KGoldrunner::showLives (long newLives)
{
    QString tmp;
    tmp.setNum (newLives);
    if (newLives < 100)
	tmp = tmp.rightJustified (3, '0');
    tmp.insert (0, i18n("   Lives: "));
    tmp.append ("   ");
    statusBar()->changeItem (tmp, ID_LIVES);
}

void KGoldrunner::showScore (long newScore)
{
    QString tmp;
    tmp.setNum (newScore);
    if (newScore < 10000)
	tmp = tmp.rightJustified (5, '0');
    tmp.insert (0, i18n("   Score: "));
    tmp.append ("   ");
    statusBar()->changeItem (tmp, ID_SCORE);
}

void KGoldrunner::showLevel (int newLevelNo)
{
    QString tmp;
    tmp.setNum (newLevelNo);
    if (newLevelNo < 100)
	tmp = tmp.rightJustified (3, '0');
    tmp.insert (0, i18n("   Level: "));
    tmp.append ("   ");
    statusBar()->changeItem (tmp, ID_LEVEL);
}

void KGoldrunner::gameFreeze (bool on_off)
{
    if (on_off)
	statusBar()->changeItem
		    (i18n("Press \"%1\" to RESUME", pauseKeys), ID_MSG);
    else
	statusBar()->changeItem
		    (i18n("Press \"%1\" to PAUSE", pauseKeys), ID_MSG);
}

void KGoldrunner::adjustHintAction (bool hintAvailable)
{
    hintAction->setEnabled (hintAvailable);

    if (hintAvailable) {
	statusBar()->changeItem (i18n("   Has hint   "), ID_HINTAVL);
    }
    else {
	statusBar()->changeItem (i18n("   No hint   "), ID_HINTAVL);
    }
}

void KGoldrunner::markRuleType (char ruleType)
{
    if (ruleType == 'T')
	tradRules->trigger();
    else
	kgrRules->trigger();
}

void KGoldrunner::setEditMenu (bool on_off)
{
    saveEdits->setEnabled  (on_off);

    saveGame->setEnabled   (! on_off);
    hintAction->setEnabled (! on_off);
    killHero->setEnabled   (! on_off);
    highScore->setEnabled  (! on_off);

    if (on_off){
	toolBar("editToolbar")->show();
    }
    else {
	toolBar("editToolbar")->hide();
    }
}

/******************************************************************************/
/*******************   SLOTS FOR MENU AND KEYBOARD ACTIONS  *******************/
/******************************************************************************/

// Slot to halt (pause) or restart the game play.

void KGoldrunner::stopStart()
{
    if (! (KGrObject::frozen)) {
	game->freeze();
    }
    else {
	game->unfreeze();
    }
}

// Local slots to create or edit game information.

void KGoldrunner::createGame()		{game->editCollection (SL_CR_GAME);}
void KGoldrunner::editGameInfo()	{game->editCollection (SL_UPD_GAME);}

// Local slots to set the landscape (colour scheme).

void KGoldrunner::lsKGoldrunner()	{view->changeLandscape ("KGoldrunner");}
void KGoldrunner::lsApple2()		{view->changeLandscape ("Apple II");}
void KGoldrunner::lsIceCave()		{view->changeLandscape ("Ice Cave");}
void KGoldrunner::lsMidnight()		{view->changeLandscape ("Midnight");}
void KGoldrunner::lsKDEKool()		{view->changeLandscape ("KDE Kool");}

// Local slots to set mouse or keyboard control of the hero.

void KGoldrunner::setMouseMode()	{game->setMouseMode (true);}
void KGoldrunner::setKeyBoardMode()	{game->setMouseMode (false);}

// Local slots to set game speed.

void KGoldrunner::normalSpeed()		{hero->setSpeed (NSPEED);}
void KGoldrunner::beginSpeed()		{hero->setSpeed (BEGINSPEED);}
void KGoldrunner::champSpeed()		{hero->setSpeed (CHAMPSPEED);}
void KGoldrunner::incSpeed()		{hero->setSpeed (+1);}
void KGoldrunner::decSpeed()		{hero->setSpeed (-1);}

// Slots to set Traditional or KGoldrunner rules.

void KGoldrunner::setTradRules()
{
    KGrFigure::variableTiming = true;
    KGrFigure::alwaysCollectNugget = true;
    KGrFigure::runThruHole = true;
    KGrFigure::reappearAtTop = true;
    KGrFigure::searchStrategy = LOW;
}

void KGoldrunner::setKGrRules()
{
    KGrFigure::variableTiming = false;
    KGrFigure::alwaysCollectNugget = false;
    KGrFigure::runThruHole = false;
    KGrFigure::reappearAtTop = false;
    KGrFigure::searchStrategy = MEDIUM;
}

// Local slots to make playing area larger or smaller.

void KGoldrunner::makeLarger()
{
    if (view->changeSize (+1))
	setFixedSize (view->size());
}

void KGoldrunner::makeSmaller()
{
    if (view->changeSize (-1))
	setFixedSize (view->size());
}

// Local slots for hero control keys.

void KGoldrunner::goUp()		{setKey (KB_UP);}
void KGoldrunner::goR()			{setKey (KB_RIGHT);}
void KGoldrunner::goDown()		{setKey (KB_DOWN);}
void KGoldrunner::goL()			{setKey (KB_LEFT);}
void KGoldrunner::stop()		{setKey (KB_STOP);}
void KGoldrunner::digR()		{setKey (KB_DIGRIGHT);}
void KGoldrunner::digL()		{setKey (KB_DIGLEFT);}

// Local slots for authors' debugging aids.

void KGoldrunner::showEnemy0()		{game->showEnemyState (0);}
void KGoldrunner::showEnemy1()		{game->showEnemyState (1);}
void KGoldrunner::showEnemy2()		{game->showEnemyState (2);}
void KGoldrunner::showEnemy3()		{game->showEnemyState (3);}
void KGoldrunner::showEnemy4()		{game->showEnemyState (4);}
void KGoldrunner::showEnemy5()		{game->showEnemyState (5);}
void KGoldrunner::showEnemy6()		{game->showEnemyState (6);}

void KGoldrunner::saveProperties(KConfig *config)
{
    // The 'config' object points to the session managed
    // config file.  Anything you write here will be available
    // later when this app is restored.

    config->setGroup ("Game");		// Prevents a compiler warning.
    printf ("I am in KGoldrunner::saveProperties.\n");
    // config->writeEntry("qqq", qqq);
}

void KGoldrunner::readProperties(KConfig *config)
{
    // The 'config' object points to the session managed
    // config file.  This function is automatically called whenever
    // the app is being restored.  Read in here whatever you wrote
    // in 'saveProperties'

    config->setGroup ("Game");		// Prevents a compiler warning.
    printf ("I am in KGoldrunner::readProperties.\n");
    // QString qqq = config->readEntry("qqq");
}

void KGoldrunner::optionsShowToolbar()
{
    // this is all very cut and paste code for showing/hiding the
    // toolbar
    // if (m_toolbarAction->isChecked())
        // toolBar()->show();
    // else
        // toolBar()->hide();
}

void KGoldrunner::optionsShowStatusbar()
{
    // this is all very cut and paste code for showing/hiding the
    // statusbar
    // if (m_statusbarAction->isChecked())
        // statusBar()->show();
    // else
        // statusBar()->hide();
}

void KGoldrunner::optionsConfigureKeys()
{
    KKeyDialog::configure(actionCollection());

    // Update the PAUSE/RESUME message in the status bar.
    pauseKeys = myPause->shortcut().toString();
    pauseKeys = pauseKeys.replace (';', "\" " + i18n("or") + " \"");
    gameFreeze (KGrObject::frozen);	// Refresh the status bar text.
}

void KGoldrunner::optionsConfigureToolbars()
{
    // use the standard toolbar editor
#if defined(KDE_MAKE_VERSION)
# if KDE_VERSION >= KDE_MAKE_VERSION(3,1,0)
    saveMainWindowSettings(KGlobal::config(), autoSaveGroup());
# else
    saveMainWindowSettings(KGlobal::config());
# endif
#else
    saveMainWindowSettings(KGlobal::config());
#endif
}

void KGoldrunner::newToolbarConfig()
{
    // this slot is called when user clicks "Ok" or "Apply" in the toolbar editor.
    // recreate our GUI, and re-apply the settings (e.g. "text under icons", etc.)
    createGUI();

#if defined(KDE_MAKE_VERSION)
# if KDE_VERSION >= KDE_MAKE_VERSION(3,1,0)
    applyMainWindowSettings(KGlobal::config(), autoSaveGroup());
# else
    applyMainWindowSettings(KGlobal::config());
# endif
#else
    applyMainWindowSettings(KGlobal::config());
#endif
}

void KGoldrunner::optionsPreferences()
{
    // popup some sort of preference dialog, here
    // KGoldrunnerPreferences dlg;
    // if (dlg.exec())
    // {
        // redo your settings
    // }
}

void KGoldrunner::changeStatusbar(const QString& text)
{
    // display the text on the statusbar
    statusBar()->showMessage(text);
}

void KGoldrunner::changeCaption(const QString& text)
{
    // display the text on the caption
    setCaption(text);
}

bool KGoldrunner::getDirectories()
{
    bool result = true;

    // WHERE THINGS ARE: In the KDE 3 environment (Release 3.1.1), application
    // documentation and data files are in a directory structure given by
    // $KDEDIRS (e.g. "/usr/local/kde" or "/opt/kde3/").  Application user data
    // files are in a directory structure given by $KDEHOME ("$HOME/.kde").
    // Within those two structures, the three sub-directories will typically be
    // "share/doc/HTML/en/kgoldrunner/", "share/apps/kgoldrunner/system/" and
    // "share/apps/kgoldrunner/user/".  Note that it is necessary to have
    // an extra path level ("system" or "user") after "kgoldrunner", otherwise
    // all the KGoldrunner files have similar path names (after "apps") and
    // KDE always locates directories in $KDEHOME and never the released games.

    // The directory strings are set by KDE at run time and might change in
    // later releases, so use them with caution and only if something gets lost.

    KStandardDirs * dirs = new KStandardDirs();

#ifdef QT3
    QString myDir = "kgoldrunner";
#else
    QString myDir = "kgoldrun";
#endif

    // Find the KGoldrunner Users' Guide, English version (en).
    systemHTMLDir = dirs->findResourceDir ("html", "en/" + myDir + "/");
    if (systemHTMLDir.length() <= 0) {
	KGrMessage::information (this, i18n("Get Folders"),
	i18n("Cannot find documentation sub-folder 'en/%1/' "
	"in area '%2' of the KDE folder ($KDEDIRS).",
	 myDir, dirs->kde_default ("html")));
	// result = false;		// Don't abort if the doc is missing.
    }
    else
	systemHTMLDir.append ("en/" + myDir + "/");

    // Find the system collections in a directory of the required KDE type.
    systemDataDir = dirs->findResourceDir ("data", myDir + "/system/");
    if (systemDataDir.length() <= 0) {
	KGrMessage::information (this, i18n("Get Folders"),
	i18n("Cannot find system games sub-folder '%1/system/' "
	"in area '%2' of the KDE folder ($KDEDIRS).",
	 myDir, dirs->kde_default ("data")));
	result = false;			// ABORT if the games data is missing.
    }
    else
	systemDataDir.append (myDir + "/system/");

    // Locate and optionally create directories for user collections and levels.
    bool create = true;
    userDataDir   = dirs->saveLocation ("data", myDir + "/user/", create);
    if (userDataDir.length() <= 0) {
	KGrMessage::information (this, i18n("Get Folders"),
	i18n("Cannot find or create user games sub-folder '%1/user/' "
	"in area '%2' of the KDE user area ($KDEHOME).",
	 myDir, dirs->kde_default ("data")));
	// result = false;		// Don't abort if user area is missing.
    }
    else {
	create = dirs->makeDir (userDataDir + "levels/");
	if (! create) {
	    KGrMessage::information (this, i18n("Get Folders"),
	    i18n("Cannot find or create 'levels/' folder in "
	    "sub-folder '%1/user/' in the KDE user area ($KDEHOME).", myDir));
	    // result = false;		// Don't abort if user area is missing.
	}
    }

    return (result);
}

// This method is invoked when top-level window is closed, whether by selecting
// "Quit" from the menu or by clicking the "X" at the top right of the window.

bool KGoldrunner::queryClose ()
{
    // Last chance to save: user has clicked "X" widget or menu-Quit.
    bool cannotContinue = true;
    game->saveOK (cannotContinue);
    return (true);
}

void KGoldrunner::setKey (KBAction movement)
{
    if (game->inEditMode()) return;

    // Using keyboard control can automatically disable mouse control.
    if (game->inMouseMode()) {
        // Halt the game while a message is displayed.
        game->setMessageFreeze (true);

        switch (KGrMessage::warning (this, i18n("Switch to Keyboard Mode"),
		i18n("You have pressed a key that can be used to move the "
		"Hero. Do you want to switch automatically to keyboard "
		"control? Mouse control is easier to use in the long term "
		"- like riding a bike rather than walking!"),
		i18n("Switch to &Keyboard Mode"), i18n("Stay in &Mouse Mode")))
        {
        case 0: game->setMouseMode (false);	// Set internal mouse mode OFF.
		setMouse->setChecked (false);	// Adjust the Settings menu.
		setKeyboard->setChecked (true);
                break;
        case 1: break;
        }

        // Unfreeze the game, but only if it was previously unfrozen.
        game->setMessageFreeze (false);

        if (game->inMouseMode())
            return;                    		// Stay in Mouse Mode.
    }

    if ( game->getLevel() != 0 )
    {
      if (! hero->started )			// Start when first movement
          game->startPlaying();			// key is pressed ...
      game->heroAction (movement);
    }
}

/******************************************************************************/
/**********************  MAKE A TOOLBAR FOR THE EDITOR   **********************/
/******************************************************************************/

#include <qmatrix.h>
void KGoldrunner::setupEditToolbarActions()
{
    // Set up the pixmaps for the editable objects.
    QPixmap pixmap;
    QImage image;

    QPixmap brickbg	= view->getPixmap (BRICK);
    QPixmap fbrickbg	= view->getPixmap (FBRICK);

    QPixmap freebg	= view->getPixmap (FREE);
    QPixmap nuggetbg	= view->getPixmap (NUGGET);
    QPixmap polebg	= view->getPixmap (POLE);
    QPixmap betonbg	= view->getPixmap (BETON);
    QPixmap ladderbg	= view->getPixmap (LADDER);
    QPixmap hladderbg	= view->getPixmap (HLADDER);
    QPixmap edherobg	= view->getPixmap (HERO);
    QPixmap edenemybg	= view->getPixmap (ENEMY);

    // Scale up the pixmaps (to give cleaner looking
    // icons than leaving it for QToolButton to do).
    QMatrix w;
    w = w.scale (2.0, 2.0);

#warning "How to adjust this hack?"

    brickbg		= brickbg.transformed (w);
    fbrickbg		= fbrickbg.transformed (w);

    freebg		= freebg.transformed (w);
    nuggetbg		= nuggetbg.transformed (w);
    polebg		= polebg.transformed (w);
    betonbg		= betonbg.transformed (w);
    ladderbg		= ladderbg.transformed (w);
    hladderbg		= hladderbg.transformed (w);
    edherobg		= edherobg.transformed (w);
    edenemybg		= edenemybg.transformed (w);

    // Choose a colour that enhances visibility of the KGoldrunner pixmaps.
    // editToolbar->setPalette (QPalette (QColor (150, 150, 230)));

    KAction* ktipAct = new KAction( KIcon("ktip"), i18n("Edit Name/Hint"), actionCollection(), "edit_hint" );
    connect( ktipAct, SIGNAL(triggered(bool)), game, SLOT(editNameAndHint()) );

    KToggleAction* freebgAct = new KToggleAction( KIcon(freebg), i18n("Empty space"), actionCollection(), "freebg" );
    connect( freebgAct, SIGNAL(triggered(bool)), this, SLOT(freeSlot()) );

    KToggleAction* edherobgAct = new KToggleAction( KIcon(edherobg), i18n("Hero"), actionCollection(), "edherobg" );
    connect( edherobgAct, SIGNAL(triggered(bool)), this, SLOT(edheroSlot()) );

    KToggleAction* edenemybgAct = new KToggleAction( KIcon(edenemybg), i18n("Enemy"), actionCollection(), "edenemybg" );
    connect( edenemybgAct, SIGNAL(triggered(bool)), this, SLOT(edenemySlot()) );

    KToggleAction* brickbgAct = new KToggleAction( KIcon(brickbg), i18n("Brick (can dig)"), actionCollection(), "brickbg" );
    connect( brickbgAct, SIGNAL(triggered(bool)), this, SLOT(brickSlot()) );

    KToggleAction* betonbgAct = new KToggleAction( KIcon(betonbg), i18n("Concrete (cannot dig)"), actionCollection(), "betonbg" );
    connect( betonbgAct, SIGNAL(triggered(bool)), this, SLOT(betonSlot()) );

    KToggleAction* fbrickbgAct = new KToggleAction( KIcon(fbrickbg), i18n("Trap (can fall through)"), actionCollection(), "fbrickbg" );
    connect( fbrickbgAct, SIGNAL(triggered(bool)), this, SLOT(fbrickSlot()) );

    KToggleAction* ladderbgAct = new KToggleAction( KIcon(ladderbg), i18n("Ladder"), actionCollection(), "ladderbg" );
    connect( ladderbgAct, SIGNAL(triggered(bool)), this, SLOT(ladderSlot()) );

    KToggleAction* hladderbgAct = new KToggleAction( KIcon(hladderbg), i18n("Hidden ladder"), actionCollection(), "hladderbg" );
    connect( hladderbgAct, SIGNAL(triggered(bool)), this, SLOT(hladderSlot()) );

    KToggleAction* polebgAct = new KToggleAction( KIcon(polebg), i18n("Pole (or bar)"), actionCollection(), "polebg" );
    connect( polebgAct, SIGNAL(triggered(bool)), this, SLOT(poleSlot()) );

    KToggleAction* nuggetbgAct = new KToggleAction( KIcon(nuggetbg), i18n("Gold nugget"), actionCollection(), "nuggetbg" );
    connect( nuggetbgAct, SIGNAL(triggered(bool)), this, SLOT(nuggetSlot()) );

    QActionGroup* editButtons = new QActionGroup(this);
    editButtons->setExclusive(true);
    editButtons->addAction( freebgAct );
    editButtons->addAction( edherobgAct );
    editButtons->addAction( edenemybgAct );
    editButtons->addAction( brickbgAct );
    editButtons->addAction( betonbgAct );
    editButtons->addAction( fbrickbgAct );
    editButtons->addAction( ladderbgAct );
    editButtons->addAction( hladderbgAct );
    editButtons->addAction( polebgAct );
    editButtons->addAction( nuggetbgAct );

    brickbgAct->setChecked(true);
    m_defaultEditAct = brickbgAct;
}

/******************************************************************************/
/*********************   EDIT-BUTTON SLOTS   **********************************/
/******************************************************************************/

void KGoldrunner::freeSlot()
		{ game->setEditObj (FREE);     }
void KGoldrunner::edheroSlot()
		{ game->setEditObj (HERO);     }
void KGoldrunner::edenemySlot()
		{ game->setEditObj (ENEMY);    }
void KGoldrunner::brickSlot()
		{ game->setEditObj (BRICK);    }
void KGoldrunner::betonSlot()
		{ game->setEditObj (BETON);    }
void KGoldrunner::fbrickSlot()
		{ game->setEditObj (FBRICK);   }
void KGoldrunner::ladderSlot()
		{ game->setEditObj (LADDER);   }
void KGoldrunner::hladderSlot()
		{ game->setEditObj (HLADDER);  }
void KGoldrunner::poleSlot()
		{ game->setEditObj (POLE);     }
void KGoldrunner::nuggetSlot()
		{ game->setEditObj (NUGGET);   }
void KGoldrunner::defaultEditObj()
		{ m_defaultEditAct->setChecked(true); }

#include "kgoldrunner.moc"
