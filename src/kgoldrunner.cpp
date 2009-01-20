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

#include <QApplication>
#include <QDesktopWidget>
#include <QSignalMapper>
#include <QShortcut>
#include <QKeySequence>

#include <kglobal.h>
#include <kstatusbar.h>
#include <kshortcutsdialog.h>

#include <kconfig.h>
#include <kconfiggroup.h>

#include <kdebug.h>

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
// #include "kgrfigure.h"
#include "kgrcanvas.h"
#include "kgrdialog.h"
#include "kgrgame.h"

// Shorthand for references to actions.
#define ACTION(x)   (actionCollection()->action(x))

KGoldrunner::KGoldrunner()
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
    game = new KGrGame   (view, systemDataDir, userDataDir);

    // Initialise the collections of levels (i.e. the list of games).
    if (! game->initCollections()) {
        startupOK = false;
        return;				// If no game files, abort.
    }

    kDebug() << "Calling view->setBaseScale() ...";
    view->setBaseScale();		// Set scale for level-titles font.

    // OBSOLESCENT - 9/1/09 hero = game->getHero();		// Get a pointer to the hero.

/******************************************************************************/
/*************************  SET UP THE USER INTERFACE  ************************/
/******************************************************************************/

    // Get catalog for translation
    KGlobal::locale()->insertCatalog ("libkdegames");

    // Tell the KMainWindow that this is the main widget
    setCentralWidget (view);

    // Set up our actions (menu, toolbar and keystrokes) ...
    setupActions();

    // and a status bar.
    // IDW initStatusBar();

    // Do NOT have show/hide actions for the statusbar and toolbar in the GUI:
    // we need the statusbar for game scores and the toolbar is relevant only
    // when using the game editor and then it appears automatically.  Maybe 1%
    // of players would use the game editor for 5% of their time.  Also we are
    // our own action to configure shortcut keys, so disable the KXmlGui one.
    setupGUI (static_cast<StandardWindowOption> (Default &
                        (~StatusBar) & (~ToolBar) & (~Keys))); // IDW

    // Find the theme-files and generate the Themes menu.
    setupThemes();

    initStatusBar(); // IDW

    // Connect the game actions to the menu and toolbar displays.
    connect (game, SIGNAL (quitGame()),	        SLOT (close()));
    connect (game, SIGNAL (setEditMenu (bool)),	SLOT (setEditMenu (bool)));
    connect (game, SIGNAL (markRuleType (char)), SLOT (markRuleType (char)));
    connect (game, SIGNAL (hintAvailable (bool)),	SLOT (adjustHintAction (bool)));
    connect (game, SIGNAL (defaultEditObj()),	SLOT (defaultEditObj()));

    connect (game, SIGNAL (setAvail  (const char *, const bool)),
                   SLOT   (setAvail  (const char *, const bool)));
    connect (game, SIGNAL (setToggle (const char *, const bool)),
                   SLOT   (setToggle (const char *, const bool)));

    // Apply the saved mainwindow settings, if any, and ask the mainwindow
    // to automatically save settings if changed: window size, toolbar
    // position, icon size, etc.
    setAutoSaveSettings();

    // Explicitly hide the edit toolbar - we need it in edit mode only and we
    // always start in play mode, even if the last session ended in edit mode.
    // Besides, we cannot render it until after the initial resize event (s).
    toolBar ("editToolbar")->hide();
    // IDW toolBar ("editToolbar")->setAllowedAreas (Qt::TopToolBarArea);

    // Set mouse control of the hero as the default.
    game->setMouseMode (true);

    // Do NOT paint main widget yet (title, menu, status bar, blank playfield).
    // Instead, queue a call to the "KGoldrunner_2" constructor extension.
    QMetaObject::invokeMethod (this, "KGoldrunner_2", Qt::QueuedConnection);
    kDebug() << "QMetaObject::invokeMethod (this, \"KGoldrunner_2\") done ... ";

    // Show buttons to start the config'd game and level and other options.
    game->quickStartDialog();
    kDebug() << "game->quickStartDialog() done ... ";
    kDebug() << "1st scan of event-queue ...";
}

void KGoldrunner::KGoldrunner_2()
{
    kDebug() << "Entered constructor extension ...";
    // NOW paint the main widget (title, menu, status bar, blank playfield).
    show();
    kDebug() << "Main Window show() done here ...";

    // Queue a call to the "initGame" method. This renders and paints the
    // initial graphics, but only AFTER the initial main-window resize events
    // have been seen and the final SVG scale is known.
    QMetaObject::invokeMethod (game, "initGame", Qt::QueuedConnection);
    kDebug() << "QMetaObject::invokeMethod (game, \"initGame\") done ... ";
    kDebug() << "2nd scan of event-queue ...";
}

KGoldrunner::~KGoldrunner()
{
    delete kbMapper;
}

