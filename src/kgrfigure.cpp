/***************************************************************************
 *                      kgrfigure.cpp  -  description                      *
 *                           -------------------                           *
 *    Copyright 2003 Marco Krüger <grisuji@gmx.de>                         *
 *    Copyright 2003 Ian Wadham <ianw2@optusnet.com.au>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

/*
 * Many thanks to Kevin Krammer and Alex Sopicki for translating the
 * original comments in this program code from German into English.
 */

#include "kgrfigure.h"

#include "kgrconsts.h"
#include "kgrobject.h"
#include "kgrcanvas.h"

#include <stdio.h>
#include <QList>

KGrFigure::KGrFigure (int px, int py) : direction (RIGHT)
{
    x = mem_x = px;
    y = mem_y = py;
    relx = mem_relx = 0;
    rely = mem_rely = 0;

    absx = px*16;
    absy = py*16;

    nuggets = 0;
    status = STANDING;

    walkTimer = new QTimer (this);
    fallTimer = new QTimer (this);
}

// Initialise the global settings flags.
bool           KGrFigure::variableTiming = true;
bool           KGrFigure::alwaysCollectNugget    = true;
bool           KGrFigure::runThruHole    = true;
bool           KGrFigure::reappearAtTop  = true;
SearchStrategy KGrFigure::searchStrategy = LOW;

int KGrFigure::herox = 0;
int KGrFigure::heroy = 0;

// Initialise the global game-speed factors.
int KGrFigure::speed = NSPEED;
int KGrBrick::speed  = NSPEED;

// Initialise constants for fixed (KGoldrunner) and variable (Traditional)
// timing.  Each row contains timings for hero walk and fall, enemy walk and
// fall, enemy captured in hole and dug brick.

Timing KGrFigure::fixedTiming =	{45, 50, 55, 100, 500, 40};	// KGr original.

Timing KGrFigure::varTiming[6] = {				// Traditional.
                                {40, 58, 78, 88, 170, 23},	// No enemies.
                                {50, 68, 78, 88, 170, 32},	// 1 enemy.
                                {57, 67, 114, 128, 270, 37},	// 2 enemies.
                                {60, 70, 134, 136, 330, 40},	// 3 enemies.
                                {63, 76, 165, 150, 400, 46},	// 4 enemies.
                                {70, 80, 189, 165, 460, 51}	// >4 enemies.
};

int KGrBrick::HOLETIME = 0;

int KGrFigure::getx()
{
    return absx;
}

int KGrFigure::gety()
{
    return absy;
}

Status KGrFigure::getStatus()
{
    return status;
}

void KGrFigure::init (int a,int b)
{
    walkTimer->stop();
    fallTimer->stop();
    x = mem_x = a;
    y = mem_y = b;
    relx = mem_relx = 0;
    rely = mem_rely = 0;
    nuggets = 0;
    status = STANDING;
}

void KGrFigure:: setNuggets (int n)
{
    nuggets = n;
}


bool KGrFigure::canWalkRight()
{
    return (((*playfield)[x+1][y]->whatIam() != BRICK) &&
            ((*playfield)[x+1][y]->whatIam() != BETON) &&
            ((*playfield)[x+1][y]->whatIam() != FBRICK));
}

bool KGrFigure::canWalkLeft()
{
    return (((*playfield)[x-1][y]->whatIam() != BRICK) &&
            ((*playfield)[x-1][y]->whatIam() != BETON) &&
            ((*playfield)[x-1][y]->whatIam() != FBRICK));
}

bool KGrFigure::canWalkUp()
{
    return (((*playfield)[x][y-1]->whatIam() != BRICK) &&
            ((*playfield)[x][y-1]->whatIam() != BETON) &&
            ((*playfield)[x][y-1]->whatIam() != FBRICK) &&
            ((*playfield)[x][y]->whatIam() == LADDER));
}

bool KGrFigure::canWalkDown()
{
    return (((*playfield)[x][y+1]->whatIam() != BRICK) &&
            ((*playfield)[x][y+1]->whatIam() != BETON) &&
            // v0.3 FIX - Let figure step down into FBRICK from a ladder.
            //	  ((*playfield)[x][y+1]->whatIam() != FBRICK)&&
            (((*playfield)[x][y+1]->whatIam() == LADDER)||
             ((*playfield)[x][y]->whatIam() == LADDER)));
}

bool KGrFigure::canStand()
{
    return (((*playfield)[x][y+1]->whatIam() == BRICK) ||
            ((*playfield)[x][y+1]->whatIam() == BETON) ||
            ((*playfield)[x][y+1]->whatIam() == USEDHOLE)||
            ((*playfield)[x][y+1]->whatIam() == LADDER)||
            ((*playfield)[x][y]->whatIam() == LADDER)||
            standOnEnemy());
}

bool KGrFigure::hangAtPole()
{
    return ((*playfield)[x][y]->whatIam() == POLE);
}

void KGrFigure::walkUp (int WALKDELAY)
{
    actualPixmap = (actualPixmap == CLIMB1) ? CLIMB2 : CLIMB1;
    if (actualPixmap == CLIMB1) {
        emit stepDone (true);
    }
    if (walkCounter++ < gameCycle) {
        // Not end of 4-step cycle: move one step up, if possible.
        if (canWalkUp()) {
            rely -= STEP;
            absy -= STEP;
        }
        walkTimer->setSingleShot (true);
        walkTimer->start ((WALKDELAY * NSPEED) / speed);
    }
    else {
        // End of 4-step cycle: move up to next cell, if possible.
        if (canWalkUp()) {
            y--;
        }
        // Always reset position, in case we are stuck partly into a brick.
        rely = 0;
        absy = y*16;

        // Wait for caller to set next direction.
        status = STANDING;
    }
}

void KGrFigure::walkDown (int WALKDELAY, int FALLDELAY)
{
    if (hangAtPole() || (! canStand())) {
        // On bar or no firm ground underneath: so must fall.
        initFall (FALL2, FALLDELAY);
    }
    else {
        actualPixmap = (actualPixmap == CLIMB1) ? CLIMB2 : CLIMB1;
	if (actualPixmap == CLIMB1) {
            emit stepDone (true);
	}
        if (walkCounter++ < gameCycle) {
            // Not end of 4-step cycle: move one step down, if possible.
            if (canWalkDown()) {
                rely += STEP;
                absy += STEP;
            }
            walkTimer->setSingleShot (true);
            walkTimer->start ((WALKDELAY * NSPEED) / speed);
        }
        else {
            // End of 4-step cycle: move down to next cell, if possible.
            if (canWalkDown()) {
                y++;
            }
            // Always reset position, in case we are stuck partly into a brick.
            rely = 0;
            absy = y*16;

            // Must be able to halt at a pole when going down.
            if (! (canStand() || hangAtPole()))
                initFall (FALL2, FALLDELAY);	// Nothing to hold: so fall.
            else
                // Wait for caller to set next direction.
                status = STANDING;
        }
    }
}

void KGrFigure::walkLeft (int WALKDELAY, int FALLDELAY)
{
    // If counter != 0, the figure is walking, otherwise he is turning around.
    if (walkCounter++ != 0) {
        // Change to the next pixmap in the animation.
        if ((++actualPixmap % gameCycle) != 0) {
            // Not end of 4-pixmap cycle: move one step left, if possible.
            if (canWalkLeft()) {
                relx -= STEP;
                absx -=STEP;
            }
            walkTimer->setSingleShot (true);
            walkTimer->start ((WALKDELAY * NSPEED) / speed);
        }
        else {
            // Check if we are half way through the 8-frame graphics cycle.
            alternateStepGraphics = true;
            if ((actualPixmap % graphicsCycle) == 0) {
                alternateStepGraphics = false;	// Finished the 8-frame cycle.
                actualPixmap -= graphicsCycle;	// Repeat it.
            }

            if (canWalkLeft()) {
                x--;
            }
            // Always reset position, in case we are stuck partly into a brick.
            relx = 0;
            absx = x*16;

	    int frame = actualPixmap % graphicsCycle;
	    if ((frame == 0) || (frame == 4)) {
                emit stepDone(hangAtPole());
	    }
            // If cannot stand or hang, start fall, else await next assignment.
            if (! (canStand() || hangAtPole()))
                initFall (FALL1, FALLDELAY);
            else
                status = STANDING;	// Caller should set next direction.
        }
    }
    else {
        status = STANDING;		// The figure is turning around.
    }
}

