/*
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRLEVELPLAYER_H
#define KGRLEVELPLAYER_H

#include "kgrglobals.h"

#include <QList>
#include <QObject>
#include <QVarLengthArray>

#include <QElapsedTimer> // IDW testing

class KGrTimer;
class KGrLevelGrid;
class KGrRuleBook;
class KGrView;
class KGrHero;
class KGrGame;
class KGrEnemy;

class QRandomGenerator;

/**
 * @short Class to play, record and play back a level of a game
 *
 * This class constructs and plays a single level of a KGoldrunner game.  A
 * KGrLevelPlayer object is created as each level begins and is destroyed as
 * the level finishes, whether the human player wins the level or loses it.
 * Each level is either recorded as it is played or played back from an earlier
 * recording by emulating the same inputs as were used previously.
 *
 * The KGrLevelPlayer object in turn creates all the objects needed to play
 * the level, such as a hero, enemies, a play-area grid and a rule-book, which
 * are all derived from data in a KGrLevelData structure.  After the level and
 * all its objects are set up, most of the work is done by the private slot
 * tick(), including signals to update the graphics animation.  Tick() is
 * activated periodically by time-signals from the KGrTimer object, which also
 * handles the standard Pause action and variations in the overall speed of
 * KGoldrunner, from Beginner to Champion.  KGrLevelPlayer is controlled by
 * inputs from the mouse, keyboard or touchpad, which in turn control the
 * movement of the hero and the digging of bricks.  The enemies are controlled
 * by algorithms in the polymorphic KGrRuleBook object.  Each set of rules has
 * its own distinct algorithm.  In playback mode, the inputs are emulated.
 *
 * KGrLevelPlayer and friends are the internal model and game-engine of
 * KGoldrunner and they communicate with the view, KGrCanvas and friends,
 * solely via signals that indicate what is moving and what has to be painted.
 */

class KGrLevelPlayer : public QObject
{
    Q_OBJECT
public:
    /**
     * The constructor of KGrLevelPlayer.
     *
     * @param parent     The object that owns the level-player and will destroy
     *                   it if the KGoldrunner application is terminated during
     *                   play.
     * @param pRandomGen A shared source of random numbers for all enemies.
     */
    KGrLevelPlayer             (KGrGame * parent, QRandomGenerator * pRandomGen);
    ~KGrLevelPlayer();

    /**
     * The main initialisation of KGrLevelPlayer.  This method establishes the
     * playing rules to be used, creates the internal playing-grid, creates the
     * hero and enemies and connects the various signals and slots together.  It
     * also initialises the recording or playback of moves made by the hero and
     * enemies.  Note that this is the only place where KGrLevelPlayer uses the
     * view directly.  All other references are via signals and slots.
     *
     *
     * @param view       Points to the KGrCanvas object that provides graphics.
     * @param pRecording Points to a data-object that contains all the data for
     *                   the level, including the layout of the maze and the
     *                   starting positions of hero, enemies and gold, plus the
     *                   variant of the rules to be followed: Traditional,
     *                   KGoldrunner or Scavenger.  The level is either recorded
     *                   as it is played or re-played from an earlier recording.
     *                   If playing "live", the pRecording object has empty
     *                   buffers into which the level player will store moves.
     *                   If re-playing, the buffers contain hero's moves to be
     *                   played back and random numbers for the enemies to
     *                   re-use, so that all play can be faithfully reproduced,
     *                   even if the random-number generator's code changes.
     * @param pPlayback  If false, play "live" and record the play.  If true,
     *                   play back a previously recorded level.
     * @param gameFrozen If true, go into pause-mode when the level starts.
     */
    void init                   (KGrView *            view,
                                 KGrRecording *       pRecording,
                                 const bool           pPlayback,
                                 const bool           gameFrozen);

    /**
     * Indicate that setup is complete and the human player can start playing
     * at any time, by moving the pointer device or pressing a key.
     */
    void prepareToPlay          ();

    /**
     * Pause or resume the gameplay in this level.
     *
     * @param stop      If true, pause: if false, resume.
     */
    void pause                  (bool stop);

    /**
     * Stop playback of a recorded level and adjust the content of the recording
     * so that the user can continue playing and recording from that point, if
     * required, as in the Instant Replay or Replay Last Level actions.
     */
    void interruptPlayback();

