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

#include <stdio.h>
#include <stdlib.h>
#include <KDebug>

#include <QTimer>
#include <QList>

#include "kgrcanvas.h"
#include "kgrlevelplayer.h"
#include "kgrrulebook.h"
#include "kgrlevelgrid.h"
#include "kgrrunner.h"
#include "kgrtimer.h"

KGrLevelPlayer::KGrLevelPlayer (QObject * parent)
    :
    QObject          (parent),
    hero             (0),
    controlMode      (MOUSE),
    nuggets          (0),
    playState        (NotReady),
    targetI          (1),
    targetJ          (1),
    direction        (STAND),
    timer            (0),
    digCycleTime     (200),	// Milliseconds per dig-timing cycle (default).
    digCycleCount    (40),	// Cycles while hole is fully open (default).
    digOpeningCycles (5),	// Cycles for brick-opening animation.
    digClosingCycles (4),	// Cycles for brick-closing animation.
    digKillingTime   (2)	// Cycle at which enemy/hero gets killed.
{
    t.start(); // IDW
    gameLogging = false;
    bugFixed = false;
}

KGrLevelPlayer::~KGrLevelPlayer()
{
    while (! dugBricks.isEmpty()) {
        delete dugBricks.takeFirst();
    }

    // TODO - Do we need this delete?
    // while (! enemies.isEmpty()) {
        // delete enemies.takeFirst();
    // }
}