void KGrFigure::walkRight (int WALKDELAY, int FALLDELAY)
{
    if (walkCounter++) {		// If 0, just turn the figure around.
        if ((++actualPixmap % gameCycle) != 0)  {
            // The 4-frame cycle per playfield tile is incomplete.
            if (canWalkRight()) {	// Move right, if it's possible.
                relx += STEP;
                absx += STEP;
            }
            walkTimer->setSingleShot (true);
            walkTimer->start ((WALKDELAY * NSPEED) / speed);
        }
        else {
            // Check if we are half way through the 8-frame graphics cycle.
            alternateStepGraphics = true;
            if ((actualPixmap % graphicsCycle) == 0) {
                alternateStepGraphics = false;	// Finished the 8-frame cycle.
                actualPixmap -= graphicsCycle;	// Repeat it.
            }

            if (canWalkRight()) {
                x++;
            }				// Set the figure's new position.
            // Always reset position, in case we are stuck partly into a brick.
            relx = 0;
            absx = x*16;
	    int frame = actualPixmap % graphicsCycle;
	    if ((frame == 0) || (frame == 4)) {
                emit stepDone(hangAtPole());
	    }

            if (!(canStand()||hangAtPole())) // Cannot hold on: so fall.
                initFall (FALL2, FALLDELAY);
            else
                status = STANDING;	     // Else, stand.
        }
    }
    else {
        status = STANDING;		// Just turn the figure around.
    }
}

void KGrFigure::initFall (int apm, int FALLDELAY)
{
    emit falling (true);		// Start the falling sound.
    status = FALLING;
    actualPixmap = apm;
    walkCounter=1;
    walkTimer->stop();
    fallTimer->setSingleShot (true);
    fallTimer->start ((FALLDELAY * NSPEED) / speed);
}

void KGrFigure::showFigure()
{
}

void KGrFigure::setPlayfield (KGrObject * (*p)[30][22])
{
    playfield = p;
}

KGrFigure::~KGrFigure()
{
}

KGrHero::KGrHero (KGrCanvas * view, int x, int y)
    :KGrFigure (x, y)
{
    heroView = view;
    status = STANDING;
    actualPixmap = FALL1;

    herox = x;
    heroy = y;

    started = false;
    mouseMode = true;
    walkCounter = 1;

    walkFrozen = false;
    fallFrozen = false;
    alternateStepGraphics = false;

    connect (walkTimer, SIGNAL (timeout()), SLOT (walkTimeDone()));
    connect (fallTimer, SIGNAL (timeout()), SLOT (fallTimeDone()));
}

int KGrHero::WALKDELAY = 0;
int KGrHero::FALLDELAY = 0;

/* It is necessary to execute startWalk before the timer-function code, in order
    to take changes of direction into account when doing the animation steps.
*/
void KGrHero::startWalk()
{
    switch (nextDir) {
        case UP:
            if ((*playfield)[x][y]->whatIam() == LADDER) {
                walkCounter = 1;
                direction = UP;
            }
            break;
        case RIGHT:
            if (hangAtPole()) 
                actualPixmap = (alternateStepGraphics)? RIGHTCLIMB5:RIGHTCLIMB1;
            else 
                actualPixmap = (alternateStepGraphics)? RIGHTWALK5 : RIGHTWALK1;
            if (direction != RIGHT)
                walkCounter = 0;
            else
                walkCounter = 1;
            direction = RIGHT;
            break;
        case DOWN:
            if (((*playfield)[x][y]->whatIam() == LADDER)||
                ((*playfield)[x][y+1]->whatIam() == LADDER))
            {walkCounter = 1;
            direction = DOWN;}
        else			// If the hero is hanging from a pole and there
                                  // is nothing to stand on, let him fall.
            if (hangAtPole() && (!canStand())) {
                status = STANDING;
                actualPixmap = (direction==RIGHT) ? FALL2 : FALL1;
                walkCounter = 1;
                direction = STAND;
                walkTimer->stop();
            }
            break;
        case LEFT:
            if (hangAtPole())
                actualPixmap = (alternateStepGraphics)? LEFTCLIMB5 : LEFTCLIMB1;
            else
                actualPixmap = (alternateStepGraphics)? LEFTWALK5 : LEFTWALK1;
            if (direction != LEFT)
                walkCounter = 0;
            else
                walkCounter = 1;
            direction = LEFT;
            break;
        default :
            direction = STAND;
            status = FALLING;
            break;
    }
    nextDir = STAND;
    if (status != FALLING) {	// Always execute, unless we are falling, in
        status = WALKING;	// which case the state change would be wrong
        showFigure();		// and the wrong timer would be triggered.
    }
} // END KGrHero::startWalk

void KGrHero::setKey (Direction key)
{
    // Keyboard control of hero: direction is fixed until next key is pressed.
    // Sets a simulated mouse-pointer above, below, left, right or on the hero.
    mouseMode = false;
    stopped = false;
    switch (key) {
    case UP:	mousex = x; mousey = 0; break;
    case DOWN:	mousex = x; mousey = FIELDHEIGHT + 1; break;
    case LEFT:	mousex = 0; mousey = y; break;
    case RIGHT:	mousex = FIELDWIDTH + 1; mousey = y; break;
    case STAND:	stopped = true;  mousex = x; mousey = y; break;
    }
}

void KGrHero::setDirection (int i, int j)
{
    // Mouse control of hero: direction is updated continually on a timer.
    mouseMode = true;
    stopped = false;
    mousex = i;
    mousey = j;
}

void KGrHero::setNextDir()
{
    int dx, dy;

    if (! mouseMode) {
        // Keyboard control of hero: adjust simulated mouse-pointer.
        if (stopped) {
            mousex = x;
            mousey = y;
        }
        if ((mousey < 1) || (mousey > FIELDHEIGHT)) {
            mousex = x;		// Stay directly above/below the hero.
        }
        else if ((mousex < 1) || (mousex > FIELDWIDTH)) {
            mousey = y;		// Stay directly left/right of the hero.
        }
    }

    dx = mousex - x; dy = mousey - y;

    if ((dy == 0) && (y == 1) && (nuggets <= 0)) {
        nextDir = UP;
    }
    else if ((dy > 0) &&
             (canWalkDown() ||
              // Permanent fix.  Removing the next test makes enemies
              // behave like bricks if you are standing, walking or falling on
              // their heads.  So you cannot die when walking over a trapped
              // enemy in mouse-mode, preventing a common beginner's problem.
              // standOnEnemy() ||	// Removed, 10 Aug 07.
              (hangAtPole() && ((*playfield)[x][y+1]->whatIam() != BRICK) &&
                               ((*playfield)[x][y+1]->whatIam() != BETON)))) {
        nextDir = DOWN;
    }
    else if ((dy < 0) && canWalkUp()) {
        nextDir = UP;
    }
    else if (dx > 0) {
        nextDir = RIGHT;
    }
    else if (dx < 0) {
        nextDir = LEFT;
    }
    else if (dx == 0) {
        nextDir = STAND;
    }
}

void KGrHero::doStep() {
    if (walkFrozen) {
        walkFrozen = false;
        walkTimeDone();
    }
    if (fallFrozen) {
        fallFrozen = false;
        fallTimeDone();
    }
}

