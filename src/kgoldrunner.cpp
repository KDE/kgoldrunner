/*
    Copyright 2002 Marco Krüger <grisuji@gmx.de>
    Copyright 2002 Ian Wadham <ianw2@optusnet.com.au>

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

#include "kgoldrunner.h"

#include <QPixmap>
#include <QDesktopWidget>
#include <QApplication>

#include <kglobal.h>
#include <kstatusbar.h>
#include <kshortcutsdialog.h>

#include <kconfig.h>
#include <kconfiggroup.h>

#include <ktoolbar.h>
#include <kmenubar.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <ktogglefullscreenaction.h>
#include <kstandardaction.h>
#include <kstandardgameaction.h>
#include <kicon.h>
#include <KMenu>
#include <KCmdLineArgs>
#include <KAboutData>

#include "kgrobject.h"
#include "kgrfigure.h"
#include "kgrcanvas.h"
#include "kgrdialog.h"
#include "kgrgame.h"

KGoldrunner::KGoldrunner()
      // : view (new KGrCanvas (this))
{
/******************************************************************************/
/*************  FIND WHERE THE GAMES DATA AND HANDBOOK SHOULD BE  *************/
/******************************************************************************/

    setObjectName ("KGoldrunner");

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

    // Base the size of playing-area and widgets on the monitor resolution.
    int dw = QApplication::desktop()->width();

    // Need to consider the height, for widescreen displays (eg. 1280x768).
    int dh = QApplication::desktop()->height();

    double scale = 1.0;
    if ((dw > 800) && (dh > 600)) {			// More than 800x600.
	scale = 1.25;			// Scale 1.25:1.
    }
    if ((dw > 1024) && (dh > 768))  {			// More than 1024x768.
	scale = 1.75;			// Scale 1.75:1.
    }
    view = new KGrCanvas (this, scale, systemDataDir);
    game = new KGrGame (view, systemDataDir, userDataDir);

    // Initialise the collections of levels (i.e. the list of games).
    if (! game->initCollections()) {
	startupOK = false;
	return;				// If no game files, abort.
    }

    qDebug() << "Calling view->setBaseScale() ...";
    view->setBaseScale();		// Set scale for level-titles font.

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

    setupGUI();

    // Find the theme-files and generate the Themes menu.
    setupThemes();

    // Connect the game actions to the menu and toolbar displays.
    connect(game, SIGNAL (setEditMenu (bool)),	SLOT (setEditMenu (bool)));
    connect(game, SIGNAL (setEditMenu (bool)),	SLOT (resizeMainWindow ()));
    connect(game, SIGNAL (markRuleType (char)), SLOT (markRuleType (char)));
    connect(game, SIGNAL (hintAvailable(bool)),	SLOT (adjustHintAction(bool)));
    connect(game, SIGNAL (defaultEditObj()),	SLOT (defaultEditObj()));

    // Apply the saved mainwindow settings, if any, and ask the mainwindow
    // to automatically save settings if changed: window size, toolbar
    // position, icon size, etc.
    setAutoSaveSettings();

    // Explicitly hide the edit toolbar - we need it in edit mode only and we
    // always start in play mode, even if the last session ended in edit mode
    // and the config file says to show the edit toolbar.
    toolBar("editToolbar")->hide();
    toolBar("editToolbar")->setAllowedAreas(Qt::TopToolBarArea);

    // Set mouse control of the hero as the default.
    game->setMouseMode (true);

    // Paint the main widget (title, menu, status bar, blank playfield).
    show();

    // Queue the call to the "Start Game" dialog: let KMainWindow show first.
    QMetaObject::invokeMethod(game, "startLevelOne", Qt::QueuedConnection);

    // Queue the resizing of KMainWindow.
    // IDW QMetaObject::invokeMethod(this, "resizeMainWindow", Qt::QueuedConnection);
}

