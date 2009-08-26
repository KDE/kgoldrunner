/****************************************************************************
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

#ifndef KGRLEVELPLAYER_H
#define KGRLEVELPLAYER_H

#include "kgrglobals.h"

#include <QObject>
#include <QList>
#include <QVarLengthArray>

#include <QTime> // IDW testing

class KGrTimer;
class KGrLevelGrid;
class KGrRuleBook;
class KGrCanvas;
class KGrHero;
class KGrEnemy;

class KRandomSequence;

/**
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
 *
 * @short Class to play, record and play back a level of a game
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
    KGrLevelPlayer             (QObject * parent, KRandomSequence * pRandomGen);
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
     * @param mode       The input-mode used to control the hero: mouse,
     *                   keyboard or hybrid touchpad and keyboard mode (for
     *                   laptops).
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
    void init                   (KGrCanvas *          view,
                                 const int            mode,
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
     */
    void setDirectionByKey      (Direction dirn);

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
     * activate/deactivate logging output from fprintf or kDebug() dynamically,
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

signals:
    void endLevel       (const int result);
    void getMousePos    (int & i, int & j);
    void setMousePos    (const int i, const int j);
    void animation      (bool missed);
    void paintCell      (int i, int j, char tileType, int diggingStage = 0);
    int  makeSprite     (char spriteType, int i, int j);
    void startAnimation (const int spriteId, const bool repeating,
                         const int i, const int j, const int time,
                         const Direction dirn, const AnimationType type);
    void deleteSprite   (const int spriteId);
    void gotGold        (const int  spriteId, const int i, const int j,
                         const bool hasGold, const bool lost);
    void interruptDemo  ();

private slots:
    void tick           (bool missed, int scaledTime);
    void doDig          (int button);	// Dig using mouse-buttons.

private:
    QObject *            game;
    KRandomSequence *    randomGen;
    KGrLevelGrid *       grid;
    KGrRuleBook *        rules;
    KGrHero *            hero;
    int                  heroId;
    QList<KGrEnemy *>    enemies;

    int                  controlMode;
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

    typedef struct {
        int  id;
        int  cycleTimeLeft;
        int  digI;
        int  digJ;
        int  countdown;
        int  startTime; // IDW testing
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
    QTime t; // IDW testing
    int   T; // IDW testing
};

#endif // KGRLEVELPLAYER_H
