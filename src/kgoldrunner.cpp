/*
    SPDX-FileCopyrightText: 2002 Marco Krüger <grisuji@gmx.de>
    SPDX-FileCopyrightText: 2002 Ian Wadham <iandw.au@gmail.com>
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kgoldrunner.h"

#include <QAction>
#include <QActionGroup>
#include <QIcon>
#include <QKeyEvent>
#include <QKeySequence>
#include <QScreen>
#include <QShortcut>

#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KIO/MkpathJob>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KGameStandardAction>
#include <KToggleAction>
#include <KToggleFullScreenAction>
#include <KToolBar>

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
    KXmlGuiWindow (nullptr)
{
/******************************************************************************/
/*************  FIND WHERE THE GAMES DATA AND HANDBOOK SHOULD BE  *************/
/******************************************************************************/

    setObjectName ( QStringLiteral("KGoldrunner" ));

    // Avoid "saveOK()" check if an error-exit occurs during the file checks.
    startupOK = true;

    // Get directory paths for the system levels, user levels and manual.
    if (! getDirectories()) {
        fprintf (stderr, "getDirectories() FAILED\n");
        startupOK = false;
        return;				// If games directory not found, abort.
    }

    // This message is to help diagnose distribution or installation problems.
    qCDebug(KGOLDRUNNER_LOG, "The games data should be in the following locations:\n"
            "System games: %s\nUser data:    %s",
            qPrintable(systemDataDir), qPrintable(userDataDir));