void KGrHero::showState (char option)
{
    printf ("(%02d,%02d) - Hero      ", x, y);
    switch (option) {
        case 'p': printf ("\n"); break;
        case 's': printf (" nuggets %02d status %d walk-ctr %d ",
                          nuggets, status, walkCounter);
            printf ("dirn %d next dirn %d\n", direction, nextDir);
            printf ("                     rel (%02d,%02d) abs (%03d,%03d)",
                        relx, rely, absx, absy);
            printf (" pix %02d", actualPixmap);
            printf (" mem %d %d %d %d", mem_x, mem_y, mem_relx, mem_rely);
            if (walkFrozen) printf (" wBlock");
            if (fallFrozen) printf (" fBlock");
            printf ("\n");
            break;
    }
}

void KGrHero::init (int a,int b)
{
    walkTimer->stop();
    fallTimer->stop();
    walkCounter = 1;
    started = false;

    x = mem_x = a;
    y = mem_y = b;
    relx = mem_relx = 0;
    rely = mem_rely = 0;

    absx = 16*x;
    absy = 16*y;

    nuggets = 0;

    if (herox < 1) {				// If first call to init, ...
        heroView->makeHeroSprite (x, y, actualPixmap);
    }
    herox = x;
    heroy = y;

    actualPixmap = FALL2;
    heroView->moveHero (absx, absy, actualPixmap);
}

void KGrHero::start()
{
    started = true;
    walkFrozen = false;
    fallFrozen = false;

    if (!(canStand()||hangAtPole())) {		// Hero must fall ...
        status = FALLING;
	emit falling (true);			// Start the falling sound.
        fallTimeDone();
    }
    else {
        status = STANDING;
        walkTimeDone();
    }
}

void KGrHero::setSpeed (int gamespeed)
{
    if (gamespeed >= 0) {
        if (gamespeed < MINSPEED)
            speed++;		// Increase speed.
        else
            speed = gamespeed;	// Set selected speed.
        if (speed > MAXSPEED)
            speed = MAXSPEED;	// Set upper limit.
    }
    else {
        speed--;			// Reduce speed.
        if (speed < MINSPEED)
            speed = MINSPEED;	// Set lower limit.
    }
    
    KGrBrick::speed = speed;	// Make a copy for bricks.
}
    
void KGrHero::walkTimeDone()
{
    if (! started) return;	// Ignore signals from earlier play.
    if (KGrObject::frozen) {
        walkFrozen = true;
        return;
    }

    if ((*playfield)[x][y]->whatIam() == BRICK) {
        emit caughtHero();	// Brick closed over hero.
        return;
    }

    if ((y == 1) && (nuggets <= 0)) {// If on top row and all nuggets collected,
        emit leaveLevel();	// the hero has won and can go to next level.
        return;
    }

    if (status == STANDING)
        setNextDir();
    if ((status == STANDING) && (nextDir != STAND)) {
        if ((standOnEnemy()) && (nextDir == DOWN)) {
            emit caughtHero();	// Hero is going to step down into an enemy.
            return;
        }
        startWalk();
    }
    if (status != STANDING) {
        switch (direction) {
        case UP:		walkUp (WALKDELAY); break;
        case DOWN:	walkDown (WALKDELAY, FALLDELAY); break;
        case RIGHT:	walkRight (WALKDELAY, FALLDELAY); break;
        case LEFT:	walkLeft (WALKDELAY, FALLDELAY); break;
        default :
        // The following code is strange.  It makes the hero fall off a pole.
        // It works because of other strange code in "startWalk(), case DOWN:".
            if (!canStand() || hangAtPole()) // falling
                initFall (FALL1, FALLDELAY);
            else  status = STANDING;
        break;
        }
        herox=x; heroy=y;		// Set the hero's new position.
        if ((relx==0)&&(rely==0)) {	// If he has just completed a move, see
            collectNugget();		// if there is a nugget to collect.
        }
    }
    if (status == STANDING)
        if (!canStand()&&!hangAtPole())
            initFall (FALL1, FALLDELAY);
        else {
            walkTimer->setSingleShot (true);
            walkTimer->start ((WALKDELAY * NSPEED) / speed);
        }
    
    // This additional showFigure() is to update the hero position after it is
    // altered by the hero-enemy deadlock fix in standOnEnemy().  Messy, but ...
    ////////////////////////////////////////////////////////////////////////////
    showFigure();
    if (isInEnemy()) {
        walkTimer->stop();
        emit caughtHero();
    }
}
    
void KGrHero::fallTimeDone()
{
    if (! started) return;		// Ignore signals from earlier play.
    if (KGrObject::frozen) {
        fallFrozen = true;
        return;
    }

    if (!standOnEnemy()) {
        if (walkCounter++ < gameCycle) {// The hero must fall four steps.
            fallTimer->setSingleShot (true);
            fallTimer->start ((FALLDELAY * NSPEED) / speed);
            rely+=STEP;
            absy+=STEP;
        }
        else {				// When done, move to the start position
                                        // of the next object (brick, etc.).
            heroy = ++y;
            rely = 0;
            absy = y*16;
            collectNugget();		// See if there is a nugget to collect.
            if (! (canStand()||hangAtPole())) {	// The hero goes on falling.
                fallTimer->setSingleShot (true);
                fallTimer->start ((FALLDELAY * NSPEED) / speed);
                walkCounter = 1;
            }
            else {			// The hero can stand or hang on a pole,
                status = STANDING;	// so change his state to STANDING.
                walkTimer->setSingleShot (true);
                walkTimer->start ((WALKDELAY * NSPEED) / speed);
                direction = (actualPixmap == FALL2) ? RIGHT : LEFT;
                if ((*playfield)[x][y]->whatIam() == POLE)
                    actualPixmap = (direction == RIGHT)? RIGHTCLIMB1:LEFTCLIMB1;
                // else
                    // Reduce jerkiness when descending over a falling enemy.
                    // actualPixmap = (direction==RIGHT)? RIGHTWALK1:LEFTWALK1;
            }
        }
        showFigure();
    }
    else {
        if (rely == 0) {
            // If at the bottom of a cell, try to walk or just stand still.
            status = STANDING;
            direction = (actualPixmap == FALL2) ? RIGHT : LEFT;
            if ((*playfield)[x][y]->whatIam() == POLE)
                actualPixmap = (direction == RIGHT)? RIGHTCLIMB1:LEFTCLIMB1;
            // else
                // Reduce jerkiness when descending over a falling enemy.
                // actualPixmap = (direction == RIGHT)? RIGHTWALK1:LEFTWALK1;
            walkTimer->setSingleShot (true);
            walkTimer->start ((WALKDELAY * NSPEED) / speed);
        }
        else {
            // Else, freeze hero until enemy moves out of the way.
            fallTimer->setSingleShot (true);
            fallTimer->start ((FALLDELAY * NSPEED) / speed);
        }
    }
    if (status != FALLING) {
        emit falling (false);			// Stop the falling sound.
    }
    if (isInEnemy() && (! standOnEnemy()))
        emit caughtHero();
}


void KGrHero::showFigure() {

    heroView->moveHero (absx, absy, actualPixmap);

    // Save old values for when we delete the figure from its old position later.
    mem_x = x;
    mem_y = y;
    mem_relx = relx;
    mem_rely = rely;
}

void KGrHero::dig()
{
    if (direction == LEFT)
        digLeft();
    else if (direction == RIGHT)
        digRight();
}
        