    /**
     * If not in playback mode, add a code to the recording and kill the hero.
     */
    void killHero();

    /**
     * Change the input-mode during play.
     *
     * @param mode      The new input-mode to use to control the hero: mouse, 
     *                  keyboard or hybrid touchpad and keyboard mode.
     */
    void setControlMode  (const int mode);

    /**
     * Change the keyboard click/hold option during play.
     *
     * @param option    The new option for keyboard operation: either CLICK_KEY
     *                  to start running non-stop when a direction key is
     *                  clicked or HOLD_KEY to start when a key is pressed and
     *                  stop when it is released, with simultaneous key-holding
     *                  allowed.
     */
    void setHoldKeyOption  (const int option);

    /**
     * Set the overall speed of gameplay.
     *
     * @param timeScale Value 10 is for normal speed.  Range is 2 to 20.
     *                  5 is for beginner speed: 15 for champion speed.
     */
    void setTimeScale    (const int timeScale);

    /**
     * Set a point for the hero to aim at when using mouse or touchpad control.
     *
     * @param pointerI  The required column-number on the playing-grid (>=1).
     * @param pointerJ  The required row-number on the playing-grid (>=1).
     */
    void setTarget              (int pointerI, int pointerJ);

    /**
     * Set a direction for the hero to move or dig when using keyboard control.
     *
     * @param dirn      The required direction (values defined by enum Direction
     *                  in file kgrglobals.h).
     * @param pressed   Tells whether the direction-key was pressed or released.
     */
    void setDirectionByKey      (const Direction dirn, const bool pressed);

    /**
     * Helper function for the hero to find his next direction when using mouse
     * or touchpad control.  Uses the point from setTarget() as a guide.
     *
     * @param heroI     The column-number where the hero is now (>=1).
     * @param heroJ     The row-number where the hero is now (>=1).
     *
     * @return          The required direction (values defined by enum Direction
     *                  in file kgrglobals.h).
     */
    Direction getDirection      (int heroI, int heroJ);

    /**
     * Helper function for an enemy to find his next direction, based on where
     * the hero is and the search algorithm implemented in the level's rules.
     *
     * @param enemyI    The column-number where the enemy is now (>=1).
     * @param enemyJ    The row-number where the enemy is now (>=1).
     * @param leftRightSearch The search-direction (for KGoldrunner rules only).
     *
     * @return          The required direction (values defined by enum Direction
     *                  in file kgrglobals.h).
     */
    Direction getEnemyDirection (int enemyI, int enemyJ, bool leftRightSearch);

    /**
     * Helper function for an enemy to pick up or drop gold or the hero to
     * collect gold.  Records the presence or absence of the gold on the
     * internal grid and on the screen.  Also pops up the hidden ladders (if
     * any) when there is no gold left.
     *
     * @param spriteId  The identifier of the hero or enemy.
     * @param i         The column-number where the gold is (>=1).
     * @param j         The row-number where the gold is (>=1).
     * @param hasGold   True if gold was picked up: false if it was dropped.
     * @param lost      True if gold is lost.
     *
     * @return          The number of pieces of gold remaining in this level.
     */
    int  runnerGotGold          (const int  spriteId, const int i, const int j,
                                 const bool hasGold, const bool lost = false);

    /**
     * Helper function to determine whether the hero has collided with an enemy
     * and must lose a life (unless he is standing on the enemy's head).
     *
     * @param heroX     The X grid-position of the hero (within a cell).
     * @param heroY     The Y grid-position of the hero (within a cell).
     *
     * @return          True if the hero is touching an enemy.
     */
    bool heroCaught             (const int heroX, const int heroY);

    /**
     * Helper function to determine whether the hero or an enemy is standing on
     * an enemy's head.
     *
     * @param spriteId  The identifier of the hero or enemy.
     * @param x         The X grid-position of the sprite (within a cell).
     * @param y         The Y grid-position of the sprite (within a cell).
     *
     * @return          Pointer to the enemy the sprite is standing on - or 0.
     */
    KGrEnemy * standOnEnemy     (const int spriteId, const int x, const int y);