void KGoldrunner::setupActions()
{
    /**************************************************************************/
    /******************************   GAME MENU  ******************************/
    /**************************************************************************/

    // New Game...
    // Load Saved Game...
    // --------------------------

    QAction * newAction =	KStandardGameAction::
                                gameNew (
                                game,
                                SLOT (startAnyLevel()), this);
    actionCollection()->addAction (newAction->objectName(), newAction);
    newAction->			setText (i18n ("&New Game..."));
    QAction * loadGame =	KStandardGameAction::
                                load (
                                game, SLOT (loadGame()), this);
    actionCollection()->addAction (loadGame->objectName(), loadGame);
    loadGame->			setText (i18n ("&Load Saved Game..."));

    // Save Game...
    // Save Edits... (extra copy)
    // --------------------------

    saveGame =			KStandardGameAction::
                                save (
                                game, SLOT (saveGame()), this);
    actionCollection()->addAction (saveGame->objectName(), saveGame);
    saveGame->			setText (i18n ("&Save Game..."));
    saveGame->			setShortcut (Qt::Key_S); // Alternate key.

    // Pause
    // Show High Scores
    // Get a Hint
    // Kill the Hero
    // --------------------------

    // KAction * myPause: to get KAction::shortcut() not QAction::shortcut().
    myPause = KStandardGameAction::pause (this, SLOT (stopStart()), this);
    actionCollection()->addAction (myPause->objectName(), myPause);
    // IDW KShortcut pauseShortcut = myPause->shortcut();
    // IDW QShortcut pauseShortcut = myPause->shortcut();
    // IDW pauseShortcut.setAlternate (Qt::Key_Escape);	// Add "Esc" shortcut.
    // IDW myPause->setShortcut (pauseShortcut);

    highScore = KStandardGameAction::highscores
                                (game, SLOT (showHighScores()), this);
    actionCollection()->addAction (highScore->objectName(), highScore);

    // Old hintAction = KStandardGameAction::hint
                                // Old (game, SLOT (showHint()), this);
    QSignalMapper * gameMapper = new QSignalMapper (this); // New
    connect (gameMapper, SIGNAL (mapped (int)), // New
                game, SLOT (gameActions (int))); // New

    hintAction = KStandardGameAction::hint // New
                                (gameMapper, SLOT (map()), this); // New
    actionCollection()->addAction (hintAction->objectName(), hintAction);
    gameMapper->setMapping (hintAction, HINT); // New

    killHero =	actionCollection()->addAction ("kill_hero");
    killHero->setText (i18n ("&Kill Hero"));
    killHero->setToolTip (i18n ("Kill hero"));
    killHero->setWhatsThis (i18n ("Kill the hero, in case he finds himself in "
                                "a situation from which he cannot escape"));
    killHero->setShortcut (Qt::Key_Q);
    // Old connect (killHero, SIGNAL (triggered (bool)), game, SLOT (herosDead()));
    connect (killHero, SIGNAL (triggered (bool)), gameMapper, SLOT (map())); // New NEEDED
    gameMapper->setMapping (killHero, KILL_HERO); // New

    // Quit
    // --------------------------

    KStandardGameAction::quit (this, SLOT (close()), actionCollection());

    /**************************************************************************/
    /***************************   GAME EDITOR MENU  **************************/
    /**************************************************************************/

    // Create a Level
    // Edit a Level...
    // --------------------------

    QAction* createAct = actionCollection()->addAction ("create_level");
    createAct->setText (i18n ("&Create Level"));
    createAct->setIcon (KIcon ("document-new"));
    createAct->setToolTip (i18n ("Create level"));
    createAct->setWhatsThis (i18n ("Create a completely new level"));
    connect (createAct, SIGNAL (triggered (bool)), game, SLOT (createLevel()));

    QAction* editAnyAct	= actionCollection()->addAction ("edit_any");
    editAnyAct->setText (i18n ("&Edit Level..."));
    editAnyAct->setIcon (KIcon ("document-open"));
    editAnyAct->setToolTip (i18n ("Edit level..."));
    editAnyAct->setWhatsThis (i18n ("Edit any level..."));
    connect (editAnyAct, SIGNAL (triggered (bool)), game, SLOT (updateLevel()));

    // Save Edits...
    // Move Level...
    // Delete Level...
    // --------------------------

    saveEdits =	actionCollection()->addAction ("save_edits");
    saveEdits->setText (i18n ("&Save Edits..."));
    saveEdits->setIcon (KIcon ("document-save"));
    saveEdits->setToolTip (i18n ("Save edits..."));
    saveEdits->setWhatsThis (i18n ("Save your level after editing..."));
    connect (saveEdits, SIGNAL (triggered (bool)), game, SLOT (saveLevelFile()));
    saveEdits->setEnabled (false);			// Nothing to save, yet.

    QAction* moveLevel = actionCollection()->addAction ("move_level");
    moveLevel->setText (i18n ("&Move Level..."));
    moveLevel->setToolTip (i18n ("Move level..."));
    moveLevel->setWhatsThis
                (i18n ("Change a level's number or move it to another game..."));
    connect (moveLevel, SIGNAL (triggered (bool)), game, SLOT (moveLevelFile()));

    QAction* deleteLevel = actionCollection()->addAction ("delete_level");
    deleteLevel->setText (i18n ("&Delete Level..."));
    deleteLevel->setToolTip (i18n ("Delete level..."));
    deleteLevel->setWhatsThis (i18n ("Delete a level..."));
    connect (deleteLevel,SIGNAL (triggered (bool)), game, SLOT (deleteLevelFile()));

    // Create a Game
    // Edit Game Info...
    // --------------------------

    QAction* createGame	= actionCollection()->addAction ("create_game");
    createGame->setText (i18n ("Create &Game..."));
    createGame->setToolTip (i18n ("Create game..."));
    createGame->setWhatsThis (i18n ("Create a completely new game..."));
    connect (createGame, SIGNAL (triggered (bool)), this, SLOT (createGame()));

    QAction* editGame =	actionCollection()->addAction ("edit_game");
    editGame->setText (i18n ("Edit Game &Info..."));
    editGame->setToolTip (i18n ("Edit game info..."));
    editGame->setWhatsThis
                (i18n ("Change the name, rules or description of a game..."));
    connect (editGame, SIGNAL (triggered (bool)), this, SLOT (editGameInfo()));

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

    setMouse = new KToggleAction (i18n ("&Mouse Controls Hero"), this);
    setMouse->setToolTip (i18n ("Mouse controls hero"));
    setMouse->setWhatsThis (i18n ("Use the mouse to control the hero's moves"));
    actionCollection()->addAction ("mouse_mode", setMouse);
    connect (setMouse, SIGNAL (triggered (bool)), this, SLOT (setMouseMode()));

    setKeyboard = new KToggleAction (i18n ("&Keyboard Controls Hero"), this);
    setKeyboard->setToolTip (i18n ("Keyboard controls hero"));
    setKeyboard->setWhatsThis
                        (i18n ("Use the keyboard to control the hero's moves"));
    actionCollection()->addAction ("keyboard_mode", setKeyboard);
    connect (setKeyboard,SIGNAL (triggered (bool)), this, SLOT (setKeyBoardMode()));

    QActionGroup* controlGrp = new QActionGroup (this);
    controlGrp->addAction (setMouse);
    controlGrp->addAction (setKeyboard);
    controlGrp->setExclusive (true);
    setMouse->setChecked (true);

    // Normal Speed
    // Beginner Speed
    // Champion Speed
    // Increase Speed
    // Decrease Speed
    // --------------------------

    KToggleAction * nSpeed =	new KToggleAction (
                                i18n ("Normal Speed"),
                                this);
    nSpeed->setToolTip (i18n ("Normal speed"));
    nSpeed->setWhatsThis (i18n ("Set normal game speed (12 units)"));
    actionCollection()->addAction ("normal_speed", nSpeed);
    connect (nSpeed, SIGNAL (triggered (bool)), this, SLOT (normalSpeed()));

    KToggleAction * bSpeed =	new KToggleAction (
                                i18n ("Beginner Speed"),
                                this);
    bSpeed->setToolTip (i18n ("Beginner speed"));
    bSpeed->setWhatsThis (i18n ("Set beginners' game speed (6 units)"));
    actionCollection()->addAction ("beginner_speed", bSpeed);
    connect (bSpeed, SIGNAL (triggered (bool)), this, SLOT (beginSpeed()));

    KToggleAction * cSpeed =	new KToggleAction (
                                i18n ("Champion Speed"),
                                this);
    cSpeed->setToolTip (i18n ("Champion speed"));
    cSpeed->setWhatsThis (i18n ("Set champions' game speed (18 units)"));
    actionCollection()->addAction ("champion_speed", cSpeed);
    connect (cSpeed, SIGNAL (triggered (bool)), this, SLOT (champSpeed()));

    KAction * iSpeed =	actionCollection()->addAction ("increase_speed");
    iSpeed->setText (i18n ("Increase Speed"));
    iSpeed->setToolTip (i18n ("Increase speed"));
    iSpeed->setWhatsThis (i18n ("Increase the game speed by one unit"));
    iSpeed->setShortcut (Qt::Key_Plus);
    connect (iSpeed, SIGNAL (triggered (bool)), this, SLOT (incSpeed()));

    KAction * dSpeed =	actionCollection()->addAction ("decrease_speed");
    dSpeed->setText (i18n ("Decrease Speed"));
    dSpeed->setToolTip (i18n ("Decrease speed"));
    dSpeed->setWhatsThis (i18n ("Decrease the game speed by one unit"));
    dSpeed->setShortcut (Qt::Key_Minus);
    connect (dSpeed, SIGNAL (triggered (bool)), this, SLOT (decSpeed()));

    QActionGroup* speedGrp = new QActionGroup (this);
    speedGrp->addAction (nSpeed);
    speedGrp->addAction (bSpeed);
    speedGrp->addAction (cSpeed);
    nSpeed->setChecked (true);

    // Traditional Rules
    // KGoldrunner Rules
    // --------------------------

    tradRules =			new KToggleAction (
                                i18n ("&Traditional Rules"),
                                this);
    tradRules->setToolTip (i18n ("Traditional rules"));
    tradRules->setWhatsThis (i18n ("Set Traditional rules for this game"));
    actionCollection()->addAction ("trad_rules", tradRules);
    connect (tradRules, SIGNAL (triggered (bool)), this, SLOT (setTradRules()));

    kgrRules =			new KToggleAction (
                                i18n ("K&Goldrunner Rules"),
                                this);
    kgrRules->setToolTip (i18n ("KGoldrunner rules"));
    kgrRules->setWhatsThis (i18n ("Set KGoldrunner rules for this game"));
    actionCollection()->addAction ("kgr_rules", kgrRules);
    connect (kgrRules, SIGNAL (triggered (bool)), this, SLOT (setKGrRules()));

    QActionGroup* rulesGrp = new QActionGroup (this);
    rulesGrp->addAction (tradRules);
    rulesGrp->addAction (kgrRules);
    tradRules->setChecked (true);

    // FullScreen
    fullScreen = KStandardAction::fullScreen
                        (this, SLOT (viewFullScreen (bool)), this, this);
    actionCollection()->addAction (fullScreen->objectName(), fullScreen);

#ifdef ENABLE_SOUND_SUPPORT
    setSounds = new KToggleAction (i18n ("&Play Sounds"), this);
    setSounds->setToolTip (i18n ("Play sound effects"));
    setSounds->setWhatsThis (i18n ("Play sound effects during the game"));
    actionCollection()->addAction ("options_sounds", setSounds);
    connect (setSounds, SIGNAL (triggered (bool)), game, SLOT (setPlaySounds (bool)));
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    bool soundOnOff = gameGroup.readEntry ("Sound", false);
    setSounds->setChecked (soundOnOff);
    game->setPlaySounds (soundOnOff);	// KGrGame has created a sound player.
#endif

    // Configure Shortcuts...
    // Configure Toolbars...
    // --------------------------

    KStandardAction::keyBindings (
                                this, SLOT (optionsConfigureKeys()),
                                actionCollection());
    // KStandardAction::configureToolbars (
                                // this, SLOT (optionsConfigureToolbars()),
                                // actionCollection());

    /**************************************************************************/
    /**************************   KEYSTROKE ACTIONS  **************************/
    /**************************************************************************/

    // Two-handed KB controls and alternate one-handed controls for the hero.

    kbMapper = new QSignalMapper (this);
    connect (kbMapper, SIGNAL (mapped (int)), game, SLOT(kbControl (int)));

    kbControl ("stop",       i18n ("Stop"),       Qt::Key_Space, STAND);
    kbControl ("move_right", i18n ("Move Right"), Qt::Key_Right, RIGHT);
    kbControl ("move_left",  i18n ("Move Left"),  Qt::Key_Left,  LEFT);
    kbControl ("move_up",    i18n ("Move Up"),    Qt::Key_Up,    UP);
    kbControl ("move_down",  i18n ("Move Down"),  Qt::Key_Down,  DOWN);
    kbControl ("dig_right",  i18n ("Dig Right"),  Qt::Key_C,     DIG_RIGHT);
    kbControl ("dig_left",   i18n ("Dig Left"),   Qt::Key_Z,     DIG_LEFT);

    // Alternate one-handed controls.  Set up in "kgoldrunnerui.rc".

    // Key_I, "move_up"
    // Key_L, "move_right"
    // Key_K, "move_down"
    // Key_J, "move_left"
    // Key_Space, "stop" (as above)
    // Key_O, "dig_right"
    // Key_U, "dig_left"

    setupEditToolbarActions();			// Uses pixmaps from "view".

    // Authors' debugging aids, effective when Pause is hit.  Options include
    // stepping through the animation, toggling a debug patch or log messages
    // on or off during gameplay and printing the states of runners or tiles.

    KConfigGroup debugGroup (KGlobal::config(), "Debugging");
    bool addDebuggingShortcuts = debugGroup.readEntry
                        ("DebuggingShortcuts", false);	// Get debug option.
    if (! addDebuggingShortcuts)
        return;

    KAction* step = actionCollection()->addAction ("do_step");
    step->setText (i18n ("Step"));
    step->setShortcut (Qt::Key_Period);
    connect (step, SIGNAL (triggered (bool)), game, SLOT (doStep()));
    addAction (step);

    KAction* bugFix = actionCollection()->addAction ("bug_fix");
    bugFix->setText (i18n ("Test Bug Fix"));
    bugFix->setShortcut (Qt::Key_B);
    connect (bugFix, SIGNAL (triggered (bool)), game, SLOT (bugFix()));
    addAction (bugFix);

    KAction* showPos = actionCollection()->addAction ("show_positions");
    showPos->setText (i18n ("Show Positions"));
    showPos->setShortcut (Qt::Key_D);
    connect (showPos,SIGNAL (triggered (bool)), game, SLOT (showFigurePositions()));
    addAction (showPos);

    KAction* startLog = actionCollection()->addAction ("logging");
    startLog->setText (i18n ("Start Logging"));
    startLog->setShortcut (Qt::Key_G);
    connect (startLog, SIGNAL (triggered (bool)), game, SLOT (startLogging()));
    addAction (startLog);

    KAction* showHero = actionCollection()->addAction ("show_hero");
    showHero->setText (i18n ("Show Hero"));
    showHero->setShortcut (Qt::Key_R);		// H is for Hint now.
    connect (showHero, SIGNAL (triggered (bool)), game, SLOT (showHeroState()));
    addAction (showHero);

    KAction* showObj = actionCollection()->addAction ("show_obj");
    showObj->setText (i18n ("Show Object"));
    showObj->setShortcut (Qt::Key_Question);
    connect (showObj, SIGNAL (triggered (bool)), game, SLOT (showObjectState()));
    addAction (showObj);

    KAction* showEnemy0 = actionCollection()->addAction ("show_enemy_0");
    showEnemy0->setText (i18n ("Show Enemy") + '0');
    showEnemy0->setShortcut (Qt::Key_0);
    connect (showEnemy0, SIGNAL (triggered (bool)), this, SLOT (showEnemy0()));
    addAction (showEnemy0);

    KAction* showEnemy1 = actionCollection()->addAction ("show_enemy_1");
    showEnemy1->setText (i18n ("Show Enemy") + '1');
    showEnemy1->setShortcut (Qt::Key_1);
    connect (showEnemy1, SIGNAL (triggered (bool)), this, SLOT (showEnemy1()));
    addAction (showEnemy1);

    KAction* showEnemy2 = actionCollection()->addAction ("show_enemy_2");
    showEnemy2->setText (i18n ("Show Enemy") + '2');
    showEnemy2->setShortcut (Qt::Key_2);
    connect (showEnemy2, SIGNAL (triggered (bool)), this, SLOT (showEnemy2()));
    addAction (showEnemy2);

    KAction* showEnemy3 = actionCollection()->addAction ("show_enemy_3");
    showEnemy3->setText (i18n ("Show Enemy") + '3');
    showEnemy3->setShortcut (Qt::Key_3);
    connect (showEnemy3, SIGNAL (triggered (bool)), this, SLOT (showEnemy3()));
    addAction (showEnemy3);

    KAction* showEnemy4 = actionCollection()->addAction ("show_enemy_4");
    showEnemy4->setText (i18n ("Show Enemy") + '4');
    showEnemy4->setShortcut (Qt::Key_4);
    connect (showEnemy4, SIGNAL (triggered (bool)), this, SLOT (showEnemy4()));
    addAction (showEnemy4);

    KAction* showEnemy5 = actionCollection()->addAction ("show_enemy_5");
    showEnemy5->setText (i18n ("Show Enemy") + '5');
    showEnemy5->setShortcut (Qt::Key_5);
    connect (showEnemy5, SIGNAL (triggered (bool)), this, SLOT (showEnemy5()));
    addAction (showEnemy5);

    KAction* showEnemy6 = actionCollection()->addAction ("show_enemy_6");
    showEnemy6->setText (i18n ("Show Enemy") + '6');
    showEnemy6->setShortcut (Qt::Key_6);
    connect (showEnemy6, SIGNAL (triggered (bool)), this, SLOT (showEnemy6()));
    addAction (showEnemy6);
}