void KGrHero::digLeft()
{
    int i = 1;		// If stationary or moving up/down, dig at x-1.
    if (status == STANDING)
        setNextDir();
    if ((status == WALKING) ||
        ((status == STANDING) && ((nextDir == LEFT) || (nextDir == RIGHT)))) {
        if ((direction == LEFT) && canWalkLeft())
            i = 2;	// If walking left, dig at x-2 and stop at x-1.
        else if ((direction == RIGHT) && canWalkRight())
            i = 0;	// If walking right, dig at x and stop at x+1.
    }
    int tileType = (*playfield)[x - i][y + 1]->whatIam();
    int tile2Type = (*playfield)[x - i][y]->whatIam();
    if ((tileType == BRICK) &&
        ((tile2Type == HLADDER) || (tile2Type == FREE) || (tile2Type == HOLE))) {
        ((KGrBrick*)(*playfield)[x-i][y+1])->dig();
        emit digs();
    }
}

void KGrHero::digRight()
{
    int i = 1;		// If stationary or moving up/down, dig at x+1.
    if (status == STANDING)
        setNextDir();
    if ((status == WALKING) ||
        ((status == STANDING) && ((nextDir == LEFT) || (nextDir == RIGHT)))) {
        if ((direction == LEFT) && canWalkLeft())
            i = 0;	// If walking left, dig at x and stop at x-1.
        else if ((direction == RIGHT) && canWalkRight())
            i = 2;	// If walking right, dig at x+2 and stop at x+1.
    }
    int tileType = (*playfield)[x + i][y + 1]->whatIam();
    int tile2Type = (*playfield)[x + i][y]->whatIam();
    if ((tileType == BRICK) &&
        ((tile2Type == HLADDER) || (tile2Type == FREE) || (tile2Type == HOLE))) {
        ((KGrBrick*)(*playfield)[x+i][y+1])->dig();
        emit digs();
    }
}

void KGrHero::setEnemyList (QList<KGrEnemy *> * e)
{
    enemies = e;
}

bool KGrHero::standOnEnemy()
{
    int c = 0;
    int range = enemies->count();
    if (range > 0) {
        for (KGrEnemy * enemy = enemies->at (c); c < range;) {
            enemy = enemies->at (c++);
            // Test if hero's foot is at or just below enemy's head (tolerance
            // of 4 pixels) and the two figures overlap in the X direction.
            if ((((absy + 16) == enemy->gety()) ||
                ((absy + 12) == enemy->gety())) &&
               (((absx - 16) <  enemy->getx()) &&
                ((absx + 16) >  enemy->getx()))) {
               if (((absy + 12) == enemy->gety()) &&
                    (enemy->getStatus() != FALLING)) {
                    absy = absy - rely; // Bounce back from overlap, to avoid
                    rely = 0;           // hero-enemy mid-cycle deadlock.
                    walkCounter = 1;
                }
                return true;
            }
        }
    }
    return false;
}

void KGrHero::collectNugget()
{
    if ((*playfield)[x][y]->whatIam() == NUGGET) {
        ((KGrFree *)(*playfield)[x][y])->setNugget (false);
        if (!(--nuggets))
            emit haveAllNuggets();// Tell the application that all nuggets are
                                // gone, so that it can show the hidden ladders.
        emit gotNugget (250);	// Tell the application to increase the score.
    }
}
        
void KGrHero::loseNugget()
{
    // Enemy trapped or dead and could not drop nugget (NO SCORE for this).
    if (! (--nuggets))
        emit haveAllNuggets();	// Tell the application that all the nuggets are
                                // gone, so that it can show the hidden ladders.
}

bool KGrHero::isInEnemy()
{
    int c = 0;
    int range = enemies->count();
    if (range)
        for (KGrEnemy *enemy = enemies->at (c); c < range;) {
            enemy = enemies->at (c++);
            if (isInside (enemy->getx(), enemy->gety()) ||
                isInside (enemy->getx() - 15, enemy->gety()) ||
                isInside (enemy->getx(), enemy->gety() - 15))
            return true;
        }
    return false;
}
    
bool KGrHero::isInside (int enemyx, int enemyy)
{
    return ((absx >= enemyx) &&
            (absx <= enemyx + 15) &&
            (absy >= enemyy) &&
            (absy <= enemyy + 15));
}


KGrHero::~KGrHero()
{
    delete walkTimer;
    delete fallTimer;
}


KGrEnemy::KGrEnemy (KGrCanvas * view, int x, int y)
    : KGrFigure (x, y)
{
    enemyView = view;
    actualPixmap = FALL1;
    nuggets = 0;
    enemyView->makeEnemySprite (x, y, actualPixmap);

    walkCounter = 1;
    captiveCounter = 0;

    searchStatus = HORIZONTAL;

    birthX = x;
    birthY = y;

    alternateStepGraphics = false;
    walkFrozen = false;
    fallFrozen = false;
    captiveFrozen = false;

    captiveTimer = new QTimer (this);
    connect (captiveTimer,SIGNAL (timeout()),SLOT (captiveTimeDone()));
    connect (walkTimer, SIGNAL (timeout()), SLOT (walkTimeDone()));
    connect (fallTimer, SIGNAL (timeout()), SLOT (fallTimeDone()));
}

int KGrEnemy::WALKDELAY = 0;
int KGrEnemy::FALLDELAY = 0;
int KGrEnemy::CAPTIVEDELAY = 0;

void KGrEnemy::doStep() {
    if (walkFrozen) {
        walkFrozen = false;
        walkTimeDone();
    }
    if (fallFrozen) {
        fallFrozen = false;
        fallTimeDone();
    }
    if (captiveFrozen) {
        captiveFrozen = false;
        captiveTimeDone();
    }
}

void KGrEnemy::showState (char option)
{
    printf ("(%02d,%02d) - Enemy  [%d]", x, y, enemyId);
    switch (option) {
        case 'p': printf ("\n"); break;
        case 's': printf (" nuggets %02d status %d walk-ctr %d ",
                          nuggets, status, walkCounter);
            printf ("dirn %d search %d capt-ctr %d\n",
                        direction, searchStatus, captiveCounter);
            printf ("                     rel (%02d,%02d) abs (%03d,%03d)",
                        relx, rely, absx, absy);
            printf (" pix %02d", actualPixmap);
            printf (" mem %d %d %d %d", mem_x, mem_y, mem_relx, mem_rely);
            if (walkFrozen) printf (" wBlock");
            if (fallFrozen) printf (" fBlock");
            if (captiveFrozen) printf (" cBlock");
            printf ("\n");
            break;
    }
}

void KGrEnemy::init (int a,int b)
{
    walkTimer->stop();		// Stop all timers (avoid run-ons of old level).
    fallTimer->stop();
    captiveTimer->stop();
    walkCounter = 1;
    captiveCounter = 0;

    x = mem_x = a;
    y = mem_y = b;
    relx = mem_relx = 0;
    rely = mem_rely = 0;

    absx=16*x;
    absy=16*y;

    actualPixmap = FALL2;

    status = STANDING;
}

