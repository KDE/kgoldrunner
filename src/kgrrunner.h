/****************************************************************************
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                         *
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

#ifndef KGRRUNNER_H
#define KGRRUNNER_H

#include "kgrglobals.h"

#include <QObject>
#include <QTime> // IDW

class KGrLevelPlayer;
class KGrLevelGrid;
class KGrRuleBook;

enum  Situation {NotTimeYet, CaughtInBrick, MidCell, EndCell};

/**
 * This class provides the shared features of all runners (hero and enemies).
 */
class KGrRunner : public QObject
{
    Q_OBJECT
public:
    /**
     * The constructor of the KGrRunner virtual class.
     *
     * @param pLevelPlayer The object that owns the runner and will destroy it
     *                     if the KGoldrunner application is terminated during
     *                     play.  The object also provides helper functions to
     *                     the runners.
     * @param pGrid        The grid on which the runner is playing.
     * @param i            The starting column-number (>=1).
     * @param j            The starting row-number (>=1).
     * @param pSpriteId    The sprite ID of the runner, as used in animation.
     * @param pRules       The rules that apply to this game and level.
     * @param startDelay   The starting-time advantage enemies give to the hero.
     */
    KGrRunner (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
               int i, int j, const int pSpriteId,
               KGrRuleBook  * pRules, const int startDelay);
    virtual ~KGrRunner();

    /**
     * Returns the exact position of a runner (in grid-points or cell
     * sub-divisions) and the number of grid-points per cell, from which the
     * cell's column-number and row-number can be calculated if required.
     *
     * @param x            X-coordinate in grid-points (return by reference).
     * @param y            Y-coordinate in grid-points (return by reference).
     *
     * @return             The number of grid-points per cell.
     */
    inline int whereAreYou (int & x, int & y) {
                            x = gridX; y = gridY; return pointsPerCell; }

signals:
    /**
     * Requests the KGoldrunner game to add to the human player's score.
     *
     * @param n            The amount to add to the score.
     */
    void incScore          (const int n);

    /**
     * Requests the view-object to display an animation of a runner at a
     * particular cell, cancelling and superseding any current animation.
     *
     * @param spriteId     The ID of the sprite (hero or enemy).
     * @param repeating    If true, repeat the animation until the next signal.
     * @param i            The column-number of the cell to start at.
     * @param j            The row-number of the cell to start at.
     * @param time         The time in which to traverse one cell.
     * @param dirn         The direction of motion, or STAND.
     * @param type         The type of animation (walk, climb. etc.).
     */
    void startAnimation    (const int spriteId, const bool repeating,
                            const int i, const int j, const int time,
                            const Direction dirn, const AnimationType type);

protected:
    KGrLevelPlayer * levelPlayer;
    KGrLevelGrid *   grid;
    KGrRuleBook *    rules;

    int              spriteId;

    int              gridI;
    int              gridJ;
    int              gridX;
    int              gridY;
    int              deltaX;
    int              deltaY;

    int              pointCtr;
    int              pointsPerCell;
    bool             turnAnywhere;

    void             getRules();

    Situation        situation (const int scaledTime);
    char             nextCell();
    bool             setNextMovement (const char spriteType,
                                      const char cellType,
                                      Direction & dir,
                                      AnimationType & anim, int & interval);

    bool             falling;
    Direction        currDirection;
    AnimationType    currAnimation;

    int              runTime;		// Time interval for hero/enemy running.
    int              fallTime;		// Time interval for hero/enemy falling.
    int              enemyFallTime;	// Time interval for enemy falling.
    int              trapTime;		// Time interval for which an enemy can
					// stay trapped in a brick.

    int              interval;		// The runner's current time interval.
    int              timeLeft;		// Time till the runner's next action.

    bool             leftRightSearch;	// KGoldrunner-rules enemy search-mode.

    QTime            t; // IDW
};


/**
 * This class models the behaviour of the hero.  It inherits from KGrRunner.
 *
 * The hero's main functions are running, digging holes in bricks and collecting
 * gold.  If he is caught by an enemy or trapped in a closing brick, he dies.
 * If he collects all the gold and runs to the top row, he wins the level.
 */
class KGrHero : public KGrRunner
{
    Q_OBJECT
public:
    /**
     * The constructor of the KGrHero class.  The parameters are the same as
     * for the KGrRunner constructor, which does most of the work, but this
     * constructor also initialises the hero's timing, which depends on the
     * rules being used.
     *
     * @param pLevelPlayer The object that owns the hero and will destroy him
     *                     if the KGoldrunner application is terminated during
     *                     play.  The object also provides helper functions to
     *                     the hero.
     * @param pGrid        The grid on which the hero is playing.
     * @param i            The starting column-number (>=1).
     * @param j            The starting row-number (>=1).
     * @param pSpriteId    The sprite ID of the hero, as used in animation.
     * @param pRules       The rules that apply to this game and level.
     */
    KGrHero (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                int i, int j, int pSpriteId, KGrRuleBook  * pRules);
    ~KGrHero();