void KGoldrunner::kbControl (const QString & name, const QString & text,
                             const QKeySequence & shortcut, const int dirn)
{
    // Create an action for keyboard control of KGoldrunner and connect it to
    // slot game->kbControl(int), via kbMapper and mapping-code = direction.

    KAction * a = actionCollection()->addAction (name);
    a->setText (text);
    a->setShortcut (shortcut);
    connect (a, SIGNAL (triggered (bool)), kbMapper, SLOT (map()));
    kbMapper->setMapping (a, dirn);
    addAction (a);
}

void KGoldrunner::viewFullScreen (bool activation)
{
    KToggleFullScreenAction::setFullScreen(this, activation);
}

void KGoldrunner::setupThemes()
{
    // Look for themes in files "---/share/apps/kgoldrunner/themes/*.desktop".
    KGlobal::dirs()->addResourceType ("theme", "data",
        QString (KCmdLineArgs::aboutData()->appName()) + QString ("/themes/"));

    QStringList themeFilepaths = KGlobal::dirs()->findAllResources
        ("theme", "*.desktop", KStandardDirs::NoDuplicates); // Find files.

    KConfigGroup gameGroup (KGlobal::config(), "KDEGame"); // Get prev theme.
    
    // TODO change this to a ThemeName (or simply Theme) option. The theme
    // should be searched in the themeFilepaths defined above.
    QString currentThemeFilepath = gameGroup.readEntry ("ThemeFilepath", "");
    kDebug()<< "Config() Theme" << currentThemeFilepath;

    QSignalMapper * themeMapper = new QSignalMapper (this);
    connect (themeMapper, SIGNAL (mapped (const QString &)),
                this, SLOT (changeTheme (const QString &)));

    KToggleAction * newTheme;				// Action for a theme.
    QString actionName;					// Name of the theme.
    QList<QAction *> themeList;				// Themes for menu.
    QActionGroup * themeGroup = new QActionGroup (this);
    themeGroup->setExclusive (true);			// Exclusive toggles.

    foreach (const QString &filepath, themeFilepaths) {	// Read each theme-file.
        KConfig theme (filepath, KConfig::SimpleConfig);// Extract theme-name.
        KConfigGroup group = theme.group ("KDEGameTheme");	// Translated.
        actionName = group.readEntry ("Name", i18n ("Missing Name"));

        newTheme = new KToggleAction (actionName, this);
        themeGroup->addAction (newTheme);		// Add to toggle-group.

        if ((currentThemeFilepath == filepath) ||	// If theme prev chosen
            ((currentThemeFilepath.isEmpty()) &&	// or it is the default
             (filepath.indexOf ("default") >= 0)) ||
            ((filepath == themeFilepaths.last()) &&
             (themeGroup->checkedAction() == 0))) {	// or last in the list,
            game->setInitialTheme (filepath);		// tell graphics init.
            newTheme->setChecked (true);		// and mark it as chosen
        }

        connect (newTheme, SIGNAL(triggered (bool)), themeMapper, SLOT(map()));
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
    pauseKeys = myPause->shortcut().toString(QKeySequence::NativeText);
    pauseKeys = pauseKeys.replace (';', "\" " + i18n ("or") + " \"");
    gameFreeze (false);

    statusBar()->setItemFixed (ID_LIVES, -1);		// Fix current sizes.
    statusBar()->setItemFixed (ID_SCORE, -1);
    statusBar()->setItemFixed (ID_LEVEL, -1);
    statusBar()->setItemFixed (ID_HINTAVL, -1);

    connect (game, SIGNAL (showLives (long)),	SLOT (showLives (long)));
    connect (game, SIGNAL (showScore (long)),	SLOT (showScore (long)));
    connect (game, SIGNAL (showLevel (int)),	SLOT (showLevel (int)));
    connect (game, SIGNAL (gameFreeze (bool)),	SLOT (gameFreeze (bool)));
}

void KGoldrunner::showLives (long newLives)
{
    QString tmp;
    tmp.setNum (newLives);
    if (newLives < 100)
        tmp = tmp.rightJustified (3, '0');
    tmp.insert (0, i18n ("   Lives: "));
    tmp.append ("   ");
    view->updateLives(newLives);
    statusBar()->changeItem (tmp, ID_LIVES);
}

void KGoldrunner::showScore (long newScore)
{
    QString tmp;
    tmp.setNum (newScore);
    if (newScore < 1000000)
        tmp = tmp.rightJustified (7, '0');
    tmp.insert (0, i18n ("   Score: "));
    tmp.append ("   ");
    view->updateScore(newScore);
    statusBar()->changeItem (tmp, ID_SCORE);
}

void KGoldrunner::showLevel (int newLevelNo)
{
    QString tmp;
    tmp.setNum (newLevelNo);
    if (newLevelNo < 100)
        tmp = tmp.rightJustified (3, '0');
    tmp.insert (0, i18n ("   Level: "));
    tmp.append ("   ");
    statusBar()->changeItem (tmp, ID_LEVEL);
}

void KGoldrunner::gameFreeze (bool on_off)
{
    if (on_off)
        statusBar()->changeItem
                    (i18n ("Press \"%1\" to RESUME", pauseKeys), ID_MSG);
    else
        statusBar()->changeItem
                    (i18n ("Press \"%1\" to PAUSE", pauseKeys), ID_MSG);
}

void KGoldrunner::adjustHintAction (bool hintAvailable)
{
    hintAction->setEnabled (hintAvailable);

    if (hintAvailable) {
        statusBar()->changeItem (i18n ("   Has hint   "), ID_HINTAVL);
    }
    else {
        statusBar()->changeItem (i18n ("   No hint   "), ID_HINTAVL);
    }
}

void KGoldrunner::setToggle (const char * actionName, const bool onOff)
{
    ((KToggleAction *) ACTION (actionName))->setChecked (onOff);
}


void KGoldrunner::setAvail (const char * actionName, const bool onOff)
{
    ((KAction *) ACTION (actionName))->setEnabled (onOff);
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
        toolBar ("editToolbar")->setIconSize (view->getPixmap (BRICK).size());

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

        toolBar ("editToolbar")->show();
    }
    else {
        toolBar ("editToolbar")->hide();
    }
}