void KGrEnemy::walkTimeDone()
{
    if (KGrObject::frozen) {
        walkFrozen = true;
        return;
    }

    // Check we are alive BEFORE checking for friends being in the way.
    // Maybe a friend overhead is blocking our escape from a brick.
    if ((*playfield)[x][y]->whatIam()==BRICK) {	// If stuck in a brick, die.
        dieAndReappear();
        return;			// Must leave "walkTimeDone" when an enemy dies.
    }
    
    if (! bumpingFriend()) {
        switch (direction) {
            case UP:	walkUp (WALKDELAY);
                        if ((rely == 0) &&
                            ((*playfield)[x][y+1]->whatIam() == USEDHOLE))
                            // Enemy climbs out of hole, mark it as free.
                            ((KGrBrick *)(*playfield)[x][y+1])->unUseHole();
                            break;
            case DOWN:	walkDown (WALKDELAY, FALLDELAY); break;
            case RIGHT:	walkRight (WALKDELAY, FALLDELAY); break;
            case LEFT:	walkLeft (WALKDELAY, FALLDELAY); break;
            default:	// Switch search direction in KGoldrunner search (only).
                        searchStatus = (searchStatus==VERTIKAL) ?
                                        HORIZONTAL : VERTIKAL;
            
                        // In KGoldrunner rules, if a hole opens under an enemy
                        // who is standing and waiting to move, he should fall.
                        if (!(canStand()||hangAtPole())) {
                            initFall (actualPixmap, FALLDELAY);
                        }
                        else {
                            status = STANDING;
                        }

                        break;
        }
        // If we have completed a move, look for the hero again.
        if (status == STANDING) {
            direction = searchbestway (x,y,herox,heroy);
            if (walkCounter >= gameCycle) {
                if (! nuggets)
                    collectNugget();
                else
                    dropNugget();
            }
            status = WALKING;
            walkCounter = 1;
            walkTimer->setSingleShot (true);
            walkTimer->start ((WALKDELAY * NSPEED) / speed);
            startWalk();
        }
    }
    else {
        // A friend is in the way.  Try a new dirn., but not if leaving a hole.
        Direction dirn;
                    
        // In KGoldrunner rules, change the search strategy,
        // to avoid enemy-enemy deadlock.
        searchStatus = (searchStatus==VERTIKAL) ? HORIZONTAL : VERTIKAL;

        dirn = searchbestway (x, y, herox, heroy);
        if ((dirn != direction) && ((*playfield)[x][y]->whatIam() != USEDHOLE))
        {
            direction = dirn;
            status = WALKING;
            walkCounter = 1;
            relx = 0; absx = 16 * x;
            rely = 0; absy = 16 * y;
            startWalk();
        }
        walkTimer->setSingleShot (true);
        walkTimer->start ((WALKDELAY * NSPEED) / speed);
    }
    showFigure();
}

void KGrEnemy::fallTimeDone()
{
    if (KGrObject::frozen) {
        fallFrozen = true;
        return;
    }

    if ((*playfield)[x][y+1]->whatIam() == HOLE) { // If the enemy is falling into
        ((KGrBrick*)(*playfield)[x][y+1])->useHole();// a hole, mark it as used and
        if (nuggets) {				 // drop any gold he is holding.
            nuggets = 0;
            switch ((*playfield)[x][y]->whatIam()) {
            case FREE:
            case HLADDER:	
                ((KGrFree *)(*playfield)[x][y])->setNugget (true); break;
            default:
                emit lostNugget(); break;	// Cannot drop the nugget here.
            }
        }
        emit trapped (75);			// Enemy trapped: score 75.
    }
    else if (walkCounter <= 1) {
        // Enemies collect nuggets when falling.
        if (!nuggets) {
            collectNugget();
        }
    }

    if ((*playfield)[x][y]->whatIam() == BRICK) { // If stuck in a brick, die.
        dieAndReappear();
        return;			// Must leave "fallTimeDone" when an enemy dies.
    }
    
    if (standOnEnemy()) {			// Don't fall into a friend.
        fallTimer->setSingleShot (true);
        fallTimer->start ((FALLDELAY * NSPEED) / speed);
        return;
    }
    
    if (walkCounter++ < gameCycle) {
        fallTimer->setSingleShot (true);
        fallTimer->start ((FALLDELAY * NSPEED) / speed);
            { rely+=STEP; absy+=STEP;}
    }
    else {
        rely = 0; y ++; absy=16*y;
        if ((*playfield)[x][y]->whatIam() == USEDHOLE) {
            captiveCounter = 0;
            status = CAPTIVE;
            captiveTimer->setSingleShot (true);
            captiveTimer->start ((CAPTIVEDELAY * NSPEED) / speed);
        }
        else if (!(canStand()||hangAtPole())) {
            fallTimer->setSingleShot (true);
            fallTimer->start ((FALLDELAY * NSPEED) / speed);
            walkCounter=1;
        }
        else {
            status = STANDING;
            if (hangAtPole())
                actualPixmap=(direction ==RIGHT)?RIGHTCLIMB1:LEFTCLIMB1;
        }
    }

    if (status == STANDING) {
        status = WALKING;
        walkCounter = 1;
        direction = searchbestway (x,y,herox,heroy);
        walkTimer->setSingleShot (true);
        walkTimer->start ((WALKDELAY * NSPEED) / speed);
        startWalk();
        if (!nuggets)
            collectNugget();
        else
            dropNugget();
    }
    showFigure();
}

void KGrEnemy::captiveTimeDone()
{
    if (KGrObject::frozen) {captiveFrozen = true; return; }
    if ((*playfield)[x][y]->whatIam()==BRICK)
        dieAndReappear();
    else
        if (captiveCounter > 6) {
            status = WALKING;
            walkCounter = 1;
            direction = UP;
            walkTimer->setSingleShot (true);
            walkTimer->start ((WALKDELAY * NSPEED) / speed);
            captiveCounter = 0;
        }
        else {
            captiveCounter ++;
            captiveTimer->setSingleShot (true);
            captiveTimer->start ((CAPTIVEDELAY * NSPEED) / speed);
            showFigure();
        }
}
    
void KGrEnemy::startSearching()
{
    // Called from "KGoldrunner::startPlaying" and "KGrEnemy::dieAndReappear".
    init (x,y);

    if (canStand()||((*playfield)[x][y+1]->whatIam()==USEDHOLE)) {
        status = WALKING;
        walkTimer->setSingleShot (true);
        walkTimer->start ((WALKDELAY * NSPEED) / speed);
    }
    else {
        status = FALLING;
        fallTimer->setSingleShot (true);
        fallTimer->start ((FALLDELAY * NSPEED) / speed);
    }

    walkCounter = 1;
    direction = searchbestway (x,y,herox,heroy);
    startWalk();
}

void KGrEnemy::collectNugget()
{
    if (((*playfield)[x][y]->whatIam() == NUGGET) && (nuggets == 0) &&
        (alwaysCollectNugget || ((int)(5.0*rand()/RAND_MAX+1.0) > 4))) {
        ((KGrFree *)(*playfield)[x][y])->setNugget (false);
        nuggets=1;
    }
}

void KGrEnemy::dropNugget()
{
    if (((int)(DROPNUGGETDELAY*rand()/RAND_MAX+1.0) > DROPNUGGETDELAY-5) &&
        ((*playfield)[x][y]->whatIam() == FREE)) {
        ((KGrFree *)(*playfield)[x][y])->setNugget (true);
        nuggets=0;
    }
}

void KGrEnemy::showFigure()
{
    enemyView->moveEnemy (enemyId, absx, absy, actualPixmap, nuggets);

    // Save old values for when we delete the figure from its old position later.
    mem_x = x;
    mem_y = y;
    mem_relx = relx;
    mem_rely = rely;
}

bool KGrEnemy::canWalkUp()
{
    return (((*playfield)[x][y-1]->whatIam() != BRICK) &&
            ((*playfield)[x][y-1]->whatIam() != BETON) &&
            ((*playfield)[x][y-1]->whatIam() != FBRICK) &&
            (((*playfield)[x][y]->whatIam() == USEDHOLE) ||
             ((*playfield)[x][y]->whatIam() == LADDER)));
}

void KGrEnemy::startWalk()
{
    switch (direction) {
    case UP:
        break;
    case RIGHT:
        if (hangAtPole())
            actualPixmap = (alternateStepGraphics) ? RIGHTCLIMB5 : RIGHTCLIMB1;
        else
            actualPixmap = (alternateStepGraphics) ? RIGHTWALK5 : RIGHTWALK1;
        break;
    case DOWN:
        break;
    case LEFT:
        if (hangAtPole())
            actualPixmap = (alternateStepGraphics) ? LEFTCLIMB5 : LEFTCLIMB1;
        else
            actualPixmap = (alternateStepGraphics) ? LEFTWALK5 : LEFTWALK1;
        break;
    default:
        break;
    }
}

