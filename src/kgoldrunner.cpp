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

#include <QApplication>
#include <QDesktopWidget>
#include <QSignalMapper>
#include <QShortcut>
#include <QKeySequence>
#include <QKeyEvent>

#include <kglobal.h>
#include <kstatusbar.h>
#include <kshortcutsdialog.h>
#include <KStandardDirs>

#include <kconfig.h>
#include <kconfiggroup.h>

#include <kdebug.h>
#include <QDebug>

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
    // int dw = QApplication::desktop()->width();

    // Need to consider the height, for widescreen displays (eg. 1280x768).
    // int dh = QApplication::desktop()->height();

    /*  double scale = 1.0;
    if ((dw > 800) && (dh > 600)) {			// More than 800x600.
        scale = 1.25;			// Scale 1.25:1.
    }
    if ((dw > 1024) && (dh > 768))  {			// More than 1024x768.
        scale = 1.75;			// Scale 1.75:1.
    } */

    view = new KGrView (this);
    game = new KGrGame (view, systemDataDir, userDataDir);

    // Initialise the lists of games (i.e. collections of levels).
    if (! game->initGameLists()) {
        startupOK = false;
        return;				// If no game files, abort.
    }

/******************************************************************************/
/*************************  SET UP THE USER INTERFACE  ************************/
/******************************************************************************/

    // Get catalog for translation.
    KGlobal::locale()->insertCatalog ( QLatin1String( "libkdegames" ));

    // Tell the KMainWindow that the KGrCanvas object is the main widget.
    setCentralWidget (view);

    scene       = view->gameScene ();
    renderer    = scene->renderer ();

    // Set up our actions (menu, toolbar and keystrokes) ...
    setupActions();

    // Do NOT have show/hide actions for the statusbar and toolbar in the GUI:
    // we need the statusbar for game scores and the toolbar is relevant only
    // when using the game editor and then it appears automatically.  Maybe 1%
    // of players would use the game editor for 5% of their time.  Also we have
    // our own action to configure shortcut keys, so disable the KXmlGui one.
    setupGUI (static_cast<StandardWindowOption> (Default &
                        (~StatusBar) & (~ToolBar) & (~Keys)));

    // Set up a status bar (after setupGUI(), to show USER's choice of keys).
    initStatusBar();

    // Connect the game actions to the menu and toolbar displays.
    connect (game, SIGNAL (quitGame()),	         SLOT (close()));
    // connect (game, SIGNAL (setEditMenu(bool)),	 SLOT (setEditMenu(bool)));
    connect (game, SIGNAL (hintAvailable(bool)), SLOT (adjustHintAction(bool)));

    connect (game, SIGNAL (setAvail(const char*,bool)),
                   SLOT   (setAvail(const char*,bool)));
    connect (game, SIGNAL (setToggle(const char*,bool)),
                   SLOT   (setToggle(const char*,bool)));

    // Apply the saved mainwindow settings, if any, and ask the mainwindow
    // to automatically save settings if changed: window size, toolbar
    // position, icon size, etc.
    setAutoSaveSettings();

    // Explicitly hide the edit toolbar - we need it in edit mode only and we
    // always start in play mode, even if the last session ended in edit mode.
    // Besides, we cannot render it until after the initial resize event (s).
    toolBar ("editToolbar")->hide();

    // Do NOT paint main widget yet (title, menu, status bar, blank playfield).
    // Instead, queue a call to the "KGoldrunner_2" constructor extension.
    QMetaObject::invokeMethod (this, "KGoldrunner_2", Qt::QueuedConnection);
    kDebug() << "QMetaObject::invokeMethod (this, \"KGoldrunner_2\") done ... ";
    kDebug() << "1st scan of event-queue ...";
}

