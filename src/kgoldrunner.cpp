/****************************************************************************
 *    Copyright 2002  Marco Krüger <grisuji@gmx.de>                         *
 *    Copyright 2002  Ian Wadham <iandw.au@gmail.com>                       *
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

#include "kgoldrunner.h"

#include <QAction>
#include <QApplication>
#include <QCommandLineParser>
#include <QDesktopWidget>
#include <QIcon>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenuBar>
#include <QSignalMapper>
#include <QShortcut>

#include <KAboutData>
#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KIO/MkpathJob>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KStandardGameAction>
#include <KToggleAction>
#include <KToggleFullScreenAction>
#include <KToolBar>

#include "kgoldrunner_debug.h"
#include "kgoldrunner_debug.h"


#include <libkdegames_capabilities.h> //defines KGAUDIO_BACKEND_OPENAL (or not)

#include "kgrgame.h"
#include "kgrview.h"
#include "kgrscene.h"
#include "kgrrenderer.h"

// Shorthand for references to actions.
#define ACTION(x)   (actionCollection()->action(x))

KGoldrunner::KGoldrunner()
    :
    KXmlGuiWindow (0)
{
/******************************************************************************/
/*************  FIND WHERE THE GAMES DATA AND HANDBOOK SHOULD BE  *************/
/******************************************************************************/

    setObjectName ( QLatin1String("KGoldrunner" ));

    // Avoid "saveOK()" check if an error-exit occurs during the file checks.
    startupOK = true;

    // Get directory paths for the system levels, user levels and manual.
    if (! getDirectories()) {
        fprintf (stderr, "getDirectories() FAILED\n");
        startupOK = false;
        return;				// If games directory not found, abort.
    }

    // This message is to help diagnose distribution or installation problems.
    fprintf (stderr,
        "The games data and handbook should be in the following locations:\n");
    fprintf (stderr, "System games: %s\nUser data:    %s\nHandbook:     %s\n",
        qPrintable(systemDataDir), qPrintable(userDataDir), qPrintable(systemHTMLDir));

/******************************************************************************/
/************************  SET PLAYFIELD AND GAME DATA  ***********************/
/******************************************************************************/

    // Base the size of playing-area and widgets on the monitor resolution.
    int dw = QApplication::desktop()->width();

    // Need to consider the height, for widescreen displays (eg. 1280x768).
    int dh = QApplication::desktop()->height();

    dw = qMin ((4 * dh + 1) / 3, dw);	// KGoldrunner aspect ratio is 4:3.
    dh = (3 * dw + 2) / 4;

    view = new KGrView (this);
    view->setMinimumSize ((dw + 1) / 2, (dh + 1) / 2);

    game = new KGrGame (view, systemDataDir, userDataDir);

    // Initialise the lists of games (i.e. collections of levels).
    if (! game->initGameLists()) {
        startupOK = false;
        return;				// If no game files, abort.
    }

/******************************************************************************/
/*************************  SET UP THE USER INTERFACE  ************************/
/******************************************************************************/

    // Tell the KMainWindow that the KGrView object is the main widget.
    setCentralWidget (view);

    scene       = view->gameScene ();
    renderer    = scene->renderer ();

    // Set up our actions (menu, toolbar and keystrokes) ...
    setupActions();

    // Do NOT put show/hide actions for the statusbar and toolbar in the GUI.
    // We do not have a statusbar any more and the toolbar is relevant only
    // when using the game editor and then it appears automatically.  Maybe 1%
    // of players would use the game editor for 5% of their time.  Also we have
    // our own action to configure shortcut keys, so disable the KXmlGui one.
    setupGUI (static_cast<StandardWindowOption> (Default &
                        (~StatusBar) & (~ToolBar) & (~Keys)));

    // Initialize text-item lengths in the scene, before the first resize.
    scene->showLives (0);
    scene->showScore (0);
    adjustHintAction (false);
    gameFreeze (false);

    // Connect the game actions to the menu and toolbar displays.
    connect (game, &KGrGame::quitGame, this, &KGoldrunner::close);
    connect (game, &KGrGame::setEditMenu, this, &KGoldrunner::setEditMenu);
    connect (game, &KGrGame::showLives, scene, &KGrScene::showLives);
    connect (game, &KGrGame::showScore, scene, &KGrScene::showScore);
    connect (game, &KGrGame::hintAvailable, this, &KGoldrunner::adjustHintAction);
    connect (game, &KGrGame::gameFreeze, this, &KGoldrunner::gameFreeze);

    connect (game, &KGrGame::setAvail, this, &KGoldrunner::setAvail);
    connect (game, &KGrGame::setToggle, this, &KGoldrunner::setToggle);

    connect (scene, &KGrScene::redrawEditToolbar, this, &KGoldrunner::redrawEditToolbar);

    // Apply the saved mainwindow settings, if any, and ask the mainwindow
    // to automatically save settings if changed: window size, toolbar
    // position, icon size, etc.
    setAutoSaveSettings();

    // Explicitly hide the edit toolbar - we need it in edit mode only and we
    // always start in play mode, even if the last session ended in edit mode.
    // Besides, we cannot render it until after the initial resize event (s).
    toolBar ("editToolbar")->hide();

    // This is needed to make the arrow keys control the hero properly.
    setUpKeyboardControl();

    // Do NOT paint main widget yet (title bar, menu, blank playfield).
    // Instead, queue a call to the "KGoldrunner_2" constructor extension.
    QMetaObject::invokeMethod (this, "KGoldrunner_2", Qt::QueuedConnection);
    //qCDebug(KGOLDRUNNER_LOG) << "QMetaObject::invokeMethod (this, \"KGoldrunner_2\") done ... ";
    //qCDebug(KGOLDRUNNER_LOG) << "1st scan of event-queue ...";
}