int KGrEnemy::reappearIndex = FIELDWIDTH;
int KGrEnemy::reappearPos [FIELDWIDTH];

void KGrEnemy::makeReappearanceSequence()
{
    // The idea is to make each possible x co-ord come up once per FIELDWIDTH
    // reappearances of enemies.  This is not truly random, but it reduces the
    // tedium in levels where you must keep killing enemies until a particular
    // x or range of x comes up (e.g. if they have to collect gold for you).

    // First put the positions in ascending sequence.
    for (int k = 0; k < FIELDWIDTH; k++) {
        reappearPos [k] = k + 1;
    }

    int z;
    int left = FIELDWIDTH;
    int temp;

    // Shuffle the x co-ords of reappearance positions, for x = 1 to FIELDWIDTH.
    for (int k = 0; k < FIELDWIDTH; k++) {
        // Pick a random element from those that are left.
        z = (int)((left * (float) rand()) / RAND_MAX);
        // Exchange its value with the last of the ones left.
        temp = reappearPos [z];
        reappearPos [z] = reappearPos [left - 1];
        reappearPos [left - 1] = temp;
        left--;
    }
    reappearIndex = 0;
}

void KGrEnemy::dieAndReappear()
{
    bool looking;
    int i;

    if (nuggets > 0) {
        nuggets = 0;			// Enemy died and could not drop nugget.
        emit lostNugget();		// NO SCORE for lost nugget.
    }
    emit killed (75);			// Killed an enemy: score 75.

    if (reappearAtTop) {
        // Follow Traditional rules: enemies reappear at top.
        looking = true;
        y = 2;
        // Randomly look for a free spot in row 2.  Limit the number of tries.
        for (i = 1; ((i <= 3) && looking); i++) {
            if (reappearIndex >= FIELDWIDTH) {
                makeReappearanceSequence();	// Get next array of random x.
            }
            x = reappearPos [reappearIndex++];
            switch ((*playfield)[x][y]->whatIam()) {
            case FREE:
            case HLADDER:
                looking = false;
                break;
            default:
                break;
            }
        }
        // If unsuccessful, choose the first free spot in row 3 or below.
        while ((y < FIELDHEIGHT) && looking) {
            y++;
            x = 0;
            while ((x < FIELDWIDTH) && looking) {
                x++;
                switch ((*playfield)[x][y]->whatIam()) {
                case FREE:
                case HLADDER:
                    looking = false;
                    break;
                default:
                    break;
                }
            }
        }
    }
    else {
        // Follow KGoldrunner rules: enemies reappear where they started.
        x = birthX;
        y = birthY;
    }

    // Enemy reappears and starts searching for the hero.
    startSearching();
}

Direction KGrEnemy::searchbestway (int ew, int eh, int hw, int hh)
{
    Direction dirn;

    if ((*playfield)[x][y]->whatIam() == USEDHOLE)	// Could not get out of
        return UP;					// hole (eg. brick above
                                                        // closed): keep trying.
    
    if (!canStand() &&				// Cannot stand,
        !hangAtPole() &&				// not on pole, not
        !standOnEnemy() &&				// walking on friend and
        !((*playfield)[x][y+1]->whatIam() == HOLE))	// not just out of hole,
        return DOWN;				// so must fall.
    
    switch (searchStrategy) {
    
        // Traditional search strategy.
        case LOW:
        dirn = STAND;
        if (eh == hh) {					// Hero on same row.
            dirn = lowGetHero (ew, eh, hw);
        }
        if (dirn != STAND) return (dirn);		// Can go towards him.
    
        if (eh >= hh) {					// Hero above enemy.
            dirn = lowSearchUp (ew, eh, hh);		// Find a way up.
        }
        else {						// Hero below enemy.
            dirn = lowSearchDown (ew, eh, hh);		// Find way down to him.
            if (dirn == STAND) {
                dirn = lowSearchUp (ew, eh, hh);	// No go: try up.
            }
        }

        if (dirn == STAND) {				// When all else fails,
            dirn = lowSearchDown (ew, eh, eh - 1);	// find way below hero.
        }
        return dirn;
        break;

        // KGoldrunner search strategy.
        case MEDIUM:
        case HIGH:
        if (searchStatus==VERTIKAL) {
            if (eh > hh)
                return searchupway (ew, eh);
            if (eh < hh)
                return searchdownway (ew, eh);
            return STAND;
        }
        else {
            if (ew > hw)
                return searchleftway (ew,eh);
            if (ew < hw)
                return searchrightway (ew,eh);
            return STAND;
        }
        break;
    }
    return STAND;
}
        
///////////////////////////////////////////////
// Methods for medium-level search strategy. //
///////////////////////////////////////////////

Direction KGrEnemy::searchdownway (int ew, int eh)
{
    int i, j;
    i = j = ew;
    if ((*playfield)[ew][eh]->searchValue & CANWALKDOWN)
        return DOWN;
    else
        while ((i >= 0) || (j <= FIELDWIDTH)) {
            if (i >= 0)
                if ((*playfield)[i][eh]->searchValue & CANWALKDOWN)
                    return LEFT;
                else if (!(((*playfield)[i--][eh]->searchValue & CANWALKLEFT) ||
                    (runThruHole && ((*playfield)[i][eh]->whatIam() == HOLE))))
                    i = -1;
            if (j <= FIELDWIDTH)
                if ((*playfield)[j][eh]->searchValue & CANWALKDOWN)
                    return RIGHT;
                else
                    if (!(((*playfield)[j++][eh]->searchValue & CANWALKRIGHT) ||
                    (runThruHole && ((*playfield)[j][eh]->whatIam() == HOLE))))
                    j = FIELDWIDTH + 1;
        }
    return STAND;
}
           
Direction KGrEnemy::searchupway (int ew, int eh)
{
    int i, j;
    i = j = ew;
    if ((*playfield)[ew][eh]->searchValue & CANWALKUP)
        return UP;
    else
        while ((i >= 0) || (j <= FIELDWIDTH)) {	// search for the first way up
            if (i >= 0)
                if ((*playfield)[i][eh]->searchValue & CANWALKUP)
                    return LEFT;
                else if (!(((*playfield)[i--][eh]->searchValue & CANWALKLEFT) ||
                    (runThruHole && ((*playfield)[i][eh]->whatIam() == HOLE))))
                    i = -1;
        if (j<=FIELDWIDTH)
            if ((*playfield)[j][eh]->searchValue & CANWALKUP)
                return RIGHT;
            else
                if (!(((*playfield)[j++][eh]->searchValue & CANWALKRIGHT) ||
                    (runThruHole && ((*playfield)[j][eh]->whatIam() == HOLE))))
                    j = FIELDWIDTH + 1;
        }
    // BUG FIX - Ian W., 30/4/01 - Don't leave an enemy standing in mid air.
    if (!canStand())
        return DOWN;
    else
        return STAND;
}
       
Direction KGrEnemy::searchleftway (int ew, int eh)
{
    int i, j;
    i = j = eh;
    if (((*playfield)[ew][eh]->searchValue & CANWALKLEFT) || /* Can walk left? */
             (runThruHole && ((*playfield)[ew-1][eh]->whatIam() == HOLE)))
        return LEFT;
    else while ((i >= 0) || (j <= FIELDHEIGHT)) {	/* Within bounds? */
        if (i>=0)
            if (((*playfield)[ew][i]->searchValue & CANWALKLEFT) || /* Up and left? */
                (runThruHole && ((*playfield)[ew-1][i]->whatIam() == HOLE)))
                return UP;					/* Go up. */
        else
            if (!((*playfield)[ew][i--]->searchValue & CANWALKUP)) /* Otherwise */
                i = -1;
        if (j <= FIELDHEIGHT)
            if (((*playfield)[ew][j]->searchValue & CANWALKLEFT) || /* Down and L? */
                    (runThruHole && ((*playfield)[ew-1][j]->whatIam() == HOLE)))
                return DOWN;					/* Go down. */
            else
                if (!((*playfield)[ew][j++]->searchValue & CANWALKDOWN)) /* Otherwise */
                    j = FIELDHEIGHT + 1;
        }
    return STAND; /* default */
}
               