void KGrLevelPlayer::init (KGrCanvas * view, const Control mode,
                           const char rulesCode, const KGrLevelData * levelData)
{
    // TODO - Should not really remember the view: needed for setMousePos.
    mView = view;

    grid            = new KGrLevelGrid (this, levelData);

    controlMode     = mode;
    levelWidth      = levelData->width;
    levelHeight     = levelData->height;
    reappearIndex   = levelWidth;
    reappearPos.fill (1, levelWidth);

    // Set the rules of this game.
    switch (rulesCode) {
    case TraditionalRules:
        rules = new KGrTraditionalRules (this);
        break;
    case KGoldrunnerRules:
        rules = new KGrKGoldrunnerRules (this);
        break;
    case ScavengerRules:
        rules = new KGrScavengerRules (this);
        break;
    }
    rules->printRules();

    view->setGoldEnemiesRule (rules->enemiesShowGold());
    grid->calculateAccess    (rules->runThruHole());

    // Connect to code that paints grid cells and start-positions of sprites.
    connect (this, SIGNAL (paintCell (int, int, char, int)),
             view, SLOT   (paintCell (int, int, char, int)));
    connect (this, SIGNAL (makeSprite (char, int, int)),
             view, SLOT   (makeSprite (char, int, int)));

    // Connect to the mouse-positioning code in the graphics.
    connect (this, SIGNAL (setMousePos (const int, const int)),
             view, SLOT   (setMousePos (const int, const int)));

    // Show the layout of this level in the view (KGrCanvas).
    int wall = ConcreteWall;
    int enemyCount = 0;
    for (int j = wall ; j < levelHeight + wall; j++) {
        for (int i = wall; i < levelWidth + wall; i++) {
            char type = grid->cellType (i, j);

            // Hide false bricks.
            if (type == FBRICK) {
                type = BRICK;
            }

            // Count the gold in this level.
            if (type == NUGGET) {
                nuggets++;
            }

            // If the hero is here, leave the tile empty.
            if (type == HERO) {
                emit paintCell (i, j, FREE, 0);
            }

            // If an enemy is here, count him and leave the tile empty.
            else if (type == ENEMY) {
                enemyCount++;
                emit paintCell (i, j, FREE, 0);
            }

            // Or, just paint this tile.
            else {
                emit paintCell (i, j, type, 0);
            }
        }
    }

    // Set the timing rules, maybe based on the number of enemies.
    rules->setTiming (enemyCount);
    rules->getDigTimes (digCycleTime, digCycleCount);

// TODO - Do hero in pass 1 and enemies in pass 2, to ensure the hero has id 0.
    // Create the hero (always sprite 0), with the proper timing.
    for (int j = wall ; j < levelHeight + wall; j++) {
        for (int i = wall; i < levelWidth + wall; i++) {
            char type = grid->cellType (i, j);
            if (type == HERO) {
                if (hero == 0) {
                    targetI = i;
                    targetJ = j;
                    heroId  = emit makeSprite (HERO, i, j);
                    hero    = new KGrHero (this, grid, i, j, heroId, rules);
                    // TODO - Iff mouse mode, setMousePos();
                    emit setMousePos (targetI, targetJ);
                    grid->changeCellAt (i, j, FREE);	// Hero now a sprite.
                }
            }
        }
    }

    // Create the enemies (sprites 1-n), with the proper timing.
    for (int j = wall ; j < levelHeight + wall; j++) {
        for (int i = wall; i < levelWidth + wall; i++) {
            char type = grid->cellType (i, j);
            if (type == ENEMY) {
                KGrEnemy * enemy;
                int id = emit makeSprite (ENEMY, i, j);
                enemy = new KGrEnemy (this, grid, i, j, id, rules);
                enemies.append (enemy);
                grid->changeCellAt (i, j, FREE);	// Enemy now a sprite.
            }
        }
    }

    // Connect the hero's and enemies' efforts to the graphics.
    connect (this, SIGNAL (gotGold (int, int, int, bool)),
             view, SLOT   (gotGold (int, int, int, bool)));

    // Connect mouse-clicks from KGrCanvas to digging slot.
    connect (view, SIGNAL (mouseClick (int)), SLOT (doDig (int)));

    // Connect the new hero and enemies (if any) to the animation code.
    connect (hero, SIGNAL (startAnimation (int, bool, int, int, int,
                                           Direction, AnimationType)),
             view, SLOT   (startAnimation (int, bool, int, int, int,
                                           Direction, AnimationType)));
    foreach (KGrEnemy * enemy, enemies) {
        connect (enemy, SIGNAL (startAnimation (int, bool, int, int, int,
                                                Direction, AnimationType)),
                 view,  SLOT   (startAnimation (int, bool, int, int, int,
                                                Direction, AnimationType)));
    }

    // Connect the level player to the animation code (for use with dug bricks).
    connect (this, SIGNAL (startAnimation (int, bool, int, int, int,
                                           Direction, AnimationType)),
             view, SLOT   (startAnimation (int, bool, int, int, int,
                                           Direction, AnimationType)));
    connect (this, SIGNAL (deleteSprite (int)),
             view, SLOT   (deleteSprite (int)));

    // Connect the grid to the view, to show hidden ladders when the time comes.
    connect (grid, SIGNAL (showHiddenLadders (const QList<int> &, const int)),
             view, SLOT   (showHiddenLadders (const QList<int> &, const int)));

    // Connect and start the timer.  The tick() slot emits signal animation(),
    // so there is just one time-source for the model and the view.

    timer = new KGrTimer (this, TickTime);	// TickTime def in kgrglobals.h.

    connect (timer, SIGNAL (tick (bool, int)), this, SLOT (tick (bool, int)));
    connect (this,  SIGNAL (animation (bool)), view, SLOT (animate (bool)));
    // timer->start (TickTime);	// Interval = TickTime, defined in kgrglobals.h.
}

void KGrLevelPlayer::startDigging (Direction diggingDirection)
{
    int digI = 1;
    int digJ = 1;

    // We need the hero to decide if he CAN dig and if so, where.
    if (hero->dig (diggingDirection, digI, digJ)) {
        // The hero can dig as requested: the chosen brick is at (digI, digJ).
        grid->changeCellAt (digI, digJ, HOLE);

        // Delete the brick-image so that animations are visible in all themes.
        // TODO - Remove. emit paintCell (digI, digJ, FREE, 0);

        // Start the brick-opening animation (non-repeating).
        int id = emit makeSprite (BRICK, digI, digJ);
        emit startAnimation (id, false, digI, digJ,
                        (digOpeningCycles * digCycleTime), STAND, OPEN_BRICK);

        DugBrick * thisBrick = new DugBrick;
        DugBrick   brick     = {id, digCycleTime, digI, digJ,
                    (digCycleCount + digOpeningCycles + digClosingCycles - 1),
                    t.elapsed()}; // IDW test
        (* thisBrick)        = brick;
        dugBricks.append (thisBrick);
        // kDebug() << "DIG" << thisBrick->id << thisBrick->countdown
                 // << "time" << (t.elapsed() - thisBrick->startTime);
    }
}

