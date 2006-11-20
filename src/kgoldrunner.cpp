/*
 * Copyright (C) 2003 Ian Wadham and Marco Krüger <ianw2@optusnet.com.au>
 */

#include <kprinter.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>

#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <kkeydialog.h>
#include <kaccel.h>
#include <kio/netaccess.h>
#include <kfiledialog.h>
#include <kconfig.h>

#include <kedittoolbar.h>
#include <ktoolbar.h>

#include <kstdaccel.h>
#include <kaction.h>
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
    startupOK = TRUE;

    // Get directory paths for the system levels, user levels and manual.
    if (! getDirectories()) {
	fprintf (stderr, "getDirectories() FAILED\n");
	startupOK = FALSE;
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
	startupOK = FALSE;
	return;				// If no game files, abort.
    }

    hero = game->getHero();		// Get a pointer to the hero.

/******************************************************************************/
/*************************  SET UP THE USER INTERFACE  ************************/
/******************************************************************************/

    // Get catalog for translation
    KGlobal::locale()->insertCatalogue("libkdegames");

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

#ifdef QT3
    // Base size of playing-area and widgets on the monitor resolution.
    int dw = KApplication::desktop()->width();
    if (dw > 800) {				// More than 800x600.
	view->changeSize (+1);		// Scale 1.25:1.
    }
    if (dw > 1024) {			// More than 1024x768.
	view->changeSize (+1);
	view->changeSize (+1);		// Scale 1.75:1.
	setUsesBigPixmaps (TRUE);	// Use big toolbar buttons.
    }
    view->setBaseScale();		// Set scale for level-names.