void KGoldrunner::setEditIcon (const QString & actionName, const char iconType)
{
    ((KToggleAction *) (actionCollection()->action (actionName)))->
                setIcon (KIcon (view->getPixmap (iconType)));
}

/******************************************************************************/
/*******************   SLOTS FOR MENU AND KEYBOARD ACTIONS  *******************/
/******************************************************************************/

// Slot to halt (pause) or restart the game play.

void KGoldrunner::stopStart()
{
    if (! (game->isFrozen())) {
        game->freeze();
    }
    else {
        game->unfreeze();
    }
}

void KGoldrunner::changeTheme (const QString & themeFilepath)
{
    if (view->changeTheme (themeFilepath)) {
        if (game->inEditMode()) {
            setEditMenu (true);
        }
    }
    else {
        KGrMessage::information (this, i18n ("Theme Not Loaded"),
                i18n ("Cannot load the theme you selected.  It is not "
                     "in the required graphics format (SVG)."));
    }
}

// Local slots to create or edit game information.

void KGoldrunner::createGame()		{}// Force compile IDW game->editCollection (SL_CR_GAME);}
void KGoldrunner::editGameInfo()	{}// Force compile IDW game->editCollection (SL_UPD_GAME);}

// Local slots to set mouse or keyboard control of the hero.