void KGrLevelPlayer::processDugBricks (const int scaledTime)
{
    DugBrick * dugBrick;
    QMutableListIterator<DugBrick *> iterator (dugBricks);

    while (iterator.hasNext()) {
        dugBrick = iterator.next();
        dugBrick->cycleTimeLeft -= scaledTime;
        if (dugBrick->cycleTimeLeft < scaledTime) {
            int id = dugBrick->id; // IDW testing
            dugBrick->cycleTimeLeft += digCycleTime;
            if (--dugBrick->countdown == digClosingCycles) {
                // Start the brick-closing animation (non-repeating).
                // kDebug() << "Brick" << dugBrick->digI << dugBrick->digJ <<
                            // "count" << dugBrick->countdown;
                emit startAnimation (dugBrick->id, false,
                                     dugBrick->digI, dugBrick->digJ,
                                     (digClosingCycles * digCycleTime),
                                     STAND, CLOSE_BRICK);
            }
            if (dugBrick->countdown == digKillingTime) {
                // kDebug() << "Brick" << dugBrick->digI << dugBrick->digJ <<
                            // "count" << dugBrick->countdown;
                // Close the hole and maybe capture the hero or an enemy.
                grid->changeCellAt (dugBrick->digI, dugBrick->digJ, BRICK);
            }
            if (dugBrick->countdown <= 0) {
                kDebug() << "DIG" << id << dugBrick->countdown
                         << "time" << (t.elapsed() - dugBrick->startTime);
                // Dispose of the dug brick and remove it from the list.
                emit deleteSprite (dugBrick->id);
                // TODO - Remove. emit paintCell (dugBrick->digI, dugBrick->digJ, BRICK);
                delete dugBrick;
                iterator.remove();
            }
            // TODO - Hero gets hidden by dug-brick in the Egyptian theme.
            // TODO - Why do we get so many MISSED ticks when we dig?
            // TODO - Hero falls through floor when caught in a closing brick.
            // TODO - Implement speed-variation as a parameter of tick().
        }
    }
}

void KGrLevelPlayer::prepareToPlay()
{
    // TODO - Should this be a signal?
    kDebug() << "Set mouse to:" << targetI << targetJ;
    emit setMousePos (targetI, targetJ);
    playState = Ready;
}

void KGrLevelPlayer::setTarget (int pointerI, int pointerJ)
{
    // Mouse or other pointer device (eg. laptop touchpad) controls the hero.
    switch (playState) {
    case NotReady:
        // Ignore the pointer until KGrLevelPlayer is ready to start.
        break;
    case Ready:
        // Wait until the human player is ready to start playing.
        if ((pointerI == targetI) && (pointerJ == targetJ)) {
            // The pointer is still over the hero: do not start playing yet.
            break;
        }
        else {
            // The pointer moved: fall into "case Playing:" and start playing.
            playState = Playing;
        }
    case Playing:
        // The human player is playing now.
        targetI = pointerI;
        targetJ = pointerJ;
        break;
    }
}

void KGrLevelPlayer::doDig (int button)
{
    // If not ready or game control is not by mouse, ignore mouse-clicks.
    if ((playState == NotReady) || (controlMode != MOUSE)) {
        return;
    }

    // TODO - Work this in with game-freezes.
    playState = Playing;
    switch (button) {
    case Qt::LeftButton:
        startDigging (DIG_LEFT);
        break;
    case Qt::RightButton:
        startDigging (DIG_RIGHT);
        break;
    default:
        break;
    }
}