#endif
    setFixedSize (view->size());

    makeEditToolbar();			// Uses pixmaps from "view".
    editToolbar->hide();
    setDockEnabled (DockBottom, FALSE);
    setDockEnabled (DockLeft, FALSE);
    setDockEnabled (DockRight, FALSE);

    // Make it impossible to turn off the editor toolbar.
    // Accidentally hiding it would make editing impossible.
    setDockMenuEnabled (FALSE);

    // Set mouse control of the hero as the default.
    game->setMouseMode (TRUE);

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
    (void)			new KAction (
				i18n("&Play Any Level..."),
				0,
				game, SLOT(startAnyLevel()), actionCollection(),
				"play_any");
    (void)			new KAction (
				i18n("Play &Next Level..."),
				0,
				game,
				SLOT(startNextLevel()), actionCollection(),
				"play_next");

    // Save Game...
    // Save Edits... (extra copy)
    // --------------------------

    saveGame =			KStdGameAction::
				save (
				game, SLOT(saveGame()), actionCollection());
    saveGame->			setText (i18n("&Save Game..."));
    saveGame->			setShortcut (Key_S); // Alternate key.

    // Pause
    // Show High Scores
    // Get a Hint
    // Kill the Hero
    // --------------------------

    myPause =			KStdGameAction::
				pause (
				this, SLOT(stopStart()), actionCollection());
    myPause->			setShortcut (Key_Escape); // Alternate key.
    highScore =			KStdGameAction::
				highscores (
				game, SLOT(showHighScores()), actionCollection());
    hintAction =		new KAction (
				i18n("&Get Hint"), "ktip",
				0,
				game, SLOT(showHint()), actionCollection(),
				"get_hint");
    killHero =			new KAction (
				i18n("&Kill Hero"),
				Key_Q,
				game, SLOT(herosDead()), actionCollection(),
				"kill_hero");

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

    (void)			new KAction (
				i18n("&Create Level"),
				0,
				game, SLOT(createLevel()), actionCollection(),
				"create");
    (void)			new KAction (
				i18n("&Edit Any Level..."),
				0,
				game, SLOT(updateLevel()), actionCollection(),
				"edit_any");
    (void)			new KAction (
				i18n("Edit &Next Level..."),
				0,
				game, SLOT(updateNext()), actionCollection(),
				"edit_next");

    // Save Edits...
    // Move Level...
    // Delete Level...
    // --------------------------

    saveEdits =			new KAction (
				i18n("&Save Edits..."),
				0,
				game, SLOT(saveLevelFile()), actionCollection(),
				"save_edits");
    saveEdits->setEnabled (FALSE);			// Nothing to save, yet.

    (void)			new KAction (
				i18n("&Move Level..."),
				0,
				game, SLOT(moveLevelFile()), actionCollection(),
				"move_level");
    (void)			new KAction (
				i18n("&Delete Level..."),
				0,
				game,
				SLOT(deleteLevelFile()), actionCollection(),
				"delete_level");

    // Create a Game
    // Edit Game Info...
    // --------------------------

    (void)			new KAction (
				i18n("Create Game..."),
				0,
				this, SLOT(createGame()), actionCollection(),
				"create_game");
    (void)			new KAction (
				i18n("Edit Game Info..."),
				0,
				this,
				SLOT(editGameInfo()), actionCollection(),
				"edit_game");

    /**************************************************************************/
    /***************************   LANDSCAPES MENU  ***************************/
    /**************************************************************************/

    // Default shortcut keys are set by "kgoldrunnerui.rc".

    setKGoldrunner =		new KRadioAction (
				"K&Goldrunner",
				0,			// Default Shift+G
				this, SLOT(lsKGoldrunner()), actionCollection(),
				"kgoldrunner");
    setAppleII =		new KRadioAction (
				"&Apple II",
				0,			// Default Shift+A
				this, SLOT(lsApple2()), actionCollection(),
				"apple_2");
    setIceCave =		new KRadioAction (
				i18n("&Ice Cave"),
				0,			// Default Shift+I
				this, SLOT(lsIceCave()), actionCollection(),
				"ice_cave");
    setMidnight =		new KRadioAction (
				i18n("&Midnight"),
				0,			// Default Shift+M
				this, SLOT(lsMidnight()), actionCollection(),
				"midnight");
    setKDEKool =		new KRadioAction (
				i18n("&KDE Kool"),
				0,			// Default Shift+K
				this, SLOT(lsKDEKool()), actionCollection(),
				"kde_kool");

    setKGoldrunner->		setExclusiveGroup ("landscapes");
    setAppleII->		setExclusiveGroup ("landscapes");
    setIceCave->		setExclusiveGroup ("landscapes");
    setMidnight->		setExclusiveGroup ("landscapes");
    setKDEKool->		setExclusiveGroup ("landscapes");
    setKGoldrunner->		setChecked (TRUE);

    /**************************************************************************/
    /****************************   SETTINGS MENU  ****************************/
    /**************************************************************************/

    // Mouse Controls Hero
    // Keyboard Controls Hero
    // --------------------------

    setMouse =			new KRadioAction (
				i18n("&Mouse Controls Hero"),
				0,
				this,
				SLOT(setMouseMode()), actionCollection(),
				"mouse_mode");
    setKeyboard =		new KRadioAction (
				i18n("&Keyboard Controls Hero"),
				0,
				this,
				SLOT(setKeyBoardMode()), actionCollection(),
				"keyboard_mode");

    setMouse->			setExclusiveGroup ("control");
    setKeyboard->		setExclusiveGroup ("control");
    setMouse->			setChecked (TRUE);

    // Normal Speed
    // Beginner Speed
    // Champion Speed
    // Increase Speed
    // Decrease Speed
    // --------------------------

    KRadioAction * nSpeed =	new KRadioAction (
				i18n("Normal Speed"),
				0,
				this, SLOT(normalSpeed()), actionCollection(),
				"normal_speed");
    KRadioAction * bSpeed =	new KRadioAction (
				i18n("Beginner Speed"),
				0,
				this, SLOT(beginSpeed()), actionCollection(),
				"beginner_speed");
    KRadioAction * cSpeed =	new KRadioAction (
				i18n("Champion Speed"),
				0,
				this, SLOT(champSpeed()), actionCollection(),
				"champion_speed");
    (void)			new KAction (		// Repeatable action.
				i18n("Increase Speed"),
				Key_Plus,
				this, SLOT(incSpeed()), actionCollection(),
				"increase_speed");
    (void)			new KAction (		// Repeatable action.
				i18n("Decrease Speed"),
				Key_Minus,
				this, SLOT(decSpeed()), actionCollection(),
				"decrease_speed");

    nSpeed->			setExclusiveGroup ("speed");
    bSpeed->			setExclusiveGroup ("speed");
    cSpeed->			setExclusiveGroup ("speed");
    nSpeed->			setChecked (TRUE);

    // Traditional Rules
    // KGoldrunner Rules
    // --------------------------

    tradRules =			new KRadioAction (
				i18n("&Traditional Rules"),
				0,
				this, SLOT(setTradRules()), actionCollection(),
				"trad_rules");
    kgrRules =			new KRadioAction (
				i18n("K&Goldrunner Rules"),
				0,
				this, SLOT(setKGrRules()), actionCollection(),
				"kgr_rules");

    tradRules->			setExclusiveGroup ("rules");
    kgrRules->			setExclusiveGroup ("rules");
    tradRules->			setChecked (TRUE);

    // Larger Playing Area
    // Smaller Playing Area
    // --------------------------

    (void)			new KAction (
				i18n("Larger Playing Area"),
				0,
				this, SLOT(makeLarger()), actionCollection(),
				"larger_area");
    (void)			new KAction (
				i18n("Smaller Playing Area"),
				0,
				this, SLOT(makeSmaller()), actionCollection(),
				"smaller_area");

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

    (void)	new KAction (i18n("Move Up"), Key_Up,
		this, SLOT(goUp()), actionCollection(), "move_up");
    (void)	new KAction (i18n("Move Right"), Key_Right,
		this, SLOT(goR()), actionCollection(), "move_right");
    (void)	new KAction (i18n("Move Down"), Key_Down,
		this, SLOT(goDown()), actionCollection(), "move_down");
    (void)	new KAction (i18n("Move Left"), Key_Left,
		this, SLOT(goL()), actionCollection(), "move_left");
    (void)	new KAction (i18n("Stop"), Key_Space,
		this, SLOT(stop()), actionCollection(), "stop");
    (void)	new KAction (i18n("Dig Right"), Key_C,
		this, SLOT(digR()), actionCollection(), "dig_right");
    (void)	new KAction (i18n("Dig Left"), Key_Z,
		this, SLOT(digL()), actionCollection(), "dig_left");

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

    (void)	new KAction (i18n("Step"), Key_Period,
		game, SLOT(doStep()), actionCollection(), "do_step");
    (void)	new KAction (i18n("Test Bug Fix"), Key_B,
		game, SLOT(bugFix()), actionCollection(), "bug_fix");
    (void)	new KAction (i18n("Show Positions"), Key_D,
		game, SLOT(showFigurePositions()), actionCollection(), "step");
    (void)	new KAction (i18n("Start Logging"), Key_G,
		game, SLOT(startLogging()), actionCollection(), "logging");
    (void)	new KAction (i18n("Show Hero"), Key_H,
		game, SLOT(showHeroState()), actionCollection(), "show_hero");
    (void)	new KAction (i18n("Show Object"), Key_Question,
		game, SLOT(showObjectState()), actionCollection(), "show_obj");
    (void)	new KAction (i18n("Show Enemy") + "0", Key_0,
		this, SLOT(showEnemy0()), actionCollection(), "show_enemy_0");
    (void)	new KAction (i18n("Show Enemy") + "1", Key_1,
		this, SLOT(showEnemy1()), actionCollection(), "show_enemy_1");
    (void)	new KAction (i18n("Show Enemy") + "2", Key_2,
		this, SLOT(showEnemy2()), actionCollection(), "show_enemy_2");
    (void)	new KAction (i18n("Show Enemy") + "3", Key_3,
		this, SLOT(showEnemy3()), actionCollection(), "show_enemy_3");
    (void)	new KAction (i18n("Show Enemy") + "4", Key_4,
		this, SLOT(showEnemy4()), actionCollection(), "show_enemy_4");
    (void)	new KAction (i18n("Show Enemy") + "5", Key_5,
		this, SLOT(showEnemy5()), actionCollection(), "show_enemy_5");
    (void)	new KAction (i18n("Show Enemy") + "6", Key_6,
		this, SLOT(showEnemy6()), actionCollection(), "show_enemy_6");
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

    statusBar()->setSizeGripEnabled (FALSE);		// Use Settings menu ...

    statusBar()->insertItem ("", ID_LIVES);
    statusBar()->insertItem ("", ID_SCORE);
    statusBar()->insertItem ("", ID_LEVEL);
    statusBar()->insertItem ("", ID_HINTAVL);
    statusBar()->insertItem ("", ID_MSG, QSizePolicy::Horizontally);

    showLives (5);					// Start with 5 lives.
    showScore (0);
    showLevel (0);
    adjustHintAction (FALSE);

    // Set the PAUSE/RESUME key-names into the status bar message.
    pauseKeys = myPause->shortcut().toString();
    pauseKeys = pauseKeys.replace (';', "\" " + i18n("or") + " \"");
    gameFreeze (FALSE);

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
	tmp = tmp.rightJustify (3, '0');
    tmp.insert (0, i18n("   Lives: "));
    tmp.append ("   ");
    statusBar()->changeItem (tmp, ID_LIVES);
}