void KGoldrunner::setMouseMode()	{game->setMouseMode (true);}
void KGoldrunner::setKeyBoardMode()	{game->setMouseMode (false);}

// Local slots to set game speed.

void KGoldrunner::normalSpeed()		{} // hero->setSpeed (NSPEED);}
void KGoldrunner::beginSpeed()		{} // hero->setSpeed (BEGINSPEED);}
void KGoldrunner::champSpeed()		{} // hero->setSpeed (CHAMPSPEED);}
void KGoldrunner::incSpeed()		{} // hero->setSpeed (+1);}
void KGoldrunner::decSpeed()		{} // hero->setSpeed (-1);}

// Slots to set Traditional or KGoldrunner rules.

void KGoldrunner::setTradRules()
{
    // KGrFigure::variableTiming = true;
    // KGrFigure::alwaysCollectNugget = true;
    // KGrFigure::runThruHole = true;
    // KGrFigure::reappearAtTop = true;
    // KGrFigure::searchStrategy = LOW;
}

void KGoldrunner::setKGrRules()
{
    // KGrFigure::variableTiming = false;
    // KGrFigure::alwaysCollectNugget = false;
    // KGrFigure::runThruHole = false;
    // KGrFigure::reappearAtTop = false;
    // KGrFigure::searchStrategy = MEDIUM;
}