void KGoldrunner::KGoldrunner_2()
{
    //qCDebug(KGOLDRUNNER_LOG) << "Entered constructor extension ...";

    // Queue a call to the "initGame" method. This renders and paints the
    // initial graphics, but only AFTER the initial main-window resize events
    // have been seen and the final SVG scale is known.
    QMetaObject::invokeMethod (game, "initGame", Qt::QueuedConnection);
    //qCDebug(KGOLDRUNNER_LOG) << "QMetaObject::invokeMethod (game, \"initGame\") done ... ";
    //qCDebug(KGOLDRUNNER_LOG) << "2nd scan of event-queue ...";
}

KGoldrunner::~KGoldrunner()
{
}

void KGoldrunner::setupActions()
{
    /**************************************************************************/
    /******************************   GAME MENU  ******************************/
    /**************************************************************************/

    QSignalMapper * gameMapper = new QSignalMapper (this);
    connect (gameMapper, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), game, &KGrGame::gameActions);
    tempMapper = gameMapper;

    // New Game...
    // Load Saved Game...
    // --------------------------

    QAction * a = KStandardGameAction::gameNew (gameMapper, SLOT(map()), this);
    actionCollection()->addAction (a->objectName(), a);
    gameMapper->setMapping (a, NEW);
    a->setText (i18n ("&New Game..."));

    a        = gameAction ("next_level", NEXT_LEVEL,
                           i18n ("Pla&y Next Level"),
                           i18n ("Play next level."),
                           i18n ("Try the next level in the game "
                                 "you are playing."),
                           Qt::Key_Y);

    a =	KStandardGameAction::load (gameMapper, SLOT(map()), this);
    actionCollection()->addAction (a->objectName(), a);
    gameMapper->setMapping (a, LOAD);
    a->setText (i18n ("&Load Saved Game..."));

    // Save Game...
    // Save Solution...
    // --------------------------

    saveGame = KStandardGameAction::save (gameMapper, SLOT(map()), this);
    actionCollection()->addAction (saveGame->objectName(), saveGame);
    gameMapper->setMapping (saveGame, SAVE_GAME);
    saveGame->setText (i18n ("&Save Game..."));
    actionCollection()->setDefaultShortcut(saveGame, Qt::Key_S); // Alternate key.

    // The name of the solution-file is 'sol_<prefix>.txt', where <prefix> is
    // the unique prefix belonging to the game involved (eg. plws, tute, etc.).
    a        = gameAction ("save_solution", SAVE_SOLUTION,
                           i18n ("Save A Solution..."),
                           i18n ("Save A Solution..."),
                           i18n ("Save a solution for a level into a file "
                                 "called 'sol_&lt;prefix&gt;.txt' in your "
				 "user's data directory..."),
                           Qt::ShiftModifier + Qt::Key_S);

    // Pause
    // Show High Scores
    // Get a Hint
    // Kill the Hero
    // --------------------------

    myPause = KStandardGameAction::pause (gameMapper, SLOT(map()), this);
    actionCollection()->addAction (myPause->objectName(), myPause);
    gameMapper->setMapping (myPause, PAUSE);
    // QAction * myPause gets QAction::shortcut(), returning 1 OR 2 shortcuts.
    QKeySequence pauseShortcut = myPause->shortcut();
    myPause->setShortcut (pauseShortcut);

    highScore = KStandardGameAction::highscores (gameMapper, SLOT(map()), this);
    actionCollection()->addAction (highScore->objectName(), highScore);
    gameMapper->setMapping (highScore, HIGH_SCORE);

    hintAction = KStandardGameAction::hint (gameMapper, SLOT (map()), this);
    actionCollection()->addAction (hintAction->objectName(), hintAction);
    gameMapper->setMapping (hintAction, HINT);

    a = KStandardGameAction::demo (gameMapper, SLOT (map()), this);
    actionCollection()->addAction (a->objectName(), a);
    gameMapper->setMapping (a, DEMO);

    a = KStandardGameAction::solve (gameMapper, SLOT (map()), this);
    actionCollection()->addAction (a->objectName(), a);
    gameMapper->setMapping (a, SOLVE);
    a->setText      (i18n ("&Show A Solution"));
    a->setToolTip   (i18n ("Show how to win this level."));
    a->setWhatsThis (i18n ("Play a recording of how to win this level, if "
                           "there is one available."));

    a        = gameAction ("instant_replay", INSTANT_REPLAY,
                           i18n ("&Instant Replay"),
                           i18n ("Instant replay."),
                           i18n ("Show a recording of the level "
                                 "you are currently playing."),
                           Qt::Key_R);

    a        = gameAction ("replay_last", REPLAY_LAST,
                           i18n ("Replay &Last Level"),
                           i18n ("Replay last level."),
                           i18n ("Show a recording of the last level you "
                                 "played and finished, regardless of whether "
                                 "you won or lost."),
                           Qt::Key_A);

    a        = gameAction ("replay_any", REPLAY_ANY,
                           i18n ("&Replay Any Level"),
                           i18n ("Replay any level."),
                           i18n ("Show a recording of any level you have "
                                 "played so far."),
                           QKeySequence());	// No key assigned.

    killHero = gameAction ("kill_hero", KILL_HERO,
                           i18n ("&Kill Hero"),
                           i18n ("Kill Hero."),
                           i18n ("Kill the hero, in case he finds himself in "
                                 "a situation from which he cannot escape."),
                           Qt::Key_Q);

    // Quit
    // --------------------------

    KStandardGameAction::quit (this, SLOT (close()), actionCollection());

    /**************************************************************************/
    /***************************   GAME EDITOR MENU  **************************/
    /**************************************************************************/

    QSignalMapper * editMapper = new QSignalMapper (this);
    connect (editMapper, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), game, &KGrGame::editActions);
    tempMapper = editMapper;

    // Create a Level
    // Edit a Level...
    // --------------------------

    QAction * ed = editAction ("create_level", CREATE_LEVEL,
                               i18n ("&Create Level"),
                               i18n ("Create level."),
                               i18n ("Create a completely new level."));
    ed->setIcon (QIcon::fromTheme( QLatin1String( "document-new" )));
    ed->setIconText (i18n ("Create"));

    ed           = editAction ("edit_any", EDIT_ANY,
                               i18n ("&Edit Level..."),
                               i18n ("Edit level..."),
                               i18n ("Edit any level..."));
    ed->setIcon (QIcon::fromTheme( QLatin1String( "document-open" )));
    ed->setIconText (i18n ("Edit"));

    // Save Edits...
    // Move Level...
    // Delete Level...
    // --------------------------

    saveEdits    = editAction ("save_edits", SAVE_EDITS,
                               i18n ("&Save Edits..."),
                               i18n ("Save edits..."),
                               i18n ("Save your level after editing..."));
    saveEdits->setIcon (QIcon::fromTheme( QLatin1String( "document-save" )));
    saveEdits->setIconText (i18n ("Save"));
    saveEdits->setEnabled (false);		// Nothing to save, yet.

    ed           = editAction ("move_level", MOVE_LEVEL,
                               i18n ("&Move Level..."),
                               i18n ("Move level..."),
                               i18n ("Change a level's number or move "
                                     "it to another game..."));

    ed           = editAction ("delete_level", DELETE_LEVEL,
                               i18n ("&Delete Level..."),
                               i18n ("Delete level..."),
                               i18n ("Delete a level..."));

    // Create a Game
    // Edit Game Info...
    // --------------------------

    ed           = editAction ("create_game", CREATE_GAME,
                               i18n ("Create &Game..."),
                               i18n ("Create game..."),
                               i18n ("Create a completely new game..."));

    ed           = editAction ("edit_game", EDIT_GAME,
                               i18n ("Edit Game &Info..."),
                               i18n ("Edit game info..."),
                               i18n ("Change the name, rules or description "
                                     "of a game..."));

    /**************************************************************************/
    /****************************   SETTINGS MENU  ****************************/
    /**************************************************************************/

    // Theme settings are handled by this class and KGrRenderer.
    QAction * themes = actionCollection()->addAction ("select_theme");
    themes->setText      (i18n ("Change &Theme..."));
    themes->setToolTip   (i18n ("Change the graphics theme..."));
    themes->setWhatsThis (i18n ("Alter the visual appearance of the runners "
                                "and background scene..."));
    connect (themes, &QAction::triggered, this, &KGoldrunner::changeTheme);

    // Show/Exit Full Screen Mode
    KToggleFullScreenAction * fullScreen = KStandardAction::fullScreen
                        (this, &KGoldrunner::viewFullScreen, this, this);
    actionCollection()->addAction (fullScreen->objectName(), fullScreen);

    // Other settings are handled by KGrGame.
    QSignalMapper * settingMapper = new QSignalMapper (this);
    connect (settingMapper, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), game, &KGrGame::settings);
    tempMapper = settingMapper;