KGoldrunner::~KGoldrunner()
{
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

    QAction * newAction =	KStandardGameAction::
				gameNew (
				game,
				SLOT(startLevelOne()), this);
    actionCollection()->addAction(newAction->objectName(), newAction);
    newAction->			setText (i18n("&New Game..."));
    QAction * loadGame =	KStandardGameAction::
				load (
				game, SLOT(loadGame()), this);
    actionCollection()->addAction(loadGame->objectName(), loadGame);
    loadGame->			setText (i18n("&Load Saved Game..."));

    QAction* playAnyAct = actionCollection()->addAction("play_any");
    playAnyAct->setText(i18n("&Play Any Level..."));
    connect(playAnyAct, SIGNAL(triggered(bool)), game, SLOT(startAnyLevel()));

    QAction* playNextAct = actionCollection()->addAction("play_next");
    playNextAct->setText(i18n("Play &Next Level..."));
    connect(playNextAct, SIGNAL(triggered(bool)), game, SLOT(startNextLevel()));

    // Save Game...
    // Save Edits... (extra copy)
    // --------------------------

    saveGame =			KStandardGameAction::
				save (
				game, SLOT(saveGame()), this);
    actionCollection()->addAction(saveGame->objectName(), saveGame);
    saveGame->			setText (i18n("&Save Game..."));
    saveGame->			setShortcut (Qt::Key_S); // Alternate key.

    // Pause
    // Show High Scores
    // Get a Hint
    // Kill the Hero
    // --------------------------

    // KAction * myPause: to get KAction::shortcut() not QAction::shortcut().
    myPause = KStandardGameAction::pause (this, SLOT(stopStart()), this);
    actionCollection()->addAction (myPause->objectName(), myPause);
    KShortcut pauseShortcut = myPause->shortcut();
    pauseShortcut.setAlternate (Qt::Key_Escape);	// Add "Esc" shortcut.
    myPause->setShortcut (pauseShortcut);

    highScore = KStandardGameAction::highscores
				(game, SLOT(showHighScores()), this);
    actionCollection()->addAction(highScore->objectName(), highScore);

    hintAction = KStandardGameAction::hint
				(game, SLOT(showHint()), this);
    actionCollection()->addAction(hintAction->objectName(), hintAction);

    killHero =	actionCollection()->addAction("kill_hero");
    killHero->setText (i18n("&Kill Hero"));
    killHero->setShortcut (Qt::Key_Q);
    connect( killHero, SIGNAL(triggered(bool)), game, SLOT(herosDead()));

    // Quit
    // --------------------------

    KStandardGameAction::quit (this, SLOT(close()), actionCollection());

    /**************************************************************************/
    /***************************   GAME EDITOR MENU  **************************/
    /**************************************************************************/

    // Create a Level
    // Edit Any Level...
    // Edit Next Level...
    // --------------------------

    QAction* createAct = actionCollection()->addAction("create_level");
    createAct->setText(i18n("&Create Level"));
    createAct->setIcon(KIcon("document-new"));
    connect( createAct, SIGNAL(triggered(bool)), game, SLOT(createLevel()));

    QAction* editAnyAct	= actionCollection()->addAction("edit_any");
    editAnyAct->setText(i18n("&Edit Any Level..."));
    editAnyAct->setIcon(KIcon("document-open"));
    connect( editAnyAct, SIGNAL(triggered(bool)), game, SLOT(updateLevel()));

    QAction* editNextAct = actionCollection()->addAction("edit_next");
    editNextAct->setText(i18n("Edit &Next Level..."));
    connect( editNextAct, SIGNAL(triggered(bool)), game, SLOT(updateNext()));

    // Save Edits...
    // Move Level...
    // Delete Level...
    // --------------------------

    saveEdits =	actionCollection()->addAction("save_edits");
    saveEdits->setText(i18n("&Save Edits..."));
    saveEdits->setIcon(KIcon("document-save"));
    connect( saveEdits, SIGNAL(triggered(bool)), game, SLOT(saveLevelFile()));
    saveEdits->setEnabled (false);			// Nothing to save, yet.

    QAction* moveLevel = actionCollection()->addAction("move_level");
    moveLevel->setText(i18n("&Move Level..."));
    connect( moveLevel, SIGNAL(triggered(bool)), game, SLOT(moveLevelFile()));

    QAction* deleteLevel = actionCollection()->addAction("delete_level");
    deleteLevel->setText(i18n("&Delete Level..."));
    connect( deleteLevel, SIGNAL(triggered(bool)), game, SLOT(deleteLevelFile()));

    // Create a Game
    // Edit Game Info...
    // --------------------------

    QAction* createGame	= actionCollection()->addAction("create_game");
    createGame->setText(i18n("Create Game..."));
    connect( createGame, SIGNAL(triggered(bool)), this, SLOT(createGame()));

    QAction* editGame =	actionCollection()->addAction("edit_game");
    editGame->setText(i18n("Edit Game Info..."));
    connect( editGame, SIGNAL(triggered(bool)), this, SLOT(editGameInfo()));

    /**************************************************************************/
    /*****************************   THEMES MENU  *****************************/
    /**************************************************************************/

    // The Themes menu is obtained AFTER calling setupGUI(), by locating an
    // open-ended list of theme-files and plugging the translated text-names of
    // the themes in place of ActionList <name="theme_list" /> in the ui.rc file.

    /**************************************************************************/
    /****************************   SETTINGS MENU  ****************************/
    /**************************************************************************/

    // Mouse Controls Hero
    // Keyboard Controls Hero
    // --------------------------

    setMouse = new KToggleAction(i18n("&Mouse Controls Hero"), this);
    actionCollection()->addAction("mouse_mode", setMouse);
    connect( setMouse, SIGNAL(triggered(bool)), this, SLOT(setMouseMode()));

    setKeyboard = new KToggleAction (i18n("&Keyboard Controls Hero"), this);
    actionCollection()->addAction("keyboard_mode", setKeyboard);
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
				this);
    actionCollection()->addAction("normal_speed", nSpeed);
    connect( nSpeed, SIGNAL(triggered(bool)), this, SLOT(normalSpeed()));

    KToggleAction * bSpeed =	new KToggleAction (
				i18n("Beginner Speed"),
				this);
    actionCollection()->addAction("beginner_speed", bSpeed);
    connect( bSpeed, SIGNAL(triggered(bool)), this, SLOT(beginSpeed()));

    KToggleAction * cSpeed =	new KToggleAction (
				i18n("Champion Speed"),
				this);
    actionCollection()->addAction("champion_speed", cSpeed);
    connect( cSpeed, SIGNAL(triggered(bool)), this, SLOT(champSpeed()));

    QAction * iSpeed =	actionCollection()->addAction("increase_speed");
    iSpeed->setText(i18n("Increase Speed"));
    iSpeed->setShortcut( Qt::Key_Plus );
    connect( iSpeed, SIGNAL(triggered(bool)), this, SLOT(incSpeed()));

    QAction * dSpeed =	actionCollection()->addAction("decrease_speed");
    dSpeed->setText(i18n("Decrease Speed"));
    dSpeed->setShortcut( Qt::Key_Minus );
    connect( dSpeed, SIGNAL(triggered(bool)), this, SLOT(decSpeed()));

    QActionGroup* speedGrp = new QActionGroup(this);
    speedGrp->addAction(nSpeed);
    speedGrp->addAction(bSpeed);
    speedGrp->addAction(cSpeed);
    nSpeed->setChecked(true);

    // Traditional Rules
    // KGoldrunner Rules
    // --------------------------

    tradRules =			new KToggleAction (
				i18n("&Traditional Rules"),
				this);
    actionCollection()->addAction("trad_rules", tradRules);
    connect( tradRules, SIGNAL(triggered(bool)), this, SLOT(setTradRules()));

    kgrRules =			new KToggleAction (
				i18n("K&Goldrunner Rules"),
				this);
    actionCollection()->addAction("kgr_rules", kgrRules);
    connect( kgrRules, SIGNAL(triggered(bool)), this, SLOT(setKGrRules()));

    QActionGroup* rulesGrp = new QActionGroup(this);
    rulesGrp->addAction(tradRules);
    rulesGrp->addAction(kgrRules);
    tradRules->setChecked (true);

    // FullScreen
    fullScreen = KStandardAction::fullScreen(this, SLOT(viewFullScreen(bool)), this, this);
    actionCollection()->addAction(fullScreen->objectName(), fullScreen);

    
    // Configure Shortcuts...
    // Configure Toolbars...
    // --------------------------

    KStandardAction::keyBindings (
				this, SLOT(optionsConfigureKeys()),
				actionCollection());
    // KStandardAction::configureToolbars (
				// this, SLOT(optionsConfigureToolbars()),
				// actionCollection());

    /**************************************************************************/
    /**************************   KEYSTROKE ACTIONS  **************************/
    /**************************************************************************/

    // Two-handed KB controls and alternate one-handed controls for the hero.

    QAction* moveUp = actionCollection()->addAction("move_up");
    moveUp->setText(i18n("Move Up"));
    moveUp->setShortcut( Qt::Key_Up );
    connect( moveUp, SIGNAL(triggered(bool)), this, SLOT(goUp()));

    QAction* moveRight = actionCollection()->addAction("move_right");
    moveRight->setText(i18n("Move Right"));
    moveRight->setShortcut( Qt::Key_Right );
    connect( moveRight, SIGNAL(triggered(bool)), this, SLOT(goR()));

    QAction* moveDown = actionCollection()->addAction("move_down");
    moveDown->setText(i18n("Move Down"));
    moveDown->setShortcut( Qt::Key_Down );
    connect( moveDown, SIGNAL(triggered(bool)), this, SLOT(goDown()));

    QAction* moveLeft = actionCollection()->addAction("move_left");
    moveLeft->setText(i18n("Move Left"));
    moveLeft->setShortcut( Qt::Key_Left );
    connect( moveLeft, SIGNAL(triggered(bool)), this, SLOT(goL()));

    QAction* stop = actionCollection()->addAction("stop");
    stop->setText(i18n("Stop"));
    stop->setShortcut( Qt::Key_Space );
    connect( stop, SIGNAL(triggered(bool)), this, SLOT(stop()));

    QAction* digRight = actionCollection()->addAction("dig_right");
    digRight->setText(i18n("Dig Right"));
    digRight->setShortcut( Qt::Key_C );
    connect( digRight, SIGNAL(triggered(bool)), this, SLOT(digR()));

    QAction* digLeft = actionCollection()->addAction("dig_left");
    digLeft->setText(i18n("Dig Left"));
    digLeft->setShortcut( Qt::Key_Z );
    connect( digLeft, SIGNAL(triggered(bool)), this, SLOT(digL()));

    // Plug actions into the gui, or accelerators will not work (KDE4)
    addAction(moveUp);
    addAction(moveDown);
    addAction(moveLeft);
    addAction(moveRight);
    addAction(digLeft);
    addAction(digRight);
    addAction(stop);

    setupEditToolbarActions();			// Uses pixmaps from "view".

    // Alternate one-handed controls.  Set up in "kgoldrunnerui.rc".

    // Key_I, "move_up"
    // Key_L, "move_right"
    // Key_K, "move_down"
    // Key_J, "move_left"
    // Key_Space, "stop" (as above)
    // Key_O, "dig_right"
    // Key_U, "dig_left"

    KConfigGroup gameGroup (KGlobal::config(), "Debugging"); // Get prev theme.
    bool addDebuggingShortcuts = gameGroup.readEntry ("DebuggingShortcuts", false);
    if (!addDebuggingShortcuts) return;
    // Authors' debugging aids.

    QAction* step = actionCollection()->addAction("do_step");
    step->setText(i18n("Step"));
    step->setShortcut( Qt::Key_Period );
    connect( step, SIGNAL(triggered(bool)), game, SLOT(doStep()) );
    addAction(step);

    QAction* bugFix = actionCollection()->addAction("bug_fix");
    bugFix->setText(i18n("Test Bug Fix"));
    bugFix->setShortcut( Qt::Key_B );
    connect( bugFix, SIGNAL(triggered(bool)), game, SLOT(bugFix()) );
    addAction(bugFix);

    QAction* showPos = actionCollection()->addAction("step");
    showPos->setText(i18n("Show Positions"));
    showPos->setShortcut( Qt::Key_D );
    connect( showPos, SIGNAL(triggered(bool)), game, SLOT(showFigurePositions()) );
    addAction(showPos);

    QAction* startLog = actionCollection()->addAction("logging");
    startLog->setText(i18n("Start Logging"));
    startLog->setShortcut( Qt::Key_G );
    connect( startLog, SIGNAL(triggered(bool)), game, SLOT(startLogging()) );
    addAction(startLog);

    QAction* showHero = actionCollection()->addAction("show_hero");
    showHero->setText(i18n("Show Hero"));
    showHero->setShortcut( Qt::Key_R );		// H is for Hint now.
    connect( showHero, SIGNAL(triggered(bool)), game, SLOT(showHeroState()) );
    addAction(showHero);

    QAction* showObj = actionCollection()->addAction("show_obj");
    showObj->setText(i18n("Show Object"));
    showObj->setShortcut( Qt::Key_Question );
    connect( showObj, SIGNAL(triggered(bool)), game, SLOT(showObjectState()) );
    addAction(showObj);

    QAction* showEnemy0 = actionCollection()->addAction("show_enemy_0");
    showEnemy0->setText(i18n("Show Enemy") + '0');
    showEnemy0->setShortcut( Qt::Key_0 );
    connect( showEnemy0, SIGNAL(triggered(bool)), this, SLOT(showEnemy0()) );
    addAction(showEnemy0);

    QAction* showEnemy1 = actionCollection()->addAction("show_enemy_1");
    showEnemy1->setText(i18n("Show Enemy") + '1');
    showEnemy1->setShortcut( Qt::Key_1 );
    connect( showEnemy1, SIGNAL(triggered(bool)), this, SLOT(showEnemy1()) );
    addAction(showEnemy1);

    QAction* showEnemy2 = actionCollection()->addAction("show_enemy_2");
    showEnemy2->setText(i18n("Show Enemy") + '2');
    showEnemy2->setShortcut( Qt::Key_2 );
    connect( showEnemy2, SIGNAL(triggered(bool)), this, SLOT(showEnemy2()) );
    addAction(showEnemy2);

    QAction* showEnemy3 = actionCollection()->addAction("show_enemy_3");
    showEnemy3->setText(i18n("Show Enemy") + '3');
    showEnemy3->setShortcut( Qt::Key_3 );
    connect( showEnemy3, SIGNAL(triggered(bool)), this, SLOT(showEnemy3()) );
    addAction(showEnemy3);

    QAction* showEnemy4 = actionCollection()->addAction("show_enemy_4");
    showEnemy4->setText(i18n("Show Enemy") + '4');
    showEnemy4->setShortcut( Qt::Key_4 );
    connect( showEnemy4, SIGNAL(triggered(bool)), this, SLOT(showEnemy4()) );
    addAction(showEnemy4);

    QAction* showEnemy5 = actionCollection()->addAction("show_enemy_5");
    showEnemy5->setText(i18n("Show Enemy") + '5');
    showEnemy5->setShortcut( Qt::Key_5 );
    connect( showEnemy5, SIGNAL(triggered(bool)), this, SLOT(showEnemy5()) );
    addAction(showEnemy5);

    QAction* showEnemy6 = actionCollection()->addAction("show_enemy_6");
    showEnemy6->setText(i18n("Show Enemy") + '6');
    showEnemy6->setShortcut( Qt::Key_6 );
    connect( showEnemy6, SIGNAL(triggered(bool)), this, SLOT(showEnemy6()) );
    addAction(showEnemy6);
}