void KGoldrunner::showScore (long newScore)
{
    QString tmp;
    tmp.setNum (newScore);
    if (newScore < 10000)
	tmp = tmp.rightJustify (5, '0');
    tmp.insert (0, i18n("   Score: "));
    tmp.append ("   ");
    statusBar()->changeItem (tmp, ID_SCORE);
}

void KGoldrunner::showLevel (int newLevelNo)
{
    QString tmp;
    tmp.setNum (newLevelNo);
    if (newLevelNo < 100)
	tmp = tmp.rightJustify (3, '0');
    tmp.insert (0, i18n("   Level: "));
    tmp.append ("   ");
    statusBar()->changeItem (tmp, ID_LEVEL);
}

void KGoldrunner::gameFreeze (bool on_off)
{
    if (on_off)
	statusBar()->changeItem
		    (i18n("Press \"%1\" to RESUME").arg(pauseKeys), ID_MSG);
    else
	statusBar()->changeItem
		    (i18n("Press \"%1\" to PAUSE").arg(pauseKeys), ID_MSG);
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
	tradRules->activate();
    else
	kgrRules->activate();
}

void KGoldrunner::setEditMenu (bool on_off)
{
    saveEdits->setEnabled  (on_off);

    saveGame->setEnabled   (! on_off);
    hintAction->setEnabled (! on_off);
    killHero->setEnabled   (! on_off);
    highScore->setEnabled  (! on_off);

    if (on_off){
	editToolbar->show();
    }
    else {
	editToolbar->hide();
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

void KGoldrunner::setMouseMode()	{game->setMouseMode (TRUE);}
void KGoldrunner::setKeyBoardMode()	{game->setMouseMode (FALSE);}

// Local slots to set game speed.

void KGoldrunner::normalSpeed()		{hero->setSpeed (NSPEED);}
void KGoldrunner::beginSpeed()		{hero->setSpeed (BEGINSPEED);}
void KGoldrunner::champSpeed()		{hero->setSpeed (CHAMPSPEED);}
void KGoldrunner::incSpeed()		{hero->setSpeed (+1);}
void KGoldrunner::decSpeed()		{hero->setSpeed (-1);}

// Slots to set Traditional or KGoldrunner rules.

void KGoldrunner::setTradRules()
{
    KGrFigure::variableTiming = TRUE;
    KGrFigure::alwaysCollectNugget = TRUE;
    KGrFigure::runThruHole = TRUE;
    KGrFigure::reappearAtTop = TRUE;
    KGrFigure::searchStrategy = LOW;
}

void KGoldrunner::setKGrRules()
{
    KGrFigure::variableTiming = FALSE;
    KGrFigure::alwaysCollectNugget = FALSE;
    KGrFigure::runThruHole = FALSE;
    KGrFigure::reappearAtTop = FALSE;
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
    statusBar()->message(text);
}

void KGoldrunner::changeCaption(const QString& text)
{
    // display the text on the caption
    setCaption(text);
}

bool KGoldrunner::getDirectories()
{
    bool result = TRUE;

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
	"in area '%2' of the KDE folder ($KDEDIRS).")
	.arg(myDir).arg(dirs->kde_default ("html")));
	// result = FALSE;		// Don't abort if the doc is missing.
    }
    else
	systemHTMLDir.append ("en/" + myDir + "/");

    // Find the system collections in a directory of the required KDE type.
    systemDataDir = dirs->findResourceDir ("data", myDir + "/system/");
    if (systemDataDir.length() <= 0) {
	KGrMessage::information (this, i18n("Get Folders"),
	i18n("Cannot find system games sub-folder '%1/system/' "
	"in area '%2' of the KDE folder ($KDEDIRS).")
	.arg(myDir).arg(dirs->kde_default ("data")));
	result = FALSE;			// ABORT if the games data is missing.
    }
    else
	systemDataDir.append (myDir + "/system/");

    // Locate and optionally create directories for user collections and levels.
    bool create = TRUE;
    userDataDir   = dirs->saveLocation ("data", myDir + "/user/", create);
    if (userDataDir.length() <= 0) {
	KGrMessage::information (this, i18n("Get Folders"),
	i18n("Cannot find or create user games sub-folder '%1/user/' "
	"in area '%2' of the KDE user area ($KDEHOME).")
	.arg(myDir).arg(dirs->kde_default ("data")));
	// result = FALSE;		// Don't abort if user area is missing.
    }
    else {
	create = dirs->makeDir (userDataDir + "levels/");
	if (! create) {
	    KGrMessage::information (this, i18n("Get Folders"),
	    i18n("Cannot find or create 'levels/' folder in "
	    "sub-folder '%1/user/' in the KDE user area ($KDEHOME).").arg(myDir));
	    // result = FALSE;		// Don't abort if user area is missing.
	}
    }

    return (result);
}