#ifdef KGAUDIO_BACKEND_OPENAL
    // Sound effects on/off
                                  settingAction ("options_sounds", PLAY_SOUNDS,
                                  i18n ("&Play Sounds"),
                                  i18n ("Play sound effects."),
                                  i18n ("Play sound effects during the game."));

                                  settingAction ("options_steps", PLAY_STEPS,
                                  i18n ("Play &Footstep Sounds"),
                                  i18n ("Make sounds of player's footsteps."),
                                  i18n ("Make sounds of player's footsteps."));
#endif

    // Demo at start on/off.
                                  settingAction ("options_demo", STARTUP_DEMO,
                                  i18n ("&Demo At Start"),
                                  i18n ("Run a demo when the game starts."),
                                  i18n ("Run a demo when the game starts."));

    // Mouse Controls Hero
    // Keyboard Controls Hero
    // Laptop Hybrid
    // --------------------------

    KToggleAction * setMouse    = settingAction ("mouse_mode", MOUSE,
                                  i18n ("&Mouse Controls Hero"),
                                  i18n ("Mouse controls hero."),
                                  i18n ("Use the mouse to control "
                                        "the hero's moves."));

    KToggleAction * setKeyboard = settingAction ("keyboard_mode", KEYBOARD,
                                  i18n ("&Keyboard Controls Hero"),
                                  i18n ("Keyboard controls hero."),
                                  i18n ("Use the keyboard to control "
                                        "the hero's moves."));

    KToggleAction * setLaptop   = settingAction ("laptop_mode", LAPTOP,
                                  i18n ("Hybrid Control (&Laptop)"),
                                  i18n ("Pointer controls hero; dig "
                                        "using keyboard."),
                                  i18n ("Use the laptop's pointer device "
                                        "to control the hero's moves, and use "
                                        "the keyboard to dig left and right."));

    QActionGroup* controlGrp = new QActionGroup (this);
    controlGrp->addAction (setMouse);
    controlGrp->addAction (setKeyboard);
    controlGrp->addAction (setLaptop);
    controlGrp->setExclusive (true);

    // Options within keyboard mode.
    // Click key to begin moving and continue moving indefinitely.
    // Click and hold key to begin moving: release key to stop.

    KToggleAction * clickKey    = settingAction ("click_key", CLICK_KEY,
                                  i18n ("&Click Key To Move"),
                                  i18n ("Click Key To Move."),
                                  i18n ("In keyboard mode, click a "
                                        "direction-key to start moving "
                                        "and keep on going until you "
                                        "click another key."));

    KToggleAction * holdKey     = settingAction ("hold_key", HOLD_KEY,
                                  i18n ("&Hold Key To Move"),
                                  i18n ("Hold Key To Move."),
                                  i18n ("In keyboard mode, hold down a "
                                        "direction-key to move "
                                        "and release it to stop."));

    QActionGroup * keyGrp = new QActionGroup (this);
    keyGrp->addAction (clickKey);
    keyGrp->addAction (holdKey);
    keyGrp->setExclusive (true);

    // Normal Speed
    // Beginner Speed
    // Champion Speed
    // Increase Speed
    // Decrease Speed
    // --------------------------

    KToggleAction * nSpeed      = settingAction ("normal_speed", NORMAL_SPEED,
                                  i18n ("Normal Speed"),
                                  i18n ("Set normal speed."),
                                  i18n ("Set normal game speed."));

    KToggleAction * bSpeed      = settingAction ("beginner_speed",
                                  BEGINNER_SPEED,
                                  i18n ("Beginner Speed"),
                                  i18n ("Set beginners' speed."),
                                  i18n ("Set beginners' game speed "
                                        "(0.5 times normal)."));

    KToggleAction * cSpeed      = settingAction ("champion_speed",
                                  CHAMPION_SPEED,
                                  i18n ("Champion Speed"),
                                  i18n ("Set champions' speed."),
                                  i18n ("Set champions' game speed "
                                        "(1.5 times normal)."));

    a                           = gameAction ("increase_speed", INC_SPEED,
                                  i18n ("Increase Speed"),
                                  i18n ("Increase speed."),
                                  i18n ("Increase the game speed by 0.1 "
                                        "(maximum is 2.0 times normal)."),
                                  Qt::Key_Plus);

    a                           = gameAction ("decrease_speed", DEC_SPEED,
                                  i18n ("Decrease Speed"),
                                  i18n ("Decrease speed."),
                                  i18n ("Decrease the game speed by 0.1 "
                                        "(minimum is 0.2 times normal)."),
                                  Qt::Key_Minus);

    QActionGroup* speedGrp = new QActionGroup (this);
    speedGrp->addAction (nSpeed);
    speedGrp->addAction (bSpeed);
    speedGrp->addAction (cSpeed);
    speedGrp->setExclusive (true);

    // Configure Shortcuts...
    // --------------------------

    KStandardAction::keyBindings (
                                this, SLOT (optionsConfigureKeys()),
                                actionCollection());

    /**************************************************************************/
    /**************************   KEYSTROKE ACTIONS  **************************/
    /**************************************************************************/

    // Two-handed KB controls and alternate one-handed controls for the hero.

    QSignalMapper * kbMapper = new QSignalMapper (this);
    connect (kbMapper, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), [&](int dirn) { game->kbControl(dirn); } );
    tempMapper = kbMapper;

    // The actions for the movement keys are created but disabled.  This lets
    // keyPressEvent() come through, instead of a signal, while still allowing
    // Settings->Configure Keys to change the key mappings.  The keyPressEvent()
    // call is needed so that the key can be identified and matched to the
    // corresponding keyReleaseEvent() call and make the hold-key option
    // work correctly when two keys are held down simultaneously.

    keyControl ("stop",       i18n ("Stop"),       Qt::Key_Space, STAND);
    keyControl ("move_right", i18n ("Move Right"), Qt::Key_Right, RIGHT, true);
    keyControl ("move_left",  i18n ("Move Left"),  Qt::Key_Left,  LEFT,  true);
    keyControl ("move_up",    i18n ("Move Up"),    Qt::Key_Up,    UP,    true);
    keyControl ("move_down",  i18n ("Move Down"),  Qt::Key_Down,  DOWN,  true);
    keyControl ("dig_right",  i18n ("Dig Right"),  Qt::Key_C,     DIG_RIGHT);
    keyControl ("dig_left",   i18n ("Dig Left"),   Qt::Key_Z,     DIG_LEFT);

    // Alternate one-handed controls.  Set up in "kgoldrunnerui.rc".

    // Key_I, "move_up"
    // Key_L, "move_right"
    // Key_K, "move_down"
    // Key_J, "move_left"
    // Key_Space, "stop" (as above)
    // Key_O, "dig_right"
    // Key_U, "dig_left"

    setupEditToolbarActions();		// Uses pixmaps from "renderer".

    // Authors' debugging aids, effective when Pause is hit.  Options include
    // stepping through the animation, toggling a debug patch or log messages
    // on or off during gameplay and printing the states of runners or tiles.

    KConfigGroup debugGroup (KSharedConfig::openConfig(), "Debugging");
    bool addDebuggingShortcuts = debugGroup.readEntry
                        ("DebuggingShortcuts", false);	// Get debug option.
    if (! addDebuggingShortcuts)
        return;

    QSignalMapper * dbgMapper = new QSignalMapper (this);
    connect (dbgMapper, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), game, &KGrGame::dbgControl);
    tempMapper = dbgMapper;

    keyControl ("do_step",      i18n ("Do a Step"), Qt::Key_Period, DO_STEP);
    keyControl ("bug_fix",      i18n ("Test Bug Fix"), Qt::Key_B, BUG_FIX);
    keyControl ("show_positions", i18n ("Show Positions"), Qt::Key_W, S_POSNS);
    keyControl ("logging",      i18n ("Start Logging"), Qt::Key_G, LOGGING);
    keyControl ("show_hero",    i18n ("Show Hero"), Qt::Key_E, S_HERO);
    keyControl ("show_obj",     i18n ("Show Object"), Qt::Key_Slash, S_OBJ);

    keyControl ("show_enemy_0", i18n ("Show Enemy") + '0', Qt::Key_0, ENEMY_0);
    keyControl ("show_enemy_1", i18n ("Show Enemy") + '1', Qt::Key_1, ENEMY_1);
    keyControl ("show_enemy_2", i18n ("Show Enemy") + '2', Qt::Key_2, ENEMY_2);
    keyControl ("show_enemy_3", i18n ("Show Enemy") + '3', Qt::Key_3, ENEMY_3);
    keyControl ("show_enemy_4", i18n ("Show Enemy") + '4', Qt::Key_4, ENEMY_4);
    keyControl ("show_enemy_5", i18n ("Show Enemy") + '5', Qt::Key_5, ENEMY_5);
    keyControl ("show_enemy_6", i18n ("Show Enemy") + '6', Qt::Key_6, ENEMY_6);
}