void KGoldrunner::viewFullScreen(bool activation)
{
    if (activation) 
	showFullScreen();
    else
	showNormal();
}

void KGoldrunner::setupThemes ()
{
    // Look for themes in files "---/share/apps/kgoldrunner/themes/*.desktop".
    KGlobal::dirs()->addResourceType ("theme", "data", QString(KCmdLineArgs::aboutData()->appName()) +
                                      QString("/themes/"));

    QStringList themeFilepaths = KGlobal::dirs()->findAllResources
	("theme", "*.desktop", KStandardDirs::NoDuplicates); // Find files.

    KConfigGroup gameGroup (KGlobal::config(), "KDEGame"); // Get prev theme.
    QString currentThemeFilepath = gameGroup.readEntry ("ThemeFilepath", "");
    qDebug() << endl << "Config() Theme" << currentThemeFilepath;

    QSignalMapper * themeMapper = new QSignalMapper (this);
    connect (themeMapper, SIGNAL (mapped (const QString &)),
		this, SLOT (changeTheme (const QString &)));

    KToggleAction * newTheme;				// Action for a theme.
    QString actionName;					// Name of the theme.
    QList<QAction *> themeList;				// Themes for menu.
    QActionGroup * themeGroup = new QActionGroup (this);
    themeGroup->setExclusive (true);			// Exclusive toggles.

    foreach (QString filepath, themeFilepaths) {	// Read each theme-file.
	KConfig theme (filepath, KConfig::OnlyLocal);	// Extract theme-name.
	KConfigGroup group = theme.group ("KDEGameTheme");	// Translated.
	actionName = group.readEntry ("Name", i18n("Missing Name"));

	newTheme = new KToggleAction (actionName, this);
	themeGroup->addAction (newTheme);		// Add to toggle-group.

	if ((currentThemeFilepath == filepath) ||	// If theme prev chosen
	    ((currentThemeFilepath.isEmpty()) &&	// or it is the default
	     (filepath.indexOf ("default") >= 0)) ||
	    ((filepath == themeFilepaths.last()) &&
	     (themeGroup->checkedAction() == 0))) {	// or last in the list,
	    newTheme->setChecked (true);		// mark it as chosen and
	    view->changeTheme (filepath);		// tell graphics init.
	}

	connect (newTheme, SIGNAL(triggered (bool)), themeMapper, SLOT(map ()));
	themeMapper->setMapping (newTheme, filepath);	// Add path to signal.
	themeList.append (newTheme);			// Theme --> menu list.
    }

    unplugActionList ("theme_list");
    plugActionList   ("theme_list", themeList);		// Insert list in menu.
}