    /**
     * Helper function to determine whether an enemy is colliding with another
     * enemy.  This to prevent enemies occupying the same cell, depending on
     * what the rules for this level allow.
     *
     * @param spriteId  The identifier of the enemy.
     * @param dirn      The direction in which the enemy wants to go.
     * @param gridI     The column-position of the enemy.
     * @param gridJ     The row-position of the enemy.
     *
     * @return          True if the enemy is too close to another enemy.
     */
    bool bumpingFriend          (const int spriteId, const Direction dirn,
                                 const int gridI,    const int gridJ);

    /**
     * Helper function to remove an enemy from among several stacked in a cell.
     *
     * @param spriteId  The identifier of the enemy.
     * @param gridI     The column-position of the enemy.
     * @param gridJ     The row-position of the enemy.
     * @param prevEnemy The previously stacked enemy in the same cell (or -1).
     */
    void unstackEnemy           (const int spriteId,
                                 const int gridI, const int gridJ,
                                 const int prevEnemy);
    /**
     * Helper function to determine where an enemy should reappear after being
     * trapped in a brick.  This applies with Traditional and Scavenger rules
     * only.  The procedure chooses a random place in row 2 or row 1.
     *
     * @param gridI     A randomly-chosen column (return by reference).
     * @param gridJ     Row 2 Traditional or 1 Scavenger (return by reference).
     */
    void enemyReappear          (int & gridI, int & gridJ);

    /**
     * Helper function to provide enemies with random numbers for reappearing
     * and deciding whether to pick up or drop gold.  The random bytes
     * generated are stored during recording and re-used during playback, so
     * that play is completely reproducible, even if the library's random number
     * generator behavior should vary across platforms or in future versions.
     *
     * @param limit     The upper limit for the number to be returned.
     *
     * @return          The random number, value >= 0 and < limit.
     */
    uchar randomByte            (const uchar limit);

    /**
     * Implement author's debugging aids, which are activated only if the level
     * is paused and the KConfig file contains group Debugging with setting
     * DebuggingShortcuts=true.  The main actions are to do timer steps one at
     * a time, activate/deactivate a bug-fix or new-feature patch dynamically,
     * activate/deactivate logging output from fprintf or qCDebug(KGOLDRUNNER_LOG) dynamically,
     * print the status of a cell pointed to by the mouse and print the status
     * of the hero or an enemy.  See the code in file kgoldrunner.cpp, at the
     * end of KGoldrunner::setupActions() for details of codes and keystrokes.
     *
     * To use the BUG_FIX or LOGGING options, first patch in and compile some
     * code to achieve the effect required, with tests of static bool flags
     * KGrGame::bugFix or KGrGame::logging surrounding that code.  The relevant
     * keystrokes then toggle those flags, so as to execute or skip the code
     * dynamically as the game runs.
     *
     * @param code      A code to indicate the action required (see enum
     *                  DebugCodes in file kgrglobals.h).
     */
    void dbgControl             (int code);	// Authors' debugging aids.

Q_SIGNALS:
    void endLevel       (const int result);
    void getMousePos    (int & i, int & j);
    void setMousePos    (const int i, const int j);

    /**
     * Requests the view to update animated sprites. Each sprite moves as
     * decided by the parameters of its last startAnimation() signal.
     *
     * @param missed       If true, moves and frame changes occur normally, but
     *                     they are not displayed on the screen. This is to
     *                     allow the graphics to catch up if Qt has missed one
     *                     or more time signals.
     */
    void animation      (bool missed);

    /**
     * Requests the view to display a particular type of tile at a particular
     * cell, or make it empty and show the background (tileType = FREE). Used
     * when loading level-layouts.
     *
     * @param i            The column-number of the cell to paint.
     * @param j            The row-number of the cell to paint.
     * @param tileType     The type of tile to paint (gold, brick, ladder, etc).
     */
    void paintCell      (int i, int j, char tileType);

    int  makeSprite     (char spriteType, int i, int j);

    /**
     * Requests the view to display an animation of a dug brick at a
     * particular cell, cancelling and superseding any current animation.
     *
     * @param spriteId     The ID of the sprite (dug brick).
     * @param repeating    If true, repeat the animation (false for dug brick).
     * @param i            The column-number of the cell to dig.
     * @param j            The row-number of the cell to dig.
     * @param time         The time in which to traverse one cell.
     * @param dirn         The direction of motion, always STAND.
     * @param type         The type of animation (open or close the brick).
     */
    void startAnimation (const int spriteId, const bool repeating,
                         const int i, const int j, const int time,
                         const Direction dirn, const AnimationType type);