Direction KGrEnemy::searchrightway (int ew,int eh)
{
    int i, j;
    i = j = eh;
    if (((*playfield)[ew][eh]->searchValue & CANWALKRIGHT) ||
               (runThruHole && ((*playfield)[ew+1][eh]->whatIam() == HOLE)))
        return RIGHT;
    else while ((i >= 0) || (j <= FIELDHEIGHT)) {
        if (i >= 0)
            if (((*playfield)[ew][i]->searchValue & CANWALKRIGHT) ||
                   (runThruHole && ((*playfield)[ew+1][i]->whatIam() == HOLE)))
                return UP;
            else
                if (!((*playfield)[ew][i--]->searchValue & CANWALKUP))
                    i = -1;
        if (j <= FIELDHEIGHT)
            if (((*playfield)[ew][j]->searchValue & CANWALKRIGHT) ||
                (runThruHole && ((*playfield)[ew+1][j]->whatIam() == HOLE)))
                return DOWN;
            else
                if (!((*playfield)[ew][j++]->searchValue & CANWALKDOWN))
                    j = FIELDHEIGHT + 1;
    }
    return STAND;
}
            
////////////////////////////////////////////
// Methods for low-level search strategy. //
////////////////////////////////////////////

Direction KGrEnemy::lowSearchUp (int ew, int eh, int hh)
{
    int i, ilen, ipos, j, jlen, jpos, deltah, rungs;

    deltah = eh - hh;			// Get distance up to hero's level.

    // Search for the best ladder right here or on the left.
    i = ew; ilen = 0; ipos = -1;
    while (i >= 1) {
        rungs = distanceUp (i, eh, deltah);
        if (rungs > ilen) {
            ilen = rungs;		// This the best yet.
            ipos = i;
        }
        if (searchOK (-1, i, eh))
            i--;			// Look further to the left.
        else
            i = -1;			// Cannot go any further to the left.
    }

    // Search for the best ladder on the right.
    j = ew; jlen = 0; jpos = -1;
    while (j < FIELDWIDTH) {
        if (searchOK (+1, j, eh)) {
            j++;			// Look further to the right.
            rungs = distanceUp (j, eh, deltah);
            if (rungs > jlen) {
                jlen = rungs;		// This the best yet.
                jpos = j;
            }
        }
        else
            j = FIELDWIDTH+1;		// Cannot go any further to the right.
    }

    if ((ilen == 0) && (jlen == 0))	// No ladder found.
        return STAND;

    // Choose a ladder to go to.
    if (ilen != jlen) {			// If the ladders are not the same
                                        // length, choose the longer one.
        if (ilen > jlen) {
            if (ipos == ew)		// If already on the best ladder, go up.
                return UP;
            else
                return LEFT;
        }
        else
            return RIGHT;
    }
    else {				// Both ladders are the same length.

        if (ipos == ew)			// If already on the best ladder, go up.
            return UP;
        else if (ilen == deltah) {	// If both reach the hero's level,
            if ((ew - ipos) <= (jpos - ew)) // choose the closest.
                return LEFT;
            else
                return RIGHT;
        }
        else return LEFT;		// Else choose the left ladder.
    }
}

Direction KGrEnemy::lowSearchDown (int ew, int eh, int hh)
{
    int i, ilen, ipos, j, jlen, jpos, deltah, rungs, path;

    deltah = hh - eh;			// Get distance down to hero's level.

    // Search for the best way down, right here or on the left.
    ilen = 0; ipos = -1;
    i = (willNotFall (ew, eh)) ? ew : -1;
    rungs = distanceDown (ew, eh, deltah);
    if (rungs > 0) {
        ilen = rungs; ipos = ew;
    }

    while (i >= 1) {
        rungs = distanceDown (i - 1, eh, deltah);
        if (((rungs > 0) && (ilen == 0)) ||
            ((deltah > 0) && (rungs > ilen)) ||
            ((deltah <= 0) && (rungs < ilen) && (rungs != 0))) {
            ilen = rungs;		// This the best way yet.
            ipos = i - 1;
        }
        if (searchOK (-1, i, eh))
            i--;			// Look further to the left.
        else
            i = -1;			// Cannot go any further to the left.
    }

    // Search for the best way down, on the right.
    j = ew; jlen = 0; jpos = -1;
    while (j < FIELDWIDTH) {
        rungs = distanceDown (j + 1, eh, deltah);
        if (((rungs > 0) && (jlen == 0)) ||
            ((deltah > 0) && (rungs > jlen)) ||
            ((deltah <= 0) && (rungs < jlen) && (rungs != 0))) {
            jlen = rungs;		// This the best way yet.
            jpos = j + 1;
        }
        if (searchOK (+1, j, eh)) {
            j++;			// Look further to the right.
        }
        else
            j = FIELDWIDTH+1;		// Cannot go any further to the right.
    }

    if ((ilen == 0) && (jlen == 0))	// Found no way down.
        return STAND;

    // Choose a way down to follow.
    if (ilen == 0)
        path = jpos;
    else if (jlen == 0)
        path = ipos;
    else if (ilen != jlen) {		// If the ways down are not same length,
                                        // choose closest to hero's level.
        if (deltah > 0) {
            if (jlen > ilen)
                path = jpos;
            else
                path = ipos;
        }
        else {
            if (jlen > ilen)
                path = ipos;
            else
                path = jpos;
        }
    }
    else {				// Both ways down are the same length.
        if ((deltah > 0) &&		// If both reach the hero's level,
            (ilen == deltah)) {		// choose the closest.
            if ((ew - ipos) <= (jpos - ew))
                path = ipos;
            else
                path = jpos;
        }
        else
            path = ipos;		// Else, go left or down.
    }

    if (path == ew)
        return DOWN;
    else if (path < ew)
        return LEFT;
    else
        return RIGHT;
}

Direction KGrEnemy::lowGetHero (int ew, int eh, int hw)
{
    int i, inc, returnValue;

    inc = (ew > hw) ? -1 : +1;
    i = ew;
    while (i != hw) {
        returnValue = canWalkLR (inc, i, eh);
        if (returnValue > 0)
            i = i + inc;		// Can run further towards the hero.
        else if (returnValue < 0)
            break;			// Will run into a wall regardless.
        else
            return STAND;		// Won't run over a hole.
    }

    if (i < ew)		return LEFT;
    else if (i > ew)	return RIGHT;
    else		return STAND;
}

int KGrEnemy::distanceUp (int x, int y, int deltah)
{
    int rungs = 0;

    // If there is a ladder at (x.y), return its length, else return zero.
    while ((*playfield)[x][y - rungs]->whatIam() == LADDER) {
        rungs++;
        if (rungs >= deltah)		// To hero's level is enough.
            break;
    }
    return rungs;
}