/******************************************************************************/
/**********************  SLOTS FOR STATUS BAR UPDATES  ************************/
/******************************************************************************/

void KGoldrunner::initStatusBar()
{
    // QString s = statusBar()->fontInfo().family();	// Set bold font.
    // int i = statusBar()->fontInfo().pointSize();
    // statusBar()->setFont (QFont (s, i, QFont::Bold));

    statusBar()->setSizeGripEnabled (true);

    statusBar()->insertPermanentItem ("", ID_LIVES);
    statusBar()->insertPermanentItem ("", ID_SCORE);
    statusBar()->insertPermanentItem ("", ID_LEVEL);
    statusBar()->insertPermanentItem ("", ID_HINTAVL);
    statusBar()->insertPermanentItem ("", ID_MSG, 1);

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
    statusBar()->setItemFixed (ID_HINTAVL, -1);

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
    if (newScore < 1000000)
	tmp = tmp.rightJustified (7, '0');
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
	// Set the editToolbar icons to the current tile-size.
	toolBar("editToolbar")->setIconSize (view->getPixmap(BRICK).size());

	// Set the editToolbar icons up with pixmaps of the current theme.
	setEditIcon ("brickbg",   BRICK);
	setEditIcon ("fbrickbg",  FBRICK);
	setEditIcon ("freebg",    FREE);
	setEditIcon ("nuggetbg",  NUGGET);
	setEditIcon ("polebg",    POLE);
	setEditIcon ("betonbg",   BETON);
	setEditIcon ("ladderbg",  LADDER);
	setEditIcon ("hladderbg", HLADDER);
	setEditIcon ("edherobg",  HERO);
	setEditIcon ("edenemybg", ENEMY);

	toolBar("editToolbar")->show();
    }
    else {
	toolBar("editToolbar")->hide();
    }
}