// Local slots for authors' debugging aids.

void KGoldrunner::showEnemy0()		{game->showEnemyState (0);}
void KGoldrunner::showEnemy1()		{game->showEnemyState (1);}
void KGoldrunner::showEnemy2()		{game->showEnemyState (2);}
void KGoldrunner::showEnemy3()		{game->showEnemyState (3);}
void KGoldrunner::showEnemy4()		{game->showEnemyState (4);}
void KGoldrunner::showEnemy5()		{game->showEnemyState (5);}
void KGoldrunner::showEnemy6()		{game->showEnemyState (6);}

void KGoldrunner::saveProperties (KConfigGroup & /* config - unused */)
{
    // The 'config' object points to the session managed
    // config file.  Anything you write here will be available
    // later when this app is restored.

    kDebug() << "I am in KGoldrunner::saveProperties.";
}

void KGoldrunner::readProperties (const KConfigGroup & /* config - unused */)
{
    // The 'config' object points to the session managed
    // config file.  This function is automatically called whenever
    // the app is being restored.  Read in here whatever you wrote
    // in 'saveProperties'

    kDebug() << "I am in KGoldrunner::readProperties.";
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
    kDebug() << "Configure keys ...";
    // First run the standard KDE dialog for shortcut key settings.
    KShortcutsDialog::configure (actionCollection(),
	KShortcutsEditor::LetterShortcutsAllowed,	// Single letters OK.
	this,						// Parent widget.
	true);						// saveSettings value.

    // Now update the PAUSE/RESUME message in the status bar.
    kDebug() << "Update Pause/Resume message ...";
    pauseKeys = myPause->shortcut().toString(QKeySequence::NativeText);
    pauseKeys = pauseKeys.replace (';', "\" " + i18n ("or") + " \"");
    gameFreeze (game->isFrozen());	// Refresh the status bar text.
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

    // applyMainWindowSettings (KGlobal::config()->group (autoSaveGroup()));
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

void KGoldrunner::changeStatusbar (const QString& text)
{
    // display the text on the statusbar
    statusBar()->showMessage (text);
}

void KGoldrunner::changeCaption (const QString& text)
{
    // display the text on the caption
    setCaption (text);
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
        KGrMessage::information (this, i18n ("Get Folders"),
                i18n ("Cannot find documentation sub-folder 'en/%1/' "
                "in area '%2' of the KDE folder ($KDEDIRS).",
                myDir, dirs->resourceDirs ("html").join (":")));
        // result = false;		// Don't abort if the doc is missing.
    }
    else
        systemHTMLDir.append ("en/" + myDir + '/');

    // Find the system collections in a directory of the required KDE type.
    systemDataDir = dirs->findResourceDir ("data", myDir + "/system/");
    if (systemDataDir.length() <= 0) {
        KGrMessage::information (this, i18n ("Get Folders"),
        i18n ("Cannot find system games sub-folder '%1/system/' "
        "in area '%2' of the KDE folder ($KDEDIRS).",
         myDir, dirs->resourceDirs ("data").join (":")));
        result = false;			// ABORT if the games data is missing.
    }
    else
        systemDataDir.append (myDir + "/system/");

    // Locate and optionally create directories for user collections and levels.
    bool create = true;
    userDataDir   = dirs->saveLocation ("data", myDir + "/user/", create);
    if (userDataDir.length() <= 0) {
        KGrMessage::information (this, i18n ("Get Folders"),
        i18n ("Cannot find or create user games sub-folder '%1/user/' "
        "in area '%2' of the KDE user area ($KDEHOME).",
         myDir, dirs->resourceDirs ("data").join (":")));
        // result = false;		// Don't abort if user area is missing.
    }
    else {
        create = dirs->makeDir (userDataDir + "levels/");
        if (! create) {
            KGrMessage::information (this, i18n ("Get Folders"),
            i18n ("Cannot find or create 'levels/' folder in "
            "sub-folder '%1/user/' in the KDE user area ($KDEHOME).", myDir));
            // result = false;		// Don't abort if user area is missing.
        }
    }
    delete dirs;
    return (result);
}