QAction * KGoldrunner::gameAction (const QString & name,
                                   const int       code,
                                   const QString & text,
                                   const QString & toolTip,
                                   const QString & whatsThis,
                                   const QKeySequence & key)
{
    QAction * ga = actionCollection()->addAction (name);
    ga->setText (text);
    ga->setToolTip (toolTip);
    ga->setWhatsThis (whatsThis);
    if (! key.isEmpty()) {
        actionCollection()->setDefaultShortcut(ga, key);
    }
    connect (ga, &QAction::triggered, tempMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    tempMapper->setMapping (ga, code);
    return ga;
}

QAction * KGoldrunner::editAction (const QString & name,
                                   const int       code,
                                   const QString & text,
                                   const QString & toolTip,
                                   const QString & whatsThis)
{
    QAction * ed = actionCollection()->addAction (name);
    ed->setText (text);
    ed->setToolTip (toolTip);
    ed->setWhatsThis (whatsThis);
    connect (ed, &QAction::triggered, tempMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    tempMapper->setMapping (ed, code);
    return ed;
}

KToggleAction * KGoldrunner::settingAction (const QString & name,
                                            const int       code,
                                            const QString & text,
                                            const QString & toolTip,
                                            const QString & whatsThis)
{
    KToggleAction * s = new KToggleAction (text, this);
    actionCollection()->addAction (name, s);
    s->setToolTip (toolTip);
    s->setWhatsThis (whatsThis);
    connect (s, &QAction::triggered, tempMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    tempMapper->setMapping (s, code);
    return s;
}

KToggleAction * KGoldrunner::editToolbarAction (const QString & name,
                                                const char      code,
                                                const QString & shortText,
                                                const QString & text,
                                                const QString & toolTip,
                                                const QString & whatsThis)
{
    int mapCode = code;
    KToggleAction * ed = new KToggleAction (text, this);
    actionCollection()->addAction (name, ed);
    ed->setIconText (shortText);
    ed->setToolTip (toolTip);
    ed->setWhatsThis (whatsThis);
    connect (ed, &QAction::triggered, tempMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    tempMapper->setMapping (ed, mapCode);
    return ed;
}

void KGoldrunner::keyControl (const QString & name, const QString & text,
                              const QKeySequence & shortcut, const int code,
                              const bool mover)
{
    QAction * a = actionCollection()->addAction (name);
    a->setText (text);
    actionCollection()->setDefaultShortcut(a, shortcut);
    a->setAutoRepeat (false);		// Avoid repeats of signals by QAction.

    // If this is a move-key, let keyPressEvent() through, instead of signal.
    if (mover) {
        a->setEnabled (false);
	addAction (a);
	return;
    }

    connect (a, &QAction::triggered, tempMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    tempMapper->setMapping (a, code);
    addAction (a);
}

void KGoldrunner::setUpKeyboardControl()
{
    // This is needed to ensure that the press and release of Up, Down, Left and
    // Right keys (arrow-keys) are all received as required.
    //
    // If the KGoldrunner widget does not have the keyboard focus, arrow-keys
    // provide only key-release events, which do not control the hero properly.
    // Other keys provide both press and release events, regardless of focus.

    this->setFocusPolicy (Qt::StrongFocus); // Tab or click gets the focus.
    view->setFocusProxy (this);		    // So does a click on the play-area.
    this->setFocus (Qt::OtherFocusReason);  // And we start by having the focus.
}

void KGoldrunner::keyPressEvent (QKeyEvent * event)
{
    // For movement keys, all presses and releases are processed, thus allowing
    // the hold-key option to work correctly when two keys are held down.

    if (! identifyMoveAction (event, true)) {
        QWidget::keyPressEvent (event);
    }
}

void KGoldrunner::keyReleaseEvent (QKeyEvent * event)
{
    if (! identifyMoveAction (event, false)) {
        QWidget::keyReleaseEvent (event);
    }
}

bool KGoldrunner::identifyMoveAction (QKeyEvent * event, bool pressed)
{
    if (event->isAutoRepeat()) {
        return false;		// Use only the release and the initial press.
    }
    Direction dirn = STAND;
    // The arrow keys show the "Keypad" modifier as being set, even if the
    // computer does NOT have a keypad (see Qt::KeypadModifier doco). It is
    // OK to ignore the Keypad modifier (see the code for "qevent.cpp" at
    // "bool QKeyEvent::matches(QKeySequence::StandardKey matchKey)"). The
    // keys on the keypad usually have equivalents on the main keyboard.
    QKeySequence keystroke (~(Qt::KeypadModifier) &
                             (event->key() | event->modifiers()));
    if ((ACTION ("move_left"))->shortcuts().contains(keystroke)) {
        dirn = LEFT;
    }
    else if ((ACTION ("move_right"))->shortcuts().contains(keystroke)) {
        dirn = RIGHT;
    }
    else if ((ACTION ("move_up"))->shortcuts().contains(keystroke)) {
        dirn = UP;
    }
    else if ((ACTION ("move_down"))->shortcuts().contains(keystroke)) {
        dirn = DOWN;
    }
    else {
        return false;
    }
    // Use this event to control the hero, if KEYBOARD mode is selected.
    game->kbControl (dirn, pressed);
    return true;
}

void KGoldrunner::viewFullScreen (bool activation)
{
    KToggleFullScreenAction::setFullScreen (this, activation);
}

void KGoldrunner::gameFreeze (bool on_off)
{
    myPause->setChecked (on_off);
    frozen = on_off;	// Remember the state (for the configure-keys case).
    QStringList pauseKeys;

    const auto keyBindings = myPause->shortcut().keyBindings(QKeySequence::StandardKey::Cancel);
    for (const QKeySequence &s : keyBindings) {
        pauseKeys.append(s.toString(QKeySequence::NativeText));
    }

    QString msg;
    if (on_off) {
        if (pauseKeys.size() == 0) {
            msg = i18n("The game is paused");
        } else if (pauseKeys.size() == 1) {
            msg = i18n("Press \"%1\" to RESUME", pauseKeys.at(0));
        } else {
            msg = i18n("Press \"%1\" or \"%2\" to RESUME", pauseKeys.at(0),
                                                           pauseKeys.at(1));
        }
    } else {
        if (pauseKeys.size() == 0) {
            msg = "";
        } else if (pauseKeys.size() == 1) {
            msg = i18n("Press \"%1\" to PAUSE", pauseKeys.at(0));
        } else {
            msg = i18n("Press \"%1\" or \"%2\" to PAUSE", pauseKeys.at(0),
                                                          pauseKeys.at(1));
        }
    }
    scene->setPauseResumeText (msg);
}

void KGoldrunner::adjustHintAction (bool hintAvailable)
{
    hintAction->setEnabled (hintAvailable);

    QString msg;
    msg = hintAvailable ? i18n("Has hint") : i18n("No hint");
    scene->setHasHintText (msg);
}

void KGoldrunner::setToggle (const char * actionName, const bool onOff)
{
    ((KToggleAction *) ACTION (actionName))->setChecked (onOff);
}

void KGoldrunner::setAvail (const char * actionName, const bool onOff)
{
    ((QAction *) ACTION (actionName))->setEnabled (onOff);
}

void KGoldrunner::setEditMenu (bool on_off)
{
    saveEdits->setEnabled  (on_off);

    saveGame->setEnabled   (! on_off);
    hintAction->setEnabled (! on_off);
    killHero->setEnabled   (! on_off);
    highScore->setEnabled  (! on_off);
    setAvail ("instant_replay", (! on_off));
    setAvail ("game_pause",     (! on_off));

    if (on_off){
        // Set the editToolbar icons to the current tile-size.
        //qCDebug(KGOLDRUNNER_LOG) << "ToolBar icon size:" << scene->tileSize ();
        toolBar ("editToolbar")->setIconSize (scene->tileSize ());

        // Set the editToolbar icons up with pixmaps of the current theme.
        setEditIcon ("brickbg",   BRICK);
        setEditIcon ("fbrickbg",  FBRICK);
        setEditIcon ("freebg",    FREE);
        setEditIcon ("nuggetbg",  NUGGET);
        setEditIcon ("polebg",    BAR);
        setEditIcon ("concretebg", CONCRETE);
        setEditIcon ("ladderbg",  LADDER);
        setEditIcon ("hladderbg", HLADDER);
        setEditIcon ("edherobg",  HERO);
        setEditIcon ("edenemybg", ENEMY);
        setToggle   ("brickbg", true);		// Default edit-object is BRICK.

        toolBar ("editToolbar")->show();
    }
    else {
        toolBar ("editToolbar")->hide();
    }
}

void KGoldrunner::setEditIcon (const QString & actionName, const char iconType)
{
    ((KToggleAction *) (actionCollection()->action (actionName)))->
                setIcon (QIcon(renderer->getPixmap (iconType)));
}

/******************************************************************************/
/*******************   SLOTS FOR MENU AND KEYBOARD ACTIONS  *******************/
/******************************************************************************/

void KGoldrunner::changeTheme ()
{
    renderer->selectTheme ();
}

void KGoldrunner::redrawEditToolbar ()
{
    // Signalled by the scene after the theme or tile size has changed.
    if (game->inEditMode()) {
        setEditMenu (true);
    }
}

void KGoldrunner::saveProperties (KConfigGroup & /* config - unused */)
{
    // The 'config' object points to the session managed
    // config file.  Anything you write here will be available
    // later when this app is restored.

    //qCDebug(KGOLDRUNNER_LOG) << "I am in KGoldrunner::saveProperties.";
}

void KGoldrunner::readProperties (const KConfigGroup & /* config - unused */)
{
    // The 'config' object points to the session managed
    // config file.  This function is automatically called whenever
    // the app is being restored.  Read in here whatever you wrote
    // in 'saveProperties'

    //qCDebug(KGOLDRUNNER_LOG) << "I am in KGoldrunner::readProperties.";
}

void KGoldrunner::optionsConfigureKeys()
{
    // First run the standard KDE dialog for shortcut key settings.
    KShortcutsDialog::configure (actionCollection(),
	KShortcutsEditor::LetterShortcutsAllowed,	// Single letters OK.
	this,						// Parent widget.
	true);						// saveSettings value.

    gameFreeze (frozen);		// Update the pause/resume text.
}

bool KGoldrunner::getDirectories()
{
    bool result = true;

    QString myDir = "kgoldrunner";
    QStringList genericDataLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QStringList appDataLocations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

    // Find the KGoldrunner Users' Guide, English version (en).
    systemHTMLDir = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                       "doc/HTML/en/" + myDir + '/',
                                       QStandardPaths::LocateDirectory);
    if (systemHTMLDir.length() <= 0) {
        KGrMessage::information (this, i18n ("Get Folders"),
                i18n ("Cannot find documentation sub-folder 'doc/HTML/en/%1/' "
                "in areas '%2'.", myDir, genericDataLocations.join(";")));
        // result = false;		// Don't abort if the doc is missing.
    }
    else
        systemHTMLDir.append ("en/" + myDir + '/');

    // Find the system collections in a directory of the required KDE type.
    systemDataDir = QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                           "system/", QStandardPaths::LocateDirectory);
    if (systemDataDir.length() <= 0) {
        KGrMessage::information (this, i18n ("Get Folders"),
        i18n ("Cannot find system games sub-folder '/system/' "
        "in areas '%1'.",
        appDataLocations.join(";")));
        result = false;			// ABORT if the games data is missing.
    }

    // Locate and optionally create directories for user collections and levels.
    userDataDir   = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/";
    QString levelDir = userDataDir + "levels";
    KIO::mkpath(QUrl::fromUserInput(levelDir))->exec();

    return (result);
}

// This method is invoked when the main window is closed, whether by selecting
// "Quit" from the menu or by clicking the "X" at the top right of the window.

// If we return true, game-edits were saved or abandoned or there was no editing
// in progress, so the main window will close.  If we return false, the user has
// asked to go on editing, so the main window stays open.

bool KGoldrunner::queryClose()
{
    return (game->saveOK());
}

/******************************************************************************/
/**********************  MAKE A TOOLBAR FOR THE EDITOR   **********************/
/******************************************************************************/

void KGoldrunner::setupEditToolbarActions()
{
    QSignalMapper * editToolbarMapper = new QSignalMapper (this);
    connect (editToolbarMapper, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), game, &KGrGame::editToolbarActions);
    tempMapper = editToolbarMapper;

    QAction * ed = editAction ("edit_hint", EDIT_HINT,
                               i18n ("Edit Name/Hint"),
                               i18n ("Edit level name or hint"),
                               i18n ("Edit text for the name or hint "
                                     "of a level"));
    ed->setIcon (QIcon::fromTheme( QLatin1String( "games-hint" )));
    ed->setIconText (i18n ("Name/Hint"));

    KToggleAction * free    = editToolbarAction ("freebg", FREE,
                              i18n ("Erase"), i18n ("Space/Erase"),
                              i18n ("Paint empty squares or erase"),
                              i18n ("Erase objects or paint empty squares"));

    KToggleAction * edhero  = editToolbarAction ("edherobg", HERO,
                              i18n("Hero"), i18n ("Hero"),
                              i18n ("Move hero"),
                              i18n ("Change the hero's starting position"));

    KToggleAction * edenemy = editToolbarAction ("edenemybg", ENEMY,
                              i18n ("Enemy"), i18n ("Enemy"),
                              i18n ("Paint enemies"),
                              i18n ("Paint enemies at their starting positions")
                              );

    KToggleAction * brick   = editToolbarAction ("brickbg", BRICK,
                              i18n ("Brick"), i18n ("Brick"),
                              i18n ("Paint bricks (can dig)"),
                              i18n ("Paint bricks (diggable objects)"));

    KToggleAction* concrete = editToolbarAction ("concretebg", CONCRETE,
                              i18n ("Concrete"), i18n ("Concrete"),
                              i18n ("Paint concrete (cannot dig)"),
                              i18n ("Paint concrete objects (not diggable)"));

    KToggleAction * fbrick  = editToolbarAction ("fbrickbg", FBRICK,
                              i18n ("Trap"), i18n ("Trap/False Brick"),
                              i18n ("Paint traps or false bricks "
                                    "(can fall through)"),
                              i18n ("Paint traps or false bricks "
                                    "(can fall through)"));

    KToggleAction * ladder  = editToolbarAction ("ladderbg", LADDER,
                              i18n ("Ladder"), i18n ("Ladder"),
                              i18n ("Paint ladders"),
                              i18n ("Paint ladders (ways to go up or down)"));

    KToggleAction * hladder = editToolbarAction ("hladderbg", HLADDER,
                              i18n ("H Ladder"), i18n ("Hidden Ladder"),
                              i18n ("Paint hidden ladders"),
                              i18n ("Paint hidden ladders, which appear "
                                    "when all the gold is gone"));

    KToggleAction * bar     = editToolbarAction ("polebg", BAR,
                              i18n ("Bar"), i18n ("Bar/Pole"),
                              i18n ("Paint bars or poles"),
                              i18n ("Paint bars or poles (can fall from these)")
                              );

    KToggleAction * nugget  = editToolbarAction ("nuggetbg", NUGGET,
                              i18n ("Gold"), i18n ("Gold/Treasure"),
                              i18n ("Paint gold (or other treasure)"),
                              i18n ("Paint gold pieces (or other treasure)"));

    QActionGroup* editButtons = new QActionGroup (this);
    editButtons->setExclusive (true);
    editButtons->addAction (free);
    editButtons->addAction (edhero);
    editButtons->addAction (edenemy);
    editButtons->addAction (brick);
    editButtons->addAction (concrete);
    editButtons->addAction (fbrick);
    editButtons->addAction (ladder);
    editButtons->addAction (hladder);
    editButtons->addAction (bar);
    editButtons->addAction (nugget);

    brick->setChecked (true);
}

QSize KGoldrunner::sizeHint() const
{
    return QSize (640, 600);
}