/******************************************************************************/
/************************  SET PLAYFIELD AND GAME DATA  ***********************/
/******************************************************************************/

    const QSize maxSize = screen()->availableGeometry().size();
    // Base the size of playing-area and widgets on the monitor resolution.
    int dw = maxSize.width();

    // Need to consider the height, for widescreen displays (eg. 1280x768).
    int dh = maxSize.height();

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
    toolBar (QStringLiteral("editToolbar"))->hide();

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

    // New Game...
    // Load Saved Game...
    // --------------------------

    QAction * a = KGameStandardAction::gameNew (this, nullptr, nullptr);
    actionCollection()->addAction (a->objectName(), a);
    connect (a, &QAction::triggered, this, [this] { game->gameActions(NEW); });
    a->setText (i18n ("&New Game..."));

    a        = gameAction (QStringLiteral("next_level"), NEXT_LEVEL,
                           i18n ("Pla&y Next Level"),
                           i18n ("Play next level."),
                           i18n ("Try the next level in the game "
                                 "you are playing."),
                           Qt::Key_Y);

    a =	KGameStandardAction::load (this, nullptr, nullptr);
    actionCollection()->addAction (a->objectName(), a);
    connect (a, &QAction::triggered, this, [this] { game->gameActions(LOAD); });
    a->setText (i18n ("&Load Saved Game..."));

    // Save Game...
    // Save Solution...
    // --------------------------

    saveGame = KGameStandardAction::save (this, nullptr, nullptr);
    actionCollection()->addAction (saveGame->objectName(), saveGame);
    connect (saveGame, &QAction::triggered, this, [this] { game->gameActions(SAVE_GAME); });
    saveGame->setText (i18n ("&Save Game..."));
    KActionCollection::setDefaultShortcut(saveGame, Qt::Key_S); // Alternate key.

    // The name of the solution-file is 'sol_<prefix>.txt', where <prefix> is
    // the unique prefix belonging to the game involved (eg. plws, tute, etc.).
    a        = gameAction (QStringLiteral("save_solution"), SAVE_SOLUTION,
                           i18n ("Save A Solution..."),
                           i18n ("Save A Solution..."),
                           i18n ("Save a solution for a level into a file "
                                 "called 'sol_&lt;prefix&gt;.txt' in your "
				 "user's data directory..."),
                           {Qt::ShiftModifier | Qt::Key_S});

    // Pause
    // Show High Scores
    // Get a Hint
    // Kill the Hero
    // --------------------------

    myPause = KGameStandardAction::pause (this, nullptr, nullptr);
    actionCollection()->addAction (myPause->objectName(), myPause);
    connect (myPause, &QAction::triggered, this, [this] { game->gameActions(PAUSE); });
    // QAction * myPause gets QAction::shortcut(), returning 1 OR 2 shortcuts.
    QList<QKeySequence> pauseShortcut = { myPause->shortcut(), Qt::Key_Escape };
    myPause->setShortcuts (pauseShortcut);

    highScore = KGameStandardAction::highscores (this, nullptr, nullptr);
    actionCollection()->addAction (highScore->objectName(), highScore);
    connect (highScore, &QAction::triggered, this, [this] { game->gameActions(HIGH_SCORE); });

    hintAction = KGameStandardAction::hint (this, nullptr, nullptr);
    actionCollection()->addAction (hintAction->objectName(), hintAction);
    connect (hintAction, &QAction::triggered, this, [this] { game->gameActions(HINT); });

    a = KGameStandardAction::demo (this, nullptr, nullptr);
    actionCollection()->addAction (a->objectName(), a);
    connect (a, &QAction::triggered, this, [this] { game->gameActions(DEMO); });

    a = KGameStandardAction::solve (this, nullptr, nullptr);
    actionCollection()->addAction (a->objectName(), a);
    connect (a, &QAction::triggered, this, [this] { game->gameActions(SOLVE); });
    a->setText      (i18n ("&Show a Solution"));
    a->setToolTip   (i18n ("Show how to win this level."));
    a->setWhatsThis (i18n ("Play a recording of how to win this level, if "
                           "there is one available."));

    a        = gameAction (QStringLiteral("instant_replay"), INSTANT_REPLAY,
                           i18n ("&Instant Replay"),
                           i18n ("Instant replay."),
                           i18n ("Show a recording of the level "
                                 "you are currently playing."),
                           Qt::Key_R);

    a        = gameAction (QStringLiteral("replay_last"), REPLAY_LAST,
                           i18n ("Replay &Last Level"),
                           i18n ("Replay last level."),
                           i18n ("Show a recording of the last level you "
                                 "played and finished, regardless of whether "
                                 "you won or lost."),
                           Qt::Key_A);

    a        = gameAction (QStringLiteral("replay_any"), REPLAY_ANY,
                           i18n ("&Replay Any Level"),
                           i18n ("Replay any level."),
                           i18n ("Show a recording of any level you have "
                                 "played so far."),
                           QKeySequence());	// No key assigned.

    killHero = gameAction (QStringLiteral("kill_hero"), KILL_HERO,
                           i18n ("&Kill Hero"),
                           i18n ("Kill Hero."),
                           i18n ("Kill the hero, in case he finds himself in "
                                 "a situation from which he cannot escape."),
                           Qt::Key_Q);

    // Quit
    // --------------------------

    KGameStandardAction::quit (this, &QWidget::close, actionCollection());

    /**************************************************************************/
    /***************************   GAME EDITOR MENU  **************************/
    /**************************************************************************/
    // Create a Level
    // Edit a Level...
    // --------------------------

    QAction * ed = editAction (QStringLiteral("create_level"), CREATE_LEVEL,
                               i18n ("&Create Level"),
                               i18n ("Create level."),
                               i18n ("Create a completely new level."));
    ed->setIcon (QIcon::fromTheme( QStringLiteral( "document-new" )));
    ed->setIconText (i18n ("Create"));

    ed           = editAction (QStringLiteral("edit_any"), EDIT_ANY,
                               i18n ("&Edit Level..."),
                               i18n ("Edit level..."),
                               i18n ("Edit any level..."));
    ed->setIcon (QIcon::fromTheme( QStringLiteral( "document-open" )));
    ed->setIconText (i18n ("Edit"));

    // Save Edits...
    // Move Level...
    // Delete Level...
    // --------------------------

    saveEdits    = editAction (QStringLiteral("save_edits"), SAVE_EDITS,
                               i18n ("&Save Edits..."),
                               i18n ("Save edits..."),
                               i18n ("Save your level after editing..."));
    saveEdits->setIcon (QIcon::fromTheme( QStringLiteral( "document-save" )));
    saveEdits->setIconText (i18n ("Save"));
    saveEdits->setEnabled (false);		// Nothing to save, yet.

    ed           = editAction (QStringLiteral("move_level"), MOVE_LEVEL,
                               i18n ("&Move Level..."),
                               i18n ("Move level..."),
                               i18n ("Change a level's number or move "
                                     "it to another game..."));

    ed           = editAction (QStringLiteral("delete_level"), DELETE_LEVEL,
                               i18n ("&Delete Level..."),
                               i18n ("Delete level..."),
                               i18n ("Delete a level..."));

    // Create a Game
    // Edit Game Info...
    // --------------------------

    ed           = editAction (QStringLiteral("create_game"), CREATE_GAME,
                               i18n ("Create &Game..."),
                               i18n ("Create game..."),
                               i18n ("Create a completely new game..."));

    ed           = editAction (QStringLiteral("edit_game"), EDIT_GAME,
                               i18n ("Edit Game &Info..."),
                               i18n ("Edit game info..."),
                               i18n ("Change the name, rules or description "
                                     "of a game..."));

    /**************************************************************************/
    /****************************   SETTINGS MENU  ****************************/
    /**************************************************************************/

    // Theme settings are handled by this class and KGrRenderer.
    QAction * themes = actionCollection()->addAction (QStringLiteral("select_theme"));
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

