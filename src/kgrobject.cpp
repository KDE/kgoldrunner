/***************************************************************************
                        kgrobject.cpp  -  description
                             -------------------
    begin                : Wed Jan 23 2002
    copyright            : (C) 2002 by Marco Krüger and Ian Wadham
    email                : See menu "Help, About KGoldrunner"
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgrconsts.h"
#include "kgrcanvas.h"

#include "kgrobject.h"

#include <stdio.h>

KGrObject::KGrObject (char objType)
{
    iamA = objType;
    searchValue = 0;
    blocker = FALSE;
    if ((objType == BRICK) || (objType == BETON) || (objType == FBRICK)) {
	blocker = TRUE;
    }
    xpos = 0;
    ypos = 0;
    objectView = NULL;
}

bool KGrObject::frozen = FALSE;	// Initialise game as running, not halted.
bool KGrObject::bugFixed = FALSE;// Initialise game with dynamic bug-fix OFF.
bool KGrObject::logging = FALSE;// Initialise game with log printing OFF.

char KGrObject::whatIam ()
{
  return iamA;
}

void KGrObject::showState (int i, int j)
{
  printf("(%02d,%02d) - Object [%c] search %d", i, j, iamA, searchValue);
  if (blocker) printf(" blocker");
  printf("\n");
}

KGrObject :: ~KGrObject ()
{
}

KGrEditable::KGrEditable (char editType) : KGrObject (editType)
{
}

void KGrEditable::setType (char editType)
{
    iamA = editType;
}

KGrEditable::~KGrEditable ()
{
}

KGrFree::KGrFree (char objType, int i, int j, KGrCanvas * view)
				: KGrObject (objType)
{
  xpos		= i;
  ypos		= j;
  objectView	= view;
  theRealMe	= FREE;		// Remember what we are, even "iamA == NUGGET".
}

void KGrFree::setNugget(bool nug)
{
    // This code must work over a hidden ladder as well as a free cell.
    if (! nug) {
	iamA = theRealMe;
	objectView->paintCell (xpos, ypos, FREE);
    }
    else {
	iamA = NUGGET;
	objectView->paintCell (xpos, ypos, NUGGET);
    }
}

KGrFree :: ~KGrFree ()
{
}

/* +++++++++++++++ BRICK ++++++++++++++++ */

KGrBrick::KGrBrick (char objType, int i, int j, KGrCanvas * view)
				: KGrObject (objType)
{
  xpos		= i;
  ypos		= j;
  objectView	= view;
  dig_counter = 0;
  holeFrozen = FALSE;
  iamA = BRICK;
  timer = new QTimer (this);
  connect (timer, SIGNAL (timeout ()), SLOT (timeDone ()));
}

void KGrBrick::dig (void)
{
  dig_counter = 1;
  hole_counter = HOLETIME;
  iamA = HOLE;
  objectView->paintCell (xpos, ypos, BRICK, dig_counter);
  objectView->updateCanvas();
  timer->start ((DIGDELAY * NSPEED) / speed, TRUE);
}

void KGrBrick::doStep() {
    if (holeFrozen) {
	holeFrozen = FALSE;
	timeDone();
    }
}

void KGrBrick::showState (int i, int j)
{
    printf ("(%02d,%02d) - Brick  [%c] search %d dig-counter %d",
	i, j, iamA, searchValue, dig_counter);
    if (blocker)
	printf (" blocker");
    printf ("\n");
}

void KGrBrick::timeDone ()
{
    if (KGrObject::frozen) {holeFrozen = TRUE; return;}

    // When the hole is complete, we need a longer delay.
    if (dig_counter == 5) {
	hole_counter--;
	if (hole_counter > 0) {
	    timer->start ((DIGDELAY * NSPEED) / speed, TRUE);
	    return;
	}
    }
    if (dig_counter < 9) {
	dig_counter++;
	timer->start ((DIGDELAY * NSPEED) / speed, TRUE);
	if (dig_counter >= 8)
	    iamA = BRICK;
    }
    else
	dig_counter = 0;

    // Brick pix:- 0 normal, 1-4 crumbling, 5 hole complete, 6-9 re-growing.
    objectView->paintCell (xpos, ypos, BRICK, dig_counter);
    objectView->updateCanvas();
}

void KGrBrick::useHole() {
    if (iamA == HOLE)
	iamA = USEDHOLE;
}

void KGrBrick::unUseHole() {
    if (iamA == USEDHOLE)
	iamA = HOLE;
}

KGrBrick :: ~KGrBrick ()
{
    delete timer;
}

KGrHladder::KGrHladder (char objType, int i, int j, KGrCanvas * view)
			    : KGrFree (objType, i, j, view)
  // Must inherit "setNugget()" from "KGrFree".
{
  theRealMe = HLADDER;		// But remember we are a hidden ladder ...
}

void KGrHladder::showLadder()
{
    iamA = LADDER;
    objectView->paintCell (xpos, ypos, LADDER);
}

KGrHladder :: ~KGrHladder ()
{
}

#include "kgrobject.moc"