    void deleteSprite   (const int spriteId);
    void gotGold        (const int  spriteId, const int i, const int j,
                         const bool hasGold, const bool lost);
    void interruptDemo  ();

private Q_SLOTS:
    /**
     * This slot powers the whole game. KGrLevelPlayer connects it to KGrTimer's
     * tick() signal. In this slot, KGrLevelPlayer EITHER plays back a recorded
     * tick OR checks the mouse/trackpad/keyboard for user-input, then processes
     * dug bricks, moves the hero, moves the enemies and finally emits the
     * animation() signal, which causes the view to update the screen.
     *
     * @param missed       If true, the QTimer has missed one or more ticks, due
     *                     to overheads elsewhere in Qt or the O/S. The game
     *                     catches up on the missed signal(s) and the graphics
     *                     view avoids painting any sprites until the catchup
     *                     is complete, thus saving further overheads. The
     *                     sprites may "jump" a little when this happens, but
     *                     at least the game stays on-time in wall-clock time.
     * @param pScaledTime  The number of milliseconds per tick. Usually this is
     *                     tickTime (= 20 msec), but it is less when the game is
     *                     slowed down or more when it is speeded up. If the
     *                     scaled time is 10 (beginner speed), the game will
     *                     take 2 ticks of 20 msec (i.e. 40 msec) to do what it
     *                     normally does in 20 msec.
     */
    void tick           (bool missed, int scaledTime);

    void doDig          (int button);	// Dig using mouse-buttons.

private:
    KGrGame *            game;
    QRandomGenerator *   randomGen;
    KGrLevelGrid *       grid;
    KGrRuleBook *        rules;
    KGrHero *            hero;
    int                  heroId;
    QList<KGrEnemy *>    enemies;

    int                  controlMode;
    int                  holdKeyOption;
    int                  levelWidth;
    int                  levelHeight;

    int                  nuggets;

    enum                 PlayState {NotReady, Ready, Playing};
    PlayState            playState;

    KGrRecording *       recording;
    bool                 playback;
    int                  recIndex;
    int                  recCount;
    int                  randIndex;

    int                  targetI;	// Where the mouse is pointing.
    int                  targetJ;

    Direction setDirectionByDelta (const int di, const int dj,
                                   const int heroI, const int heroJ);
    Direction            direction;	// Direction for the hero to take.
    Direction            newDirection;	// Next direction for the hero to take.
    KGrTimer *           timer;		// The time-standard for the level.

    void startDigging (Direction diggingDirection);
    void processDugBricks (const int scaledTime);

    int  digCycleTime;			// Milliseconds per dig-timing cycle.
    int  digCycleCount;			// Number of cycles hole is fully open.
    int  digOpeningCycles;		// Cycles for brick-opening animation.
    int  digClosingCycles;		// Cycles for brick-closing animation.
    int  digKillingTime;		// Cycle when enemy/hero gets killed.

    int                  dX;		// X motion for KEYBOARD + HOLD_KEY.
    int                  dY;		// Y motion for KEYBOARD + HOLD_KEY.

    typedef struct {
        int  id;
        int  cycleTimeLeft;
        int  digI;
        int  digJ;
        int  countdown;
        qint64  startTime; // IDW testing
    } DugBrick;

    QList <DugBrick *> dugBricks;

    int          reappearIndex;
    QVector<int> reappearPos;
    void         makeReappearanceSequence();
    bool         doRecordedMove();
    void         recordInitialWaitTime (const int ms);
    void         record (const int bytes, const int n1, const int n2 = 0);

/******************************************************************************/
/**************************  AUTHORS' DEBUGGING AIDS **************************/
/******************************************************************************/

    static int playerCount;

    void bugFix();		// Turn a bug fix on/off dynamically.
    void startLogging();	// Turn logging on/off.
    void showFigurePositions();	// Show everybody's co-ordinates.
    void showObjectState();	// Show an object's state.
    void showEnemyState (int);	// Show enemy's co-ordinates and state.

    /// TODO - Remove these ...
    QElapsedTimer t; // IDW testing
    int   T; // IDW testing
};

#endif // KGRLEVELPLAYER_H