void KGrLevelPlayer::setDirectionByKey (Direction dirn)
{
    // Keystrokes control the hero.
    if ((playState == NotReady) || (controlMode == MOUSE)) {
        return;
    }

    if ((dirn == DIG_LEFT) || (dirn == DIG_RIGHT)) {
	// Control mode is KEYBOARD or LAPTOP (hybrid: pointer + dig-keys).
        playState = Playing;
        direction = STAND;
        startDigging (dirn);
    }
    else if (controlMode == KEYBOARD) {
        playState = Playing;
        direction = dirn;
    }
}

Direction KGrLevelPlayer::getDirection (int heroI, int heroJ)
{
    if ((controlMode == MOUSE) || (controlMode == LAPTOP)) {
        // If using a pointer device, calculate the hero's next direction,
        // starting from last pointer position and hero's current position.

        int di = targetI - heroI;
        int dj = targetJ - heroJ;

        // kDebug() << "di" << di << "dj" << dj;

        if ((dj > 0) && (grid->heroMoves (heroI, heroJ) & dFlag [DOWN])) {
            // kDebug() << "Go down";
            direction = DOWN;
        }
        else if ((dj < 0) && (grid->heroMoves (heroI, heroJ) & dFlag [UP])) {
            // kDebug() << "Go up";
            direction = UP;
        }
        else if (di > 0) {
            // kDebug() << "Go right";
            direction = RIGHT;
        }
        else if (di < 0) {
            // kDebug() << "Go left";
            direction = LEFT;
        }
        else {		// Note: di is zero, but dj is not necessarily zero.
            // kDebug() << "Stand";
            direction = STAND;
        }
    }

    // kDebug() << "Hero at" << heroI << heroJ << "mouse at" << targetI << targetJ << "Direction" << direction;
    return direction;
}

Direction KGrLevelPlayer::getEnemyDirection (int enemyI, int enemyJ)
{
    int heroX, heroY, pointsPerCell;

    pointsPerCell = hero->whereAreYou (heroX, heroY);
    return rules->findBestWay (enemyI, enemyJ,
                               heroX / pointsPerCell, heroY / pointsPerCell,
                               grid);
}

bool KGrLevelPlayer::heroCaught (const int heroX, const int heroY)
{
    if (enemies.count() == 0) {
        return false;
    }
    int enemyX, enemyY, ppc1;
    foreach (KGrEnemy * enemy, enemies) {
        ppc1 = enemy->whereAreYou (enemyX, enemyY) - 1;
        if (((heroX < enemyX) ? ((heroX + ppc1) >= enemyX) :
                                 (heroX <= (enemyX + ppc1))) &&
            ((heroY < enemyY) ? ((heroY + ppc1) >= enemyY) :
                                 (heroY <= (enemyY + ppc1)))) {
            return true;
        }
    }
    return false;
}

bool KGrLevelPlayer::standOnEnemy (const int spriteId, const int x, const int y)
{
    int minEnemies = (spriteId == heroId) ? 1 : 2;
    if (enemies.count() < minEnemies) {
        return false;
    }
    int enemyX, enemyY, ppc;
    foreach (KGrEnemy * enemy, enemies) {
        ppc = enemy->whereAreYou (enemyX, enemyY);
        if (((enemyY == (y + ppc)) || (enemyY == (y + ppc - 1))) &&
            (enemyX >  (x - ppc)) && (enemyX <  (x + ppc))) {
            return true;
        }
    }
    return false;
}

bool KGrLevelPlayer::bumpingFriend (const int id, const int x, const int y)
{
    // TODO - Write this.
    return false;
}

void KGrLevelPlayer::tick (bool missed, int scaledTime)
{
    if (playState != Playing) {
        return;
    }

    if (dugBricks.count() > 0) {
        processDugBricks (scaledTime);
    }

    HeroStatus status = hero->run (scaledTime);
    if ((status == WON_LEVEL) || (status == DEAD)) {
        timer->pause();
        // TODO ? If caught in a brick, brick-closing animation is unfinished.
        // Queued connection ensures KGrGame slot runs AFTER return from here.
        emit endLevel (status);
        kDebug() << "END OF LEVEL";
        return;
    }

    foreach (KGrEnemy * enemy, enemies) {
        enemy->run (scaledTime);
    }

    emit animation (missed);
}