// This method is invoked when top-level window is closed, whether by selecting
// "Quit" from the menu or by clicking the "X" at the top right of the window.

bool KGoldrunner::queryClose()
{
    // Last chance to save: user has clicked "X" widget or menu-Quit.
    // Force compile IDW bool cannotContinue = true;
    // Force compile IDW game->saveOK (cannotContinue);
    return (true);
}

/******************************************************************************/
/**********************  MAKE A TOOLBAR FOR THE EDITOR   **********************/
/******************************************************************************/

void KGoldrunner::setupEditToolbarActions()
{
    QSignalMapper * editToolbarMapper = new QSignalMapper (this); // New
    connect (editToolbarMapper, SIGNAL (mapped (int)), // New
                game, SLOT (editToolbarActions (int))); // New

    QAction* tip = actionCollection()->addAction ("edit_hint");
    tip->setIcon (KIcon ("games-hint"));
    tip->setText (i18n ("Edit Name/Hint"));
    tip->setToolTip (i18n ("Edit level name or hint"));
    tip->setWhatsThis (i18n ("Edit text for the name or hint of a level"));
    connect (tip, SIGNAL (triggered (bool)), editToolbarMapper, SLOT (map()));
    editToolbarMapper->setMapping (tip, EDIT_HINT);

    KToggleAction* free = new KToggleAction (i18n ("Erase"), this);
    free->setToolTip (i18n ("Erase"));
    free->setWhatsThis (i18n ("Erase objects by painting empty squares"));
    actionCollection()->addAction ("freebg", free);
    connect (free, SIGNAL (triggered (bool)), editToolbarMapper, SLOT (map()));
    editToolbarMapper->setMapping (free, FREE);

    KToggleAction* edhero = new KToggleAction (i18n ("Hero"), this);
    edhero->setToolTip (i18n ("Move hero"));
    edhero->setWhatsThis (i18n ("Change the hero's starting position"));
    actionCollection()->addAction ("edherobg", edhero);
    connect (edhero, SIGNAL(triggered (bool)), editToolbarMapper, SLOT(map()));
    editToolbarMapper->setMapping (edhero, HERO);

    KToggleAction* edenemy = new KToggleAction (i18n ("Enemy"), this);
    edenemy->setToolTip (i18n ("Paint enemies"));
    edenemy->setWhatsThis
                (i18n ("Paint enemies at their starting positions"));
    actionCollection()->addAction ("edenemybg", edenemy);
    connect (edenemy, SIGNAL(triggered (bool)), editToolbarMapper,SLOT(map()));
    editToolbarMapper->setMapping (edenemy, ENEMY);

    KToggleAction* brick = new KToggleAction (i18n ("Brick"), this);
    brick->setToolTip (i18n ("Paint bricks (can dig)"));
    brick->setWhatsThis (i18n ("Paint bricks (diggable objects)"));
    actionCollection()->addAction ("brickbg", brick);
    connect (brick, SIGNAL(triggered (bool)), editToolbarMapper, SLOT(map()));
    editToolbarMapper->setMapping (brick, BRICK);

    KToggleAction* beton = new KToggleAction (i18n ("Concrete"), this);
    beton->setToolTip (i18n ("Paint concrete (cannot dig)"));
    beton->setWhatsThis (i18n ("Paint concrete objects (not diggable)"));
    actionCollection()->addAction ("betonbg", beton);
    connect (beton, SIGNAL(triggered (bool)), editToolbarMapper, SLOT(map()));
    editToolbarMapper->setMapping (beton, BETON);

    KToggleAction* fbrick = new KToggleAction (i18n ("Trap"), this);
    fbrick->setToolTip
                (i18n ("Paint traps or false bricks (can fall through)"));
    fbrick->setWhatsThis
                (i18n ("Paint traps or false bricks (can fall through)"));
    actionCollection()->addAction ("fbrickbg", fbrick);
    connect (fbrick, SIGNAL(triggered (bool)), editToolbarMapper, SLOT(map()));
    editToolbarMapper->setMapping (fbrick, FBRICK);

    KToggleAction* ladder = new KToggleAction (i18n ("Ladder"), this);
    ladder->setToolTip (i18n ("Paint ladders"));
    ladder->setWhatsThis (i18n ("Paint ladders (ways to go up or down)"));
    actionCollection()->addAction ("ladderbg", ladder);
    connect (ladder, SIGNAL(triggered (bool)), editToolbarMapper, SLOT(map()));
    editToolbarMapper->setMapping (ladder, LADDER);

    KToggleAction* hladder = new KToggleAction(i18n("Hidden Ladder"),this);
    hladder->setToolTip (i18n ("Paint hidden ladders"));
    hladder->setWhatsThis
        (i18n ("Paint hidden ladders, which appear when all the gold is gone"));
    actionCollection()->addAction ("hladderbg", hladder);
    connect (hladder, SIGNAL(triggered (bool)), editToolbarMapper, SLOT(map()));
    editToolbarMapper->setMapping (hladder, HLADDER);

    KToggleAction* pole = new KToggleAction (i18n ("Bar"), this);
    pole->setToolTip (i18n ("Paint bars or poles"));
    pole->setWhatsThis (i18n("Paint bars or poles (can fall from these)"));
    actionCollection()->addAction ("polebg", pole);
    connect (pole, SIGNAL(triggered (bool)), editToolbarMapper, SLOT(map()));
    editToolbarMapper->setMapping (pole, POLE);

    KToggleAction* nugget = new KToggleAction (i18n ("Gold"), this);
    nugget->setToolTip (i18n ("Paint gold (or other treasure)"));
    nugget->setWhatsThis (i18n ("Paint gold pieces (or other treasure)"));
    actionCollection()->addAction ("nuggetbg", nugget);
    connect (nugget, SIGNAL(triggered (bool)), editToolbarMapper, SLOT(map()));
    editToolbarMapper->setMapping (nugget, NUGGET);

    QActionGroup* editButtons = new QActionGroup (this);
    editButtons->setExclusive (true);
    editButtons->addAction (free);
    editButtons->addAction (edhero);
    editButtons->addAction (edenemy);
    editButtons->addAction (brick);
    editButtons->addAction (beton);
    editButtons->addAction (fbrick);
    editButtons->addAction (ladder);
    editButtons->addAction (hladder);
    editButtons->addAction (pole);
    editButtons->addAction (nugget);

    brick->setChecked (true);
    m_defaultEditAct = brick;
}