#ifdef KGAUDIO_BACKEND_OPENAL
    // Sound effects on/off
                                  settingAction (QStringLiteral("options_sounds"), PLAY_SOUNDS,
                                  i18n ("&Play Sounds"),
                                  i18n ("Play sound effects."),
                                  i18n ("Play sound effects during the game."));

                                  settingAction (QStringLiteral("options_steps"), PLAY_STEPS,
                                  i18n ("Play &Footstep Sounds"),
                                  i18n ("Make sounds of player's footsteps."),
                                  i18n ("Make sounds of player's footsteps."));
#endif

    // Demo at start on/off.
                                  settingAction (QStringLiteral("options_demo"), STARTUP_DEMO,
                                  i18n ("&Demo At Start"),
                                  i18n ("Run a demo when the game starts."),
                                  i18n ("Run a demo when the game starts."));

    // Mouse Controls Hero
    // Keyboard Controls Hero
    // Laptop Hybrid
    // --------------------------

    KToggleAction * setMouse    = settingAction (QStringLiteral("mouse_mode"), MOUSE,
                                  i18n ("&Mouse Controls Hero"),
                                  i18n ("Mouse controls hero."),
                                  i18n ("Use the mouse to control "
                                        "the hero's moves."));

    KToggleAction * setKeyboard = settingAction (QStringLiteral("keyboard_mode"), KEYBOARD,
                                  i18n ("&Keyboard Controls Hero"),
                                  i18n ("Keyboard controls hero."),
                                  i18n ("Use the keyboard to control "
                                        "the hero's moves."));

    KToggleAction * setLaptop   = settingAction (QStringLiteral("laptop_mode"), LAPTOP,
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

    KToggleAction * clickKey    = settingAction (QStringLiteral("click_key"), CLICK_KEY,
                                  i18n ("&Click Key To Move"),
                                  i18n ("Click Key To Move."),
                                  i18n ("In keyboard mode, click a "
                                        "direction-key to start moving "
                                        "and keep on going until you "
                                        "click another key."));

    KToggleAction * holdKey     = settingAction (QStringLiteral("hold_key"), HOLD_KEY,
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

    KToggleAction * nSpeed      = settingAction (QStringLiteral("normal_speed"), NORMAL_SPEED,
                                  i18n ("Normal Speed"),
                                  i18n ("Set normal speed."),
                                  i18n ("Set normal game speed."));

    KToggleAction * bSpeed      = settingAction (QStringLiteral("beginner_speed"),
                                  BEGINNER_SPEED,
                                  i18n ("Beginner Speed"),
                                  i18n ("Set beginners' speed."),
                                  i18n ("Set beginners' game speed "
                                        "(0.5 times normal)."));

    KToggleAction * cSpeed      = settingAction (QStringLiteral("champion_speed"),
                                  CHAMPION_SPEED,
                                  i18n ("Champion Speed"),
                                  i18n ("Set champions' speed."),
                                  i18n ("Set champions' game speed "
                                        "(1.5 times normal)."));

    a                           = gameAction (QStringLiteral("increase_speed"), INC_SPEED,
                                  i18n ("Increase Speed"),
                                  i18n ("Increase speed."),
                                  i18n ("Increase the game speed by 0.1 "
                                        "(maximum is 2.0 times normal)."),
                                  Qt::Key_Plus);

    a                           = gameAction (QStringLiteral("decrease_speed"), DEC_SPEED,
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
                                this, &KGoldrunner::optionsConfigureKeys,
                                actionCollection());

    /**************************************************************************/
    /**************************   KEYSTROKE ACTIONS  **************************/
    /**************************************************************************/

    // Two-handed KB controls and alternate one-handed controls for the hero.

    // The actions for the movement keys are created but disabled.  This lets
    // keyPressEvent() come through, instead of a signal, while still allowing
    // Settings->Configure Keys to change the key mappings.  The keyPressEvent()
    // call is needed so that the key can be identified and matched to the
    // corresponding keyReleaseEvent() call and make the hold-key option
    // work correctly when two keys are held down simultaneously.

    keyControl (QStringLiteral("stop"),       i18n ("Stop"),       Qt::Key_Space, STAND);
    keyControl (QStringLiteral("move_right"), i18n ("Move Right"), Qt::Key_Right, RIGHT, true);
    keyControl (QStringLiteral("move_left"),  i18n ("Move Left"),  Qt::Key_Left,  LEFT,  true);
    keyControl (QStringLiteral("move_up"),    i18n ("Move Up"),    Qt::Key_Up,    UP,    true);
    keyControl (QStringLiteral("move_down"),  i18n ("Move Down"),  Qt::Key_Down,  DOWN,  true);
    keyControl (QStringLiteral("dig_right"),  i18n ("Dig Right"),  Qt::Key_C,     DIG_RIGHT);
    keyControl (QStringLiteral("dig_left"),   i18n ("Dig Left"),   Qt::Key_Z,     DIG_LEFT);

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

    KConfigGroup debugGroup (KSharedConfig::openConfig(), QStringLiteral("Debugging"));
    bool addDebuggingShortcuts = debugGroup.readEntry
                        ("DebuggingShortcuts", false);	// Get debug option.
    if (! addDebuggingShortcuts)
        return;

    keyControlDebug (QStringLiteral("do_step"),      i18n ("Do a Step"), Qt::Key_Period, DO_STEP);
    keyControlDebug (QStringLiteral("bug_fix"),      i18n ("Test Bug Fix"), Qt::Key_B, BUG_FIX);
    keyControlDebug (QStringLiteral("show_positions"), i18n ("Show Positions"), Qt::Key_W, S_POSNS);
    keyControlDebug (QStringLiteral("logging"),      i18n ("Start Logging"), Qt::Key_G, LOGGING);
    keyControlDebug (QStringLiteral("show_hero"),    i18n ("Show Hero"), Qt::Key_E, S_HERO);
    keyControlDebug (QStringLiteral("show_obj"),     i18n ("Show Object"), Qt::Key_Slash, S_OBJ);

    keyControlDebug (QStringLiteral("show_enemy_0"), i18n ("Show Enemy") + QLatin1Char('0'), Qt::Key_0, ENEMY_0);
    keyControlDebug (QStringLiteral("show_enemy_1"), i18n ("Show Enemy") + QLatin1Char('1'), Qt::Key_1, ENEMY_1);
    keyControlDebug (QStringLiteral("show_enemy_2"), i18n ("Show Enemy") + QLatin1Char('2'), Qt::Key_2, ENEMY_2);
    keyControlDebug (QStringLiteral("show_enemy_3"), i18n ("Show Enemy") + QLatin1Char('3'), Qt::Key_3, ENEMY_3);
    keyControlDebug (QStringLiteral("show_enemy_4"), i18n ("Show Enemy") + QLatin1Char('4'), Qt::Key_4, ENEMY_4);
    keyControlDebug (QStringLiteral("show_enemy_5"), i18n ("Show Enemy") + QLatin1Char('5'), Qt::Key_5, ENEMY_5);
    keyControlDebug (QStringLiteral("show_enemy_6"), i18n ("Show Enemy") + QLatin1Char('6'), Qt::Key_6, ENEMY_6);
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
        KActionCollection::setDefaultShortcut(ga, key);
    }
    connect (ga, &QAction::triggered, this, [this, code] { game->gameActions(code); });
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
    connect (ed, &QAction::triggered, this, [this, code] { game->editActions(code); });
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
    connect (s, &QAction::triggered, this, [this, code] { game->settings(code); });
    return s;
}