int KGrLevelPlayer::runnerGotGold (const int  spriteId,
                                    const int  i, const int j,
                                    const bool hasGold)
{
    if (hasGold) {
        kDebug() << "GOLD COLLECTED BY" << spriteId << "AT" << i << j;
    }
    else {
        kDebug() << "GOLD DROPPED BY" << spriteId << "AT" << i << j;
    }
    grid->gotGold (i, j, hasGold);		// Record pickup/drop on grid.
    emit  gotGold (spriteId, i, j, hasGold);	// Erase/show gold on screen.

    // If hero got gold, score, maybe show hidden ladders, maybe end the level.
    if (spriteId == heroId) {
        if (--nuggets <= 0) {
            grid->placeHiddenLadders();
        }
    }
    return nuggets;
}

void KGrLevelPlayer::pause (bool stop)
{
    if (stop) {
        timer->pause();
    }
    else {
        timer->resume();
    }
}

void KGrLevelPlayer::makeReappearanceSequence()
{
    // The idea is to make each possible x co-ord come up once per levelWidth
    // reappearances of enemies.  This is not truly random, but it reduces the
    // tedium in levels where you must keep killing enemies until a particular
    // x or range of x comes up (e.g. if they have to collect gold for you).

    // First put the positions in ascending sequence.
    for (int k = 0; k < levelWidth; k++) {
        reappearPos [k] = k + 1;
    }
    kDebug() << "Width" << levelWidth << "Non-randoms" << reappearPos;

    int z;
    int left = levelWidth;
    int temp;

    // Shuffle the x co-ords of reappearance positions, for x = 1 to levelWidth.
    for (int k = 0; k < levelWidth; k++) {
        // Pick a random element from those that are left.
        z = (int)((left * (float) rand()) / RAND_MAX);
        // Exchange its value with the last of the ones left.
        temp = reappearPos [z];
        reappearPos [z] = reappearPos [left - 1];
        reappearPos [left - 1] = temp;
        left--;
    }
    kDebug() << "Randoms" << reappearPos;
    reappearIndex = 0;
}

void KGrLevelPlayer::enemyReappear (int & gridI, int & gridJ)
{
    bool looking = true;
    int  i, j, k;

    // Follow Traditional or Scavenger rules: enemies reappear at top.
    j = rules->reappearRow();

    // Randomly look for a free spot in the row.  Limit the number of tries.
    for (k = 1; ((k <= 3) && looking); k++) {
        if (reappearIndex >= levelWidth) {
            makeReappearanceSequence();	// Get next array of random i.
        }
        i = reappearPos [reappearIndex++];
        switch (grid->cellType (i, j)) {
        case FREE:
        case HLADDER:
            looking = false;
            break;
        default:
            break;
        }
    }

    // If unsuccessful, choose the first free spot in the rows below.
    while ((j < levelHeight) && looking) {
        j++;
        i = 0;
        while ((i < levelWidth) && looking) {
            i++;
            switch (grid->cellType (i, j)) {
            case FREE:
            case HLADDER:
                looking = false;
                break;
            default:
                break;
            }
        }
    }
    kDebug() << "Reappear at" << i << j;
    gridI = i;
    gridJ = j;
}

/******************************************************************************/
/**************************  AUTHORS' DEBUGGING AIDS **************************/
/******************************************************************************/

void KGrLevelPlayer::dbgControl (int code)
{
    switch (code) {
    case DO_STEP:
        // tick (false);			// Do one timer step only.
        timer->step();
        break;
    case BUG_FIX:
        bugFix();			// Turn a bug fix on/off dynamically.
        break;
    case LOGGING:
        startLogging();			// Turn logging on/off.
        break;
    case S_POSNS:
        showFigurePositions();		// Show everybody's co-ordinates.
        break;
    case S_HERO:
        hero->showState ('s');		// Show hero's co-ordinates and state.
        break;
    case S_OBJ:
        showObjectState();		// Show an object's state.
        break;
    default:
        showEnemyState (code - ENEMY_0); // Show enemy co-ords and state.
        break;
    }
}