int KGrEnemy::distanceDown (int x, int y, int deltah)
{
    // When deltah > 0, we want an exit sideways at the hero's level or above.
    // When deltah <= 0, we can go down any distance (as a last resort).

    int rungs = -1;
    int exitRung = 0;
    bool canGoThru = true;
    char objType;

    // If there is a way down at (x,y), return its length, else return zero.
    // Because rungs == -1, we first check that level y is not blocked here.
    while (canGoThru) {
        objType = (*playfield)[x][y + rungs + 1]->whatIam();
        switch (objType) {
        case BRICK:
        case BETON:
        case HOLE:			// Enemy cannot go DOWN through a hole.
        case USEDHOLE:
            if ((deltah > 0) && (rungs <= deltah))
                exitRung = rungs;
            if ((objType == HOLE) && (rungs < 0))
                rungs = 0;		// Enemy can go SIDEWAYS through a hole.
            else
                canGoThru = false;	// Cannot go through here.
            break;
        case LADDER:
        case POLE:			// Can go through or stop.
            rungs++;			// Add to path length.
            if ((deltah > 0) && (rungs >= 0)) {
                // If at or above hero's level, check for an exit from ladder.
                if ((rungs - 1) <= deltah) {
                    // Maybe can stand on top of ladder and exit L or R.
                    if ((objType == LADDER) && (searchOK (-1, x, y+rungs-1) ||
                                                searchOK (+1, x, y+rungs-1)))
                        exitRung = rungs - 1;
                    // Maybe can exit L or R from ladder body or pole.
                    if ((rungs <= deltah) && (searchOK (-1, x, y+rungs) ||
                                              searchOK (+1, x, y+rungs)))
                        exitRung = rungs;
                }
                else
                    canGoThru = false;	// Should stop at hero's level.
            }
            break;
        default:
            rungs++;			// Can go through.  Add to path length.
            break;
        }
    }
    if (rungs == 1) {
        QListIterator<KGrEnemy *> i (*enemies);
        while (i.hasNext()) {
            KGrEnemy * enemy = i.next();
            if ((x*16==enemy->getx()) && (y*16+16==enemy->gety()))
                rungs = 0;		// Pit is blocked.  Find another way.
        }
    }
    if (rungs <= 0)
        return 0;			// There is no way down.
    else if (deltah > 0)
        return exitRung;		// We want to take an exit, if any.
    else
        return rungs;			// We can go down all the way.
}

bool KGrEnemy::searchOK (int direction, int x, int y)
{
    // Check whether it is OK to search left or right.
    if (canWalkLR (direction, x, y) > 0) {
        // Can go into that cell, but check for a fall.
        if (willNotFall (x+direction, y))
            return true;
    }
    return false;			// Cannot go into and through that cell.
}

int KGrEnemy::canWalkLR (int direction, int x, int y)
{
    if (willNotFall (x, y)) {
        switch ((*playfield)[x+direction][y]->whatIam()) {
        case BETON:
        case BRICK:
        case USEDHOLE:
            return -1; break;		// Will be halted in current cell.
        default:
            // NB. FREE, LADDER, HLADDER, NUGGET and POLE are OK of course,
            //     but enemies can also walk left/right through a HOLE and
            //     THINK they can walk left/right through a FBRICK.

            return +1; break;		// Can walk into next cell.
        }
    }
    else
        return 0;			// Will fall before getting there.
}

bool KGrEnemy::willNotFall (int x, int y)
{
    int c, cmax;
    KGrEnemy *enemy;

    // Check the ceiling.
    switch ((*playfield)[x][y]->whatIam()) {
    case LADDER:
    case POLE:
        return true; break;		// OK, can hang on ladder or pole.
    default:
        break;
    }

    // Check the floor.
    switch ((*playfield)[x][y+1]->whatIam()) {

    // Cases where the enemy knows he will fall.
    case FREE:
    case HLADDER:
    case FBRICK:

    // N.B. The enemy THINKS he can run over a NUGGET, a buried POLE or a HOLE.
    // The last of these cases allows the hero to trap the enemy, of course.

    // Note that there are several Traditional levels that require an enemy to
    // be trapped permanently in a pit containing a nugget, as he runs towards
    // you.  It is also possible to use a buried POLE in the same way.

        cmax = enemies->count();
        for (c = 0; c < cmax; c++) {
            enemy = enemies->at (c);
            if ((enemy->getx()==16*x) && (enemy->gety()==16*(y+1)))
                return true;		// Standing on a friend.
        }
        return false; break;		// Will fall: there is no floor.

    default:
        return true; break;		// OK, will not fall.
    }
}

void KGrEnemy::setEnemyList (QList<KGrEnemy *> * e)
{
    enemies = e;
}

bool KGrEnemy::standOnEnemy()
{
    int c = 0;
    int range = enemies->count();
    if (range > 1) {
        for (KGrEnemy * enemy = enemies->at (c); c < range;) {
            enemy = enemies->at (c++);
            // Test if enemy's foot is at or just below enemy's head (tolerance
            // of 4 pixels) and the two figures overlap in the X direction.
            if ((((absy + 16) == enemy->gety()) ||
                ((absy + 12) == enemy->gety())) &&
               (((absx - 16) <  enemy->getx()) &&
                ((absx + 16) >  enemy->getx()))) {
               return true;
            }
        }
    }
    return false;
}

bool KGrEnemy::bumpingFriend()
{
//  Enemies that are falling are treated as being invisible (i.e. a walking
//  enemy will walk through them or they will stop on that enemy's head).
//
//  If two enemies are moving in opposite directions, they are allowed to come
//  within two cell widths of each other (8 steps).  Then one must stop before
//  entering the next cell and let the other one go into it.  If both are about
//  to enter a new cell, the one on the right or above gives way to the one on
//  the left or below (implemented by letting the latter approach to 7 steps
//  apart before detecting an impending collision, by which time the first
//  enemy should have stopped and given way).
//
//  In all other cases, enemies are allowed to approach to 4 steps apart (i.e.
//  bumping a friend), before being forced to stop and wait.  If they somehow
//  get closer than 4 steps (i.e. overlapping), the lower ID enemy is allowed
//  to move out while the higher ID enemy waits.  This can happen if one enemy
//  falls into another or is reborn where another enemy is located.

    int c, cmax;
    KGrEnemy *enemy;
    int dx, dy;

    cmax = enemies->count();
    for (c = 0; c < cmax; c++) {
        enemy = enemies->at (c);
        if ((enemy->enemyId != enemyId) && (enemy->status != FALLING)) {
            dx = enemy->getx() - absx;
            dy = enemy->gety() - absy;
            switch (direction) {
            case LEFT:
                if ((dx >= -32) && (dx < 16) && (dy > -16) && (dy < 16)) {
                    if ((enemy->direction == RIGHT) &&
                        (enemy->status == WALKING)  && (absx%16==0)) {
                        return true;
                    }
                    else if (dx >= -16) {
                        if ((dx > -16) && (enemyId < enemy->enemyId))
                            return false;
                        else
                            return true;	// Wait for the one in front.
                    }
                }
                break;
            case RIGHT:
                if ((dx > -16) && (dx < 32) && (dy > -16) && (dy < 16)) {
                    if ((enemy->direction == LEFT) &&
                        (enemy->status == WALKING) && (absx%16==0)) {
                        return true;
                    }
                    else if (dx <= 16) {
                        if ((dx < 16) && (enemyId < enemy->enemyId))
                            return false;
                        else
                            return true;	// Wait for the one in front.
                    }
                }
                break;
            case UP:
                if ((dy >= -32) && (dy < 16) && (dx > -16) && (dx < 16)) {
                    if ((enemy->direction == DOWN) &&
                        (enemy->status == WALKING) && (absy%16==0)) {
                        return true;
                    }
                    else if (dy >= -16) {
                        if ((dy > -16) && (enemyId < enemy->enemyId))
                            return false;
                        else
                            return true;	// Wait for the one above.
                    }
                }
                break;
            case DOWN:
                if ((dy > -16) && (dy < 32) && (dx > -16) && (dx < 16)) {
                    if ((enemy->direction == UP) &&
                        (enemy->status == WALKING) && (absy%16==0)) {
                        return true;
                    }
                    else if (dy <= 16) {
                        if ((dy < 16) && (enemyId < enemy->enemyId))
                            return false;
                        else
                            return true;	// Wait for the one below.
                    }
                }
                break;
            default:
                break;
            }
        }
    }
    return false;
}

KGrEnemy::~KGrEnemy()
{
    delete captiveTimer;
    delete walkTimer;
    delete fallTimer;
}

#include "kgrfigure.moc"