    /**
     * Makes the hero run, under control of a pointer or the keyboard and
     * guided by the layout of the grid.  The method is invoked by a periodic
     * timer and returns NORMAL status while play continues.  If the hero is
     * caught by an enemy or trapped in a brick, it returns DEAD status, or
     * if he collects all the gold and reaches the top row, the method returns
     * WON_LEVEL status.  Otherwise it changes the hero's position as required
     * and decides the type of animation to display (run left, climb, etc.).
     *
     * @param scaledTime   The amount by which to adjust the time, scaled
     *                     according to the current game-speed setting.  Smaller
     *                     times cause slower running in real-time: larger times
     *                     cause faster running.
     *
     * @return             The hero's status: NORMAL, DEAD or WON_LEVEL.
     */
    HeroStatus       run (const int scaledTime);

    /**
     * Decides whether the hero can dig as is required by pressing a key or a
     * mouse-button.  If OK, the KGrLevelPlayer will control the dug brick.
     *
     * @param dirn         The direction in which to dig: L or R of the hero.
     * @param digI         The column-number of the brick (return by reference).
     * @param digJ         The row-number of the brick (return by reference).
     *
     * @return             If true, a hole can be dug in the direction required.
     */
    bool             dig (const Direction dirn, int & digI, int & digJ);

    /**
     * Tells the hero how many gold nuggets are remaining.
     *
     * @param nGold        The number of gold nuggets remaining.
     */
    inline void      setNuggets (const int nGold) { nuggets = nGold; }

    /**
     * Implements the author's debugging aid that shows the hero's state.
     */
    void             showState();

signals:
    void             soundSignal (const int n, const bool onOff = true);
private:
    int              nuggets;		// Number of gold pieces remaining.
};


/**
 * This class models the behaviour of an enemy.  It inherits from KGrRunner.
 *
 * An enemy's main functions are running, chasing the hero and collecting or
 * dropping gold.  If he comes to a dug brick, he must fall in and give up any
 * gold he is carrying.  If he is trapped in a closing brick, he dies but
 * reappears elsewhere on the grid.
 */
class KGrEnemy :     public KGrRunner
{
    Q_OBJECT
public:
    /**
     * The constructor of the KGrEnemy class.
     *
     * @param pLevelPlayer The object that owns the enemy and will destroy him
     *                     if the KGoldrunner application is terminated during
     *                     play.  The object also provides helper functions to
     *                     the enemies.
     * @param pGrid        The grid on which the enemy is playing.
     * @param i            The starting column-number (>=1).
     * @param j            The starting row-number (>=1).
     * @param pSpriteId    The sprite ID of the enemy, as used in animation.
     * @param pRules       The rules that apply to this game and level.
     * @param pRandomGen   Random number generator: used to decide when to pick
     *                     up or drop gold.
     */
    KGrEnemy (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                 int i, int j, int pSpriteId, KGrRuleBook  * pRules);
    ~KGrEnemy();

    /**
     * Makes an enemy run, guided by the position of the hero and the layout of
     * the grid.  The method is invoked by a periodic timer.  If the enemy is
     * trapped in a brick, he reappears somewhere else on the grid, depending
     * on the rules for the game and level.  While running, the enemy picks up
     * and randomly drops gold.  If he comes to a dug brick, he must fall in
     * and give up any gold he is carrying.  He can climb out after a time: or
     * the hole might close first.  Otherwise the method changes the enemy's
     * position as required, avoids collisions and decides the type of animation
     * to display (run left, climb, etc.).
     *
     * @param scaledTime   The amount by which to adjust the time, scaled
     *                     according to the current game-speed setting.  Smaller
     *                     times cause slower running in real-time: larger times
     *                     cause faster running.
     */
    void             run (const int scaledTime);

    /**
     * Returns the direction in which the enemy is running: used for avoiding
     * collisions with other enemies.
     */
    inline Direction direction() { return (currDirection); }

    /**
     * Returns the ID of an enemy who already occupied the same cell (or -1).
     */
    inline int       getPrevInCell()  { return (prevInCell); }

    /**
     * Sets the ID of an enemy who already occupied the same cell (or -1).
     *
     * @param prevEnemy    The sprite ID of the previous enemy (or -1).
     */
    inline void      setPrevInCell (const int prevEnemy) {
                                    prevInCell = prevEnemy; }

    /**
     * Returns true if the enemy is falling.
     */
    inline bool      isFalling() { return falling; }

    /**
     * Implements the author's debugging aid that shows the enemy's state.
     */
    void             showState();

private:
    char             rulesType;		// Rules type and enemy search method.

    int              nuggets;		// Number of gold pieces an enemy holds.
    int              birthI;		// Enemy's starting position (used in
    int              birthJ;		// KGoldrunner rules for re-birth).
    int              prevInCell;	// ID of previous enemy in cell or -1.

    void             dropGold();
    void             checkForGold();
    void             dieAndReappear();

    void             reserveCell (const int i, const int j);
    void             releaseCell (const int i, const int j);
};

#endif // KGRRUNNER_H