void KGoldrunner::KGoldrunner_2()
{
    kDebug() << "Entered constructor extension ...";

    // Queue a call to the "initGame" method. This renders and paints the
    // initial graphics, but only AFTER the initial main-window resize events
    // have been seen and the final SVG scale is known.
    QMetaObject::invokeMethod (game, "initGame", Qt::QueuedConnection);
    kDebug() << "QMetaObject::invokeMethod (game, \"initGame\") done ... ";
    kDebug() << "2nd scan of event-queue ...";
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
    connect (gameMapper, SIGNAL (mapped(int)), game, SLOT (gameActions(int)));
    tempMapper = gameMapper;

    // New Game...
    // Load Saved Game...
    // --------------------------

    KAction * a = KStandardGameAction::gameNew (gameMapper, SLOT(map()), this);
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
    // Save Edits... (extra copy)
    // --------------------------

    saveGame = KStandardGameAction::save (gameMapper, SLOT(map()), this);
    actionCollection()->addAction (saveGame->objectName(), saveGame);
    gameMapper->setMapping (saveGame, SAVE_GAME);
    saveGame->setText (i18n ("&Save Game..."));
    saveGame->setShortcut (Qt::Key_S); // Alternate key.

    // Pause
    // Show High Scores
    // Get a Hint
    // Kill the Hero
    // --------------------------

    myPause = KStandardGameAction::pause (gameMapper, SLOT(map()), this);
    actionCollection()->addAction (myPause->objectName(), myPause);
    gameMapper->setMapping (myPause, PAUSE);

    // KAction * myPause gets KAction::shortcut(), returning 1 OR 2 shortcuts.
    KShortcut pauseShortcut = myPause->shortcut();
    pauseShortcut.setAlternate (Qt::Key_Escape);	// Add "Esc" shortcut.
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
    // connect (editMapper, SIGNAL (mapped(int)), game, SLOT (editActions(int)));
    tempMapper = editMapper;

    // Create a Level
    // Edit a Level...
    // --------------------------

    KAction * ed = editAction ("create_level", CREATE_LEVEL,
                               i18n ("&Create Level"),
                               i18n ("Create level."),
                               i18n ("Create a completely new level."));
    ed->setIcon (KIcon ( QLatin1String( "document-new" )));
    ed->setIconText (i18n ("Create"));

    ed           = editAction ("edit_any", EDIT_ANY,
                               i18n ("&Edit Level..."),
                               i18n ("Edit level..."),
                               i18n ("Edit any level..."));
    ed->setIcon (KIcon ( QLatin1String( "document-open" )));
    ed->setIconText (i18n ("Edit"));

    // Save Edits...
    // Move Level...
    // Delete Level...
    // --------------------------

    saveEdits    = editAction ("save_edits", SAVE_EDITS,
                               i18n ("&Save Edits..."),
                               i18n ("Save edits..."),
                               i18n ("Save your level after editing..."));
    saveEdits->setIcon (KIcon ( QLatin1String( "document-save" )));
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
    /*****************************   THEMES MENU  *****************************/
    /**************************************************************************/

    QAction * themes = actionCollection()->addAction ("select_theme");
    themes->setText      (i18n ("Change &Theme..."));
    themes->setToolTip   (i18n ("Change the graphics theme..."));
    themes->setWhatsThis (i18n ("Alter the visual appearance of the runners "
                                "and background scene..."));
    connect (themes, SIGNAL (triggered (bool)), this, SLOT (changeTheme ()));

    /**************************************************************************/
    /****************************   SETTINGS MENU  ****************************/
    /**************************************************************************/

    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    QSignalMapper * settingMapper = new QSignalMapper (this);
    connect (settingMapper, SIGNAL (mapped(int)), game, SLOT (settings(int)));
    tempMapper = settingMapper;

    // Show/Exit Full Screen Mode
    KToggleFullScreenAction * fullScreen = KStandardAction::fullScreen
                        (this, SLOT (viewFullScreen(bool)), this, this);
    actionCollection()->addAction (fullScreen->objectName(), fullScreen);

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
    connect (kbMapper, SIGNAL (mapped(int)), game, SLOT(kbControl(int)));
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

    // setupEditToolbarActions();		// Uses pixmaps from "view".

    // Authors' debugging aids, effective when Pause is hit.  Options include
    // stepping through the animation, toggling a debug patch or log messages
    // on or off during gameplay and printing the states of runners or tiles.

    KConfigGroup debugGroup (KGlobal::config(), "Debugging");
    bool addDebuggingShortcuts = debugGroup.readEntry
                        ("DebuggingShortcuts", false);	// Get debug option.
    if (! addDebuggingShortcuts)
        return;

    QSignalMapper * dbgMapper = new QSignalMapper (this);
    connect (dbgMapper, SIGNAL (mapped(int)), game, SLOT(dbgControl(int)));
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

KAction * KGoldrunner::gameAction (const QString & name,
                                   const int       code,
                                   const QString & text,
                                   const QString & toolTip,
                                   const QString & whatsThis,
                                   const QKeySequence & key)
{
    KAction * ga = actionCollection()->addAction (name);
    ga->setText (text);
    ga->setToolTip (toolTip);
    ga->setWhatsThis (whatsThis);
    if (! key.isEmpty()) {
        ga->setShortcut (key);
    }
    connect (ga, SIGNAL (triggered(bool)), tempMapper, SLOT (map()));
    tempMapper->setMapping (ga, code);
    return ga;
}

KAction * KGoldrunner::editAction (const QString & name,
                                   const int       code,
                                   const QString & text,
                                   const QString & toolTip,
                                   const QString & whatsThis)
{
    KAction * ed = actionCollection()->addAction (name);
    ed->setText (text);
    ed->setToolTip (toolTip);
    ed->setWhatsThis (whatsThis);
    connect (ed, SIGNAL (triggered(bool)), tempMapper, SLOT (map()));
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
    connect (s, SIGNAL (triggered(bool)), tempMapper, SLOT (map()));
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
    connect (ed, SIGNAL (triggered(bool)), tempMapper, SLOT (map()));
    tempMapper->setMapping (ed, mapCode);
    return ed;
}

void KGoldrunner::keyControl (const QString & name, const QString & text,
                              const QKeySequence & shortcut, const int code,
                              const bool mover)
{
    KAction * a = actionCollection()->addAction (name);
    a->setText (text);
    a->setShortcut (shortcut);

    // If this is a move-key, let keyPressEvent() through, instead of signal.
    if (mover) {
        a->setEnabled (false);
    }
    else {
        a->setAutoRepeat (false);	// Else, prevent QAction signal repeat.
    }

    connect (a, SIGNAL (triggered(bool)), tempMapper, SLOT (map()));
    tempMapper->setMapping (a, code);
    addAction (a);
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
    bool result = false;
    if (! event->isAutoRepeat()) {
        QKeySequence keystroke (event->key() + event->modifiers());
        result = true;

        if ((ACTION ("move_left"))->shortcuts().contains(keystroke)) {
            game->kbControl (LEFT, pressed);
        }
        else if ((ACTION ("move_right"))->shortcuts().contains(keystroke)) {
            game->kbControl (RIGHT, pressed);
        }
        else if ((ACTION ("move_up"))->shortcuts().contains(keystroke)) {
            game->kbControl (UP, pressed);
        }
        else if ((ACTION ("move_down"))->shortcuts().contains(keystroke)) {
            game->kbControl (DOWN, pressed);
        }
        else {
            result = false;
        }
    }
    return result;
}

void KGoldrunner::viewFullScreen (bool activation)
{
    KToggleFullScreenAction::setFullScreen (this, activation);
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

    gameFreeze (false);

    statusBar()->setItemFixed (ID_LIVES, -1);		// Fix current sizes.
    statusBar()->setItemFixed (ID_SCORE, -1);
    statusBar()->setItemFixed (ID_LEVEL, -1);
    statusBar()->setItemFixed (ID_HINTAVL, -1);

    connect (game, SIGNAL (showLives(long)),	SLOT (showLives(long)));
    connect (game, SIGNAL (showScore(long)),	SLOT (showScore(long)));
    connect (game, SIGNAL (showLevel(int)),	SLOT (showLevel(int)));
    connect (game, SIGNAL (gameFreeze(bool)),	SLOT (gameFreeze(bool)));
}

void KGoldrunner::showLives (long newLives)
{
    QString tmp;
    tmp.setNum (newLives);
    if (newLives < 100)
        tmp = tmp.rightJustified (3, '0');
    tmp.insert (0, i18n ("   Lives: "));
    tmp.append ("   ");
    // view->updateLives(newLives);
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
    // view->updateScore(newScore);
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
    myPause->setChecked (on_off);
    frozen = on_off;	// Remember the state (for the configure-keys case).
    QStringList pauseKeys;
    foreach (const QKeySequence &s, myPause->shortcut().toList()) {
        pauseKeys.append(s.toString(QKeySequence::NativeText));
    }
    if (on_off) {
        if (pauseKeys.size() == 0) {
            statusBar()->changeItem ("The game is paused", ID_MSG);
        } else if (pauseKeys.size() == 1) {
            statusBar()->changeItem (i18n ("Press \"%1\" to RESUME", pauseKeys.at(0)), ID_MSG);
        } else {
            statusBar()->changeItem (i18n ("Press \"%1\" or \"%2\" to RESUME", pauseKeys.at(0), pauseKeys.at(1)), ID_MSG);
        }
    } else {
        if (pauseKeys.size() == 0) {
            statusBar()->changeItem ("", ID_MSG);
        } else if (pauseKeys.size() == 1) {
            statusBar()->changeItem (i18n ("Press \"%1\" to PAUSE", pauseKeys.at(0)), ID_MSG);
        } else {
            statusBar()->changeItem (i18n ("Press \"%1\" or \"%2\" to PAUSE", pauseKeys.at(0), pauseKeys.at(1)), ID_MSG);
        }
    }
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

/*  void KGoldrunner::setEditMenu (bool on_off)
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
        kDebug() << "ToolBar icon size:" << scene->tileSize ();
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
} */

/*  void KGoldrunner::setEditIcon (const QString & actionName, const char iconType)
{
    ((KToggleAction *) (actionCollection()->action (actionName)))->
                setIcon (KIcon (renderer->getPixmap (iconType)));
} */

/******************************************************************************/
/*******************   SLOTS FOR MENU AND KEYBOARD ACTIONS  *******************/
/******************************************************************************/

void KGoldrunner::changeTheme ()
{
    renderer->selectTheme ();

    /*  if (game->inEditMode()) {
        setEditMenu (true);
    } */
}

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

void KGoldrunner::optionsConfigureKeys()
{
    // First run the standard KDE dialog for shortcut key settings.
    KShortcutsDialog::configure (actionCollection(),
	KShortcutsEditor::LetterShortcutsAllowed,	// Single letters OK.
	this,						// Parent widget.
	true);						// saveSettings value.

    gameFreeze (frozen);		// Refresh the status bar text.
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
                myDir, dirs->resourceDirs ("html").join ( QLatin1String( ":" ))));
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
         myDir, dirs->resourceDirs ("data").join ( QLatin1String( ":" ))));
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
         myDir, dirs->resourceDirs ("data").join ( QLatin1String( ":" ))));
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

/* void KGoldrunner::setupEditToolbarActions()
{
    QSignalMapper * editToolbarMapper = new QSignalMapper (this);
    connect (editToolbarMapper, SIGNAL (mapped(int)),
             game, SLOT (editToolbarActions(int)));
    tempMapper = editToolbarMapper;

    KAction * ed = editAction ("edit_hint", EDIT_HINT,
                               i18n ("Edit Name/Hint"),
                               i18n ("Edit level name or hint"),
                               i18n ("Edit text for the name or hint "
                                     "of a level"));
    ed->setIcon (KIcon ( QLatin1String( "games-hint" )));
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
} */

QSize KGoldrunner::sizeHint() const
{
    kDebug() << "KGoldrunner::sizeHint() called ... 640x600";
    return QSize (640, 600);
}

#include "kgoldrunner.moc"