void KGoldrunner::setEditIcon (const QString & actionName, const char iconType)
{
    ((KToggleAction *) (actionCollection()->action(actionName)))->
		setIcon(KIcon(view->getPixmap(iconType)));
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

void KGoldrunner::changeTheme (const QString & themeFilepath)
{
    view->changeTheme (themeFilepath);
    if (game->inEditMode()) {
	setEditMenu (true);
    }
}

// Local slots to create or edit game information.

void KGoldrunner::createGame()		{game->editCollection (SL_CR_GAME);}
void KGoldrunner::editGameInfo()	{game->editCollection (SL_UPD_GAME);}

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

void KGoldrunner::resizeMainWindow()
{
    qDebug() << "KGoldrunner::resizeMainWindow() called ...";
    return; // IDW

    // Get the area of our view, menubar and statusBar, and add the editToolbar (if visible)
    int newHeight = view->size().height() + statusBar()->height() + menuBar()->height();
    if (!toolBar("editToolbar")->isHidden())
	newHeight = newHeight + toolBar("editToolbar")->height();

    // IDW setFixedSize (view->size().width(), newHeight);
    resize (view->size().width(), newHeight);

    //Maybe we need a layout manager here? Then we could possibly just use...
    //resize(view->size());
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

void KGoldrunner::saveProperties(KConfigGroup & /* config - unused */)
{
    // The 'config' object points to the session managed
    // config file.  Anything you write here will be available
    // later when this app is restored.

    qDebug() << "I am in KGoldrunner::saveProperties.";
}

void KGoldrunner::readProperties(const KConfigGroup & /* config - unused */)
{
    // The 'config' object points to the session managed
    // config file.  This function is automatically called whenever
    // the app is being restored.  Read in here whatever you wrote
    // in 'saveProperties'

    qDebug() << "I am in KGoldrunner::readProperties.";
}

// void KGoldrunner::optionsShowToolbar()
// {
    // this is all very cut and paste code for showing/hiding the
    // toolbar
    // if (m_toolbarAction->isChecked())
        // toolBar()->show();
    // else
        // toolBar()->hide();
// }

// void KGoldrunner::optionsShowStatusbar()
// {
    // this is all very cut and paste code for showing/hiding the
    // statusbar
    // if (m_statusbarAction->isChecked())
        // statusBar()->show();
    // else
        // statusBar()->hide();
// }

void KGoldrunner::optionsConfigureKeys()
{
    KShortcutsDialog::configure(actionCollection());

    // Update the PAUSE/RESUME message in the status bar.
    pauseKeys = myPause->shortcut().toString();
    pauseKeys = pauseKeys.replace (';', "\" " + i18n("or") + " \"");
    gameFreeze (KGrObject::frozen);	// Refresh the status bar text.
}

// void KGoldrunner::optionsConfigureToolbars()
// {
    // KConfigGroup cg (KGlobal::config(), autoSaveGroup());
    // saveMainWindowSettings (cg);
// }

// void KGoldrunner::newToolbarConfig()
// {
    // this slot is called when user clicks "Ok" or "Apply" in the toolbar editor.
    // recreate our GUI, and re-apply the settings (e.g. "text under icons", etc.)
    // createGUI();

    // applyMainWindowSettings( KGlobal::config()->group( autoSaveGroup()) );
// }

// void KGoldrunner::optionsPreferences()
// {
    // popup some sort of preference dialog, here
    // KGoldrunnerPreferences dlg;
    // if (dlg.exec())
    // {
        // redo your settings
    // }
// }

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

    QString myDir = "kgoldrunner";

    // Find the KGoldrunner Users' Guide, English version (en).
    systemHTMLDir = dirs->findResourceDir ("html", "en/" + myDir + '/');
    if (systemHTMLDir.length() <= 0) {
	KGrMessage::information (this, i18n("Get Folders"),
		i18n("Cannot find documentation sub-folder 'en/%1/' "
		"in area '%2' of the KDE folder ($KDEDIRS).",
		myDir, dirs->resourceDirs("html").join( ":" )));
	// result = false;		// Don't abort if the doc is missing.
    }
    else
	systemHTMLDir.append ("en/" + myDir + '/');

    // Find the system collections in a directory of the required KDE type.
    systemDataDir = dirs->findResourceDir ("data", myDir + "/system/");
    if (systemDataDir.length() <= 0) {
	KGrMessage::information (this, i18n("Get Folders"),
	i18n("Cannot find system games sub-folder '%1/system/' "
	"in area '%2' of the KDE folder ($KDEDIRS).",
	 myDir, dirs->resourceDirs ("data").join( ":" )));
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
	 myDir, dirs->resourceDirs ("data").join( ":")));
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

void KGoldrunner::setupEditToolbarActions()
{
    // Choose a colour that enhances visibility of the KGoldrunner pixmaps.
    // editToolbar->setPalette (QPalette (QColor (150, 150, 230)));

    QAction* ktipAct = actionCollection()->addAction ("edit_hint");
    ktipAct->setIcon (KIcon("ktip"));
    ktipAct->setText (i18n("Edit Name/Hint"));
    connect (ktipAct, SIGNAL(triggered(bool)), game, SLOT(editNameAndHint()));

    KToggleAction* freebgAct = new KToggleAction (i18n("Empty space"), this);
    actionCollection()->addAction ("freebg", freebgAct);
    connect (freebgAct, SIGNAL(triggered(bool)), this, SLOT(freeSlot()));

    KToggleAction* edherobgAct = new KToggleAction (i18n("Hero"), this);
    actionCollection()->addAction( "edherobg", edherobgAct );
    connect (edherobgAct, SIGNAL(triggered(bool)), this, SLOT(edheroSlot()));

    KToggleAction* edenemybgAct = new KToggleAction (i18n("Enemy"), this );
    actionCollection()->addAction( "edenemybg", edenemybgAct );
    connect (edenemybgAct, SIGNAL(triggered(bool)), this, SLOT(edenemySlot()));

    KToggleAction* brickbgAct = new KToggleAction (i18n("Brick (can dig)"), this);
    actionCollection()->addAction ("brickbg", brickbgAct);
    connect (brickbgAct, SIGNAL(triggered(bool)), this, SLOT(brickSlot()));

    KToggleAction* betonbgAct = new KToggleAction (i18n("Concrete (cannot dig)"), this);
    actionCollection()->addAction ("betonbg", betonbgAct);
    connect (betonbgAct, SIGNAL(triggered(bool)), this, SLOT(betonSlot()));

    KToggleAction* fbrickbgAct = new KToggleAction (i18n("Trap (can fall through)"), this);
    actionCollection()->addAction ("fbrickbg", fbrickbgAct);
    connect (fbrickbgAct, SIGNAL(triggered(bool)), this, SLOT(fbrickSlot()));

    KToggleAction* ladderbgAct = new KToggleAction (i18n("Ladder"), this);
    actionCollection()->addAction ("ladderbg", ladderbgAct);
    connect (ladderbgAct, SIGNAL(triggered(bool)), this, SLOT(ladderSlot()));

    KToggleAction* hladderbgAct = new KToggleAction (i18n("Hidden ladder"), this);
    actionCollection()->addAction ("hladderbg", hladderbgAct);
    connect (hladderbgAct, SIGNAL(triggered(bool)), this, SLOT(hladderSlot()));

    KToggleAction* polebgAct = new KToggleAction (i18n("Pole (or bar)"), this);
    actionCollection()->addAction ("polebg", polebgAct);
    connect (polebgAct, SIGNAL(triggered(bool)), this, SLOT(poleSlot()));

    KToggleAction* nuggetbgAct = new KToggleAction (i18n("Gold nugget"), this);
    actionCollection()->addAction ("nuggetbg", nuggetbgAct);
    connect (nuggetbgAct, SIGNAL(triggered(bool)), this, SLOT(nuggetSlot()));

    QActionGroup* editButtons = new QActionGroup(this);
    editButtons->setExclusive (true);
    editButtons->addAction (freebgAct);
    editButtons->addAction (edherobgAct);
    editButtons->addAction (edenemybgAct);
    editButtons->addAction (brickbgAct);
    editButtons->addAction (betonbgAct);
    editButtons->addAction (fbrickbgAct);
    editButtons->addAction (ladderbgAct);
    editButtons->addAction (hladderbgAct);
    editButtons->addAction (polebgAct);
    editButtons->addAction (nuggetbgAct);

    brickbgAct->setChecked (true);
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

QSize KGoldrunner::sizeHint() const
{
    qDebug() << "KGoldrunner::sizeHint() called ... 640x600";
    return QSize (640, 600);
}

#include "kgoldrunner.moc"