KToggleAction * KGoldrunner::editToolbarAction (const QString & name,
                                                const char      code,
                                                const QString & shortText,
                                                const QString & text,
                                                const QString & toolTip,
                                                const QString & whatsThis)
{
    KToggleAction * ed = new KToggleAction (text, this);
    actionCollection()->addAction (name, ed);
    ed->setIconText (shortText);
    ed->setToolTip (toolTip);
    ed->setWhatsThis (whatsThis);
    connect (ed, &QAction::triggered, this, [this, code] { game->editToolbarActions(code); });
    return ed;
}

void KGoldrunner::keyControl (const QString & name, const QString & text,
                              const QKeySequence & shortcut, const int code,
                              const bool mover)
{
    QAction * a = actionCollection()->addAction (name);
    a->setText (text);
    KActionCollection::setDefaultShortcut(a, shortcut);
    a->setAutoRepeat (false);		// Avoid repeats of signals by QAction.

    // If this is a move-key, let keyPressEvent() through, instead of signal.
    if (mover) {
        a->setEnabled (false);
	addAction (a);
	return;
    }

    connect (a, &QAction::triggered, this, [this, code] { game->kbControl(code); });
    addAction (a);
}

void KGoldrunner::keyControlDebug (const QString & name, const QString & text,
                              const QKeySequence & shortcut, const int code,
                              const bool mover)
{
    QAction * a = actionCollection()->addAction (name);
    a->setText (text);
    KActionCollection::setDefaultShortcut(a, shortcut);
    a->setAutoRepeat (false);		// Avoid repeats of signals by QAction.

    // If this is a move-key, let keyPressEvent() through, instead of signal.
    if (mover) {
        a->setEnabled (false);
	addAction (a);
	return;
    }

    connect (a, &QAction::triggered, this, [this, code] { game->dbgControl(code); });
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
    if ((ACTION (QStringLiteral("move_left")))->shortcuts().contains(keystroke)) {
        dirn = LEFT;
    }
    else if ((ACTION (QStringLiteral("move_right")))->shortcuts().contains(keystroke)) {
        dirn = RIGHT;
    }
    else if ((ACTION (QStringLiteral("move_up")))->shortcuts().contains(keystroke)) {
        dirn = UP;
    }
    else if ((ACTION (QStringLiteral("move_down")))->shortcuts().contains(keystroke)) {
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

    const auto keyBindings = myPause->shortcuts();
    for (const QKeySequence &s : keyBindings) {
        pauseKeys.append(s.toString(QKeySequence::NativeText));
    }

    QString msg;
    if (on_off) {
        if (pauseKeys.isEmpty()) {
            msg = i18n("The game is paused");
        } else if (pauseKeys.size() == 1) {
            msg = i18n("Press \"%1\" to RESUME", pauseKeys.at(0));
        } else {
            msg = i18n("Press \"%1\" or \"%2\" to RESUME", pauseKeys.at(0),
                                                           pauseKeys.at(1));
        }
    } else {
        if (pauseKeys.isEmpty()) {
            msg = QString();
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
    ((KToggleAction *) ACTION (QLatin1String(actionName)))->setChecked (onOff);
}

void KGoldrunner::setAvail (const char * actionName, const bool onOff)
{
    ((QAction *) ACTION (QLatin1String(actionName)))->setEnabled (onOff);
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
        toolBar (QStringLiteral("editToolbar"))->setIconSize (scene->tileSize ());

        // Set the editToolbar icons up with pixmaps of the current theme.
        setEditIcon (QStringLiteral("brickbg"),   BRICK);
        setEditIcon (QStringLiteral("fbrickbg"),  FBRICK);
        setEditIcon (QStringLiteral("freebg"),    FREE);
        setEditIcon (QStringLiteral("nuggetbg"),  NUGGET);
        setEditIcon (QStringLiteral("polebg"),    BAR);
        setEditIcon (QStringLiteral("concretebg"), CONCRETE);
        setEditIcon (QStringLiteral("ladderbg"),  LADDER);
        setEditIcon (QStringLiteral("hladderbg"), HLADDER);
        setEditIcon (QStringLiteral("edherobg"),  HERO);
        setEditIcon (QStringLiteral("edenemybg"), ENEMY);
        setToggle   ("brickbg", true);		// Default edit-object is BRICK.

        toolBar (QStringLiteral("editToolbar"))->show();
    }
    else {
        toolBar (QStringLiteral("editToolbar"))->hide();
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
    auto *dlg = new KShortcutsDialog(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);
    dlg->addCollection(actionCollection());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &KShortcutsDialog::saved, this, [this]() {
        gameFreeze(frozen);
    });

    dlg->configure(true /* save settings */);
}

bool KGoldrunner::getDirectories()
{
    bool result = true;

    QString myDir = QStringLiteral("kgoldrunner");
    QStringList genericDataLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QStringList appDataLocations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

    // Find the system collections in a directory of the required KDE type.
    systemDataDir = QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                           QStringLiteral("system/")
                                           , QStandardPaths::LocateDirectory);
    if (systemDataDir.length() <= 0) {
        KGrMessage::information (this, i18n ("Get Folders"),
        i18n ("Cannot find system games sub-folder '/system/' "
        "in areas '%1'.",
        appDataLocations.join(QLatin1Char(';'))));
        result = false;			// ABORT if the games data is missing.
    }

    // Locate and optionally create directories for user collections and levels.
    userDataDir   = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/');
    QString levelDir = userDataDir + QStringLiteral("levels");
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
    QAction * ed = editAction (QStringLiteral("edit_hint"), EDIT_HINT,
                               i18n ("Edit Name/Hint"),
                               i18n ("Edit level name or hint"),
                               i18n ("Edit text for the name or hint "
                                     "of a level"));
    ed->setIcon (QIcon::fromTheme( QStringLiteral( "games-hint" )));
    ed->setIconText (i18n ("Name/Hint"));

    KToggleAction * free    = editToolbarAction (QStringLiteral("freebg"), FREE,
                              i18n ("Erase"), i18n ("Space/Erase"),
                              i18n ("Paint empty squares or erase"),
                              i18n ("Erase objects or paint empty squares"));

    KToggleAction * edhero  = editToolbarAction (QStringLiteral("edherobg"), HERO,
                              i18n("Hero"), i18n ("Hero"),
                              i18n ("Move hero"),
                              i18n ("Change the hero's starting position"));

    KToggleAction * edenemy = editToolbarAction (QStringLiteral("edenemybg"), ENEMY,
                              i18n ("Enemy"), i18n ("Enemy"),
                              i18n ("Paint enemies"),
                              i18n ("Paint enemies at their starting positions")
                              );

    KToggleAction * brick   = editToolbarAction (QStringLiteral("brickbg"), BRICK,
                              i18n ("Brick"), i18n ("Brick"),
                              i18n ("Paint bricks (can dig)"),
                              i18n ("Paint bricks (diggable objects)"));

    KToggleAction* concrete = editToolbarAction (QStringLiteral("concretebg"), CONCRETE,
                              i18n ("Concrete"), i18n ("Concrete"),
                              i18n ("Paint concrete (cannot dig)"),
                              i18n ("Paint concrete objects (not diggable)"));

    KToggleAction * fbrick  = editToolbarAction (QStringLiteral("fbrickbg"), FBRICK,
                              i18n ("Trap"), i18n ("Trap/False Brick"),
                              i18n ("Paint traps or false bricks "
                                    "(can fall through)"),
                              i18n ("Paint traps or false bricks "
                                    "(can fall through)"));

    KToggleAction * ladder  = editToolbarAction (QStringLiteral("ladderbg"), LADDER,
                              i18n ("Ladder"), i18n ("Ladder"),
                              i18n ("Paint ladders"),
                              i18n ("Paint ladders (ways to go up or down)"));

    KToggleAction * hladder = editToolbarAction (QStringLiteral("hladderbg"), HLADDER,
                              i18n ("H Ladder"), i18n ("Hidden Ladder"),
                              i18n ("Paint hidden ladders"),
                              i18n ("Paint hidden ladders, which appear "
                                    "when all the gold is gone"));

    KToggleAction * bar     = editToolbarAction (QStringLiteral("polebg"), BAR,
                              i18n ("Bar"), i18n ("Bar/Pole"),
                              i18n ("Paint bars or poles"),
                              i18n ("Paint bars or poles (can fall from these)")
                              );

    KToggleAction * nugget  = editToolbarAction (QStringLiteral("nuggetbg"), NUGGET,
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

#include "moc_kgoldrunner.cpp"