// OBSOLESCENT - 21/1/09 Can do this just by calling tick().
void KGrLevelPlayer::restart()
{
    // bool temp;
    // int i,j;

    // if (editMode)		// Can't move figures when in Edit Mode.
        // return;

    // temp = gameFrozen;

    // gameFrozen = false;	// Temporarily restart the game, by re-running
                                // any timer events that have been blocked.

    // OBSOLESCENT - 7/1/09
    // readMousePos();		// Set hero's direction.
    // hero->doStep();		// Move the hero one step.

    // OBSOLESCENT - 7/1/09
    // j = enemies.count();	// Move each enemy one step.
    // for (i = 0; i < j; i++) {
        // enemy = enemies.at (i);	// Need to use an index because called methods
        // enemy->doStep();	// change the "current()" of the "enemies" list.
    // }

    // OBSOLESCENT - 20/1/09 Need to compile after kgrobject.cpp removed.
    // for (i = 1; i <= 28; i++)
        // for (j = 1; j <= 20; j++) {
            // if ((playfield[i][j]->whatIam() == HOLE) ||
                // (playfield[i][j]->whatIam() == USEDHOLE) ||
                // (playfield[i][j]->whatIam() == BRICK))
                // ((KGrBrick *)playfield[i][j])->doStep();
        // }

    // gameFrozen = temp;	// If frozen was true, halt again, which gives a
                                // single-step effect, otherwise go on running.
}

void KGrLevelPlayer::bugFix()
{
    // Toggle a bug fix on/off dynamically.
    bugFixed = (bugFixed) ? false : true;
    printf ("%s", (bugFixed) ? "\n" : "");
    printf (">>> Bug fix is %s\n", (bugFixed) ? "ON" : "OFF\n");
}

void KGrLevelPlayer::startLogging()
{
    // Toggle logging on/off dynamically.
    gameLogging = (gameLogging) ? false : true;
    printf ("%s", (gameLogging) ? "\n" : "");
    printf (">>> Logging is %s\n", (gameLogging) ? "ON" : "OFF\n");
}

void KGrLevelPlayer::showFigurePositions()
{
    hero->showState ('p');
    foreach (KGrEnemy * enemy, enemies) {
        enemy->showState ('p');
    }
}

void KGrLevelPlayer::showObjectState()
{
    int   i       = targetI;
    int   j       = targetJ;
    char  here    = grid->cellType (i, j);
    Flags access  = grid->heroMoves (i, j);

    int enter     = (access & ENTERABLE)         ? 1 : 0;
    int stand     = (access & dFlag [STAND])     ? 1 : 0;
    int u         = (access & dFlag [UP])        ? 1 : 0;
    int d         = (access & dFlag [DOWN])      ? 1 : 0;
    int l         = (access & dFlag [LEFT])      ? 1 : 0;
    int r         = (access & dFlag [RIGHT])     ? 1 : 0;
    fprintf (stderr,
             "[%02d,%02d] [%c] %02x E %d S %d U %d D %d L %d R %d\n",
	     i, j, here, access, enter, stand, u, d, l, r);

    Flags eAccess = grid->enemyMoves (i, j);
    if (eAccess != access) {
        access    = eAccess;
        enter     = (access & ENTERABLE)         ? 1 : 0;
        stand     = (access & dFlag [STAND])     ? 1 : 0;
        u         = (access & dFlag [UP])        ? 1 : 0;
        d         = (access & dFlag [DOWN])      ? 1 : 0;
        l         = (access & dFlag [LEFT])      ? 1 : 0;
        r         = (access & dFlag [RIGHT])     ? 1 : 0;
        fprintf (stderr,
             "[%02d,%02d] [%c] %02x E %d S %d U %d D %d L %d R %d Enemy\n",
	     i, j, here, access, enter, stand, u, d, l, r);
    }
}

void KGrLevelPlayer::showEnemyState (int enemyId)
{
    if (enemyId < enemies.count()) {
        enemies.at(enemyId)->showState ('s');
    }
}


#include "kgrlevelplayer.moc"