// This method is invoked when top-level window is closed, whether by selecting
// "Quit" from the menu or by clicking the "X" at the top right of the window.

bool KGoldrunner::queryClose ()
{
    // Last chance to save: user has clicked "X" widget or menu-Quit.
    bool cannotContinue = TRUE;
    game->saveOK (cannotContinue);
    return (TRUE);
}

void KGoldrunner::setKey (KBAction movement)
{
    if (game->inEditMode()) return;

    // Using keyboard control can automatically disable mouse control.
    if (game->inMouseMode()) {
        // Halt the game while a message is displayed.
        game->setMessageFreeze (TRUE);

        switch (KGrMessage::warning (this, i18n("Switch to Keyboard Mode"),
		i18n("You have pressed a key that can be used to move the "
		"Hero. Do you want to switch automatically to keyboard "
		"control? Mouse control is easier to use in the long term "
		"- like riding a bike rather than walking!"),
		i18n("Switch to &Keyboard Mode"), i18n("Stay in &Mouse Mode")))
        {
        case 0: game->setMouseMode (FALSE);	// Set internal mouse mode OFF.
		setMouse->setChecked (FALSE);	// Adjust the Settings menu.
		setKeyboard->setChecked (TRUE);
                break;
        case 1: break;
        }

        // Unfreeze the game, but only if it was previously unfrozen.
        game->setMessageFreeze (FALSE);

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

#include <qwmatrix.h>
void KGoldrunner::makeEditToolbar()
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

    if (usesBigPixmaps()) {	// Scale up the pixmaps (to give cleaner looking
				// icons than leaving it for QToolButton to do).
	QWMatrix w;
	w = w.scale (2.0, 2.0);

	// The pixmaps shown on the buttons used to remain small and incorrectly
	// painted, in KDE, in spite of the 2x (32x32) scaling.  "insertButton"
	// calls QIconSet, to generate a set of icons from each pixmapx, then
	// seems to select the small size to paint on the button.  The following
	// line forces all icons, large and small, to be size 32x32 in advance.
	QIconSet::setIconSize (QIconSet::Small, QSize (32, 32));

	brickbg			= brickbg.xForm (w);
	fbrickbg		= fbrickbg.xForm (w);

	freebg			= freebg.xForm (w);
	nuggetbg		= nuggetbg.xForm (w);
	polebg			= polebg.xForm (w);
	betonbg			= betonbg.xForm (w);
	ladderbg		= ladderbg.xForm (w);
	hladderbg		= hladderbg.xForm (w);
	edherobg		= edherobg.xForm (w);
	edenemybg		= edenemybg.xForm (w);
    }

    editToolbar = new KToolBar (this, Qt::DockTop, TRUE, "Editor", TRUE);

    // Choose a colour that enhances visibility of the KGoldrunner pixmaps.
    // editToolbar->setPalette (QPalette (QColor (150, 150, 230)));

    // editToolbar->setHorizontallyStretchable (TRUE);	// Not effective in KDE.

    // All those separators are just to get reasonable visual spacing of the
    // pixmaps in KDE.  "setHorizontallyStretchable(TRUE)" does it in Qt.

    editToolbar->insertSeparator();

    editToolbar->insertButton ("filenew",  0,           SIGNAL(clicked()), game,
			SLOT(createLevel()),  TRUE,  i18n("&Create a Level"));
    editToolbar->insertButton ("fileopen", 1,           SIGNAL(clicked()), game,
			SLOT(updateLevel()),  TRUE,  i18n("&Edit Any Level..."));
    editToolbar->insertButton ("filesave", 2,           SIGNAL(clicked()), game,
			SLOT(saveLevelFile()),TRUE,  i18n("&Save Edits..."));

    editToolbar->insertSeparator();
    editToolbar->insertSeparator();

    editToolbar->insertButton ("ktip",     3,           SIGNAL(clicked()), game,
		SLOT(editNameAndHint()),TRUE,i18n("Edit Name/Hint"));

    editToolbar->insertSeparator();
    editToolbar->insertSeparator();

    editToolbar->insertButton (freebg,    (int)FREE,    SIGNAL(clicked()), this,
		SLOT(freeSlot()),     TRUE,  i18n("Empty space"));
    editToolbar->insertSeparator();
    editToolbar->insertButton (edherobg,  (int)HERO,    SIGNAL(clicked()), this,
		SLOT (edheroSlot()),  TRUE,  i18n("Hero"));
    editToolbar->insertSeparator();
    editToolbar->insertButton (edenemybg, (int)ENEMY,   SIGNAL(clicked()), this,
		SLOT (edenemySlot()), TRUE,  i18n("Enemy"));
    editToolbar->insertSeparator();
    editToolbar->insertButton (brickbg,   (int)BRICK,   SIGNAL(clicked()), this,
		SLOT (brickSlot()),   TRUE,  i18n("Brick (can dig)"));
    editToolbar->insertSeparator();
    editToolbar->insertButton (betonbg,   (int)BETON,   SIGNAL(clicked()), this,
		SLOT (betonSlot()),   TRUE,  i18n("Concrete (cannot dig)"));
    editToolbar->insertSeparator();
    editToolbar->insertButton (fbrickbg,  (int)FBRICK,  SIGNAL(clicked()), this,
		SLOT (fbrickSlot()),  TRUE,  i18n("Trap (can fall through)"));
    editToolbar->insertSeparator();
    editToolbar->insertButton (ladderbg,  (int)LADDER,  SIGNAL(clicked()), this,
		SLOT (ladderSlot()),  TRUE,  i18n("Ladder"));
    editToolbar->insertSeparator();
    editToolbar->insertButton (hladderbg, (int)HLADDER, SIGNAL(clicked()), this,
		SLOT (hladderSlot()), TRUE,  i18n("Hidden ladder"));
    editToolbar->insertSeparator();
    editToolbar->insertButton (polebg,    (int)POLE,    SIGNAL(clicked()), this,
		SLOT (poleSlot()),    TRUE,  i18n("Pole (or bar)"));
    editToolbar->insertSeparator();
    editToolbar->insertButton (nuggetbg,  (int)NUGGET,  SIGNAL(clicked()), this,
		SLOT (nuggetSlot()),  TRUE,  i18n("Gold nugget"));

    editToolbar->setToggle ((int) FREE,    TRUE);
    editToolbar->setToggle ((int) HERO,    TRUE);
    editToolbar->setToggle ((int) ENEMY,   TRUE);
    editToolbar->setToggle ((int) BRICK,   TRUE);
    editToolbar->setToggle ((int) BETON,   TRUE);
    editToolbar->setToggle ((int) FBRICK,  TRUE);
    editToolbar->setToggle ((int) LADDER,  TRUE);
    editToolbar->setToggle ((int) HLADDER, TRUE);
    editToolbar->setToggle ((int) POLE,    TRUE);
    editToolbar->setToggle ((int) NUGGET,  TRUE);

    pressedButton = (int) BRICK;
    editToolbar->setButton (pressedButton, TRUE);
}

/******************************************************************************/
/*********************   EDIT-BUTTON SLOTS   **********************************/
/******************************************************************************/

void KGoldrunner::freeSlot()
		{ game->setEditObj (FREE);    setButton ((int) FREE);    }
void KGoldrunner::edheroSlot()
		{ game->setEditObj (HERO);    setButton ((int) HERO);    }
void KGoldrunner::edenemySlot()
		{ game->setEditObj (ENEMY);   setButton ((int) ENEMY);   }
void KGoldrunner::brickSlot()
		{ game->setEditObj (BRICK);   setButton ((int) BRICK);   }
void KGoldrunner::betonSlot()
		{ game->setEditObj (BETON);   setButton ((int) BETON);   }
void KGoldrunner::fbrickSlot()
		{ game->setEditObj (FBRICK);  setButton ((int) FBRICK);  }
void KGoldrunner::ladderSlot()
		{ game->setEditObj (LADDER);  setButton ((int) LADDER);  }
void KGoldrunner::hladderSlot()
		{ game->setEditObj (HLADDER); setButton ((int) HLADDER); }
void KGoldrunner::poleSlot()
		{ game->setEditObj (POLE);    setButton ((int) POLE);    }
void KGoldrunner::nuggetSlot()
		{ game->setEditObj (NUGGET);  setButton ((int) NUGGET);  }
void KGoldrunner::defaultEditObj()
		{ setButton ((int) BRICK);				 }

void KGoldrunner::setButton (int btn)
{
    editToolbar->setButton (pressedButton, FALSE);
    pressedButton = btn;
    editToolbar->setButton (pressedButton, TRUE);
}

#include "kgoldrunner.moc"