/******************************************************************************/
/*********************   EDIT-BUTTON SLOTS   **********************************/
/******************************************************************************/

void KGoldrunner::freeSlot()
                {} // Force compile IDW game->setEditObj (FREE);     }
void KGoldrunner::edheroSlot()
                {} // Force compile IDW game->setEditObj (HERO);     }
void KGoldrunner::edenemySlot()
                {} // Force compile IDW game->setEditObj (ENEMY);    }
void KGoldrunner::brickSlot()
                {} // Force compile IDW game->setEditObj (BRICK);    }
void KGoldrunner::betonSlot()
                {} // Force compile IDW game->setEditObj (BETON);    }
void KGoldrunner::fbrickSlot()
                {} // Force compile IDW game->setEditObj (FBRICK);   }
void KGoldrunner::ladderSlot()
                {} // Force compile IDW game->setEditObj (LADDER);   }
void KGoldrunner::hladderSlot()
                {} // Force compile IDW game->setEditObj (HLADDER);  }
void KGoldrunner::poleSlot()
                {} // Force compile IDW game->setEditObj (POLE);     }
void KGoldrunner::nuggetSlot()
                {} // Force compile IDW game->setEditObj (NUGGET);   }
void KGoldrunner::defaultEditObj()
                { m_defaultEditAct->setChecked (true); }

QSize KGoldrunner::sizeHint() const
{
    kDebug() << "KGoldrunner::sizeHint() called ... 640x600";
    return QSize (640, 600);
}

#include "kgoldrunner.moc"
