/***************************************************************************
 *                       kgrconsts.h  -  description                       *
 *                           -------------------                           *
 *   Copyright (C) 2003 by Ian Wadham and Marco Krüger                     *
 *   email       : See menu "Help, About KGoldrunner"                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef KGRCONSTS_H
#define KGRCONSTS_H

enum Owner {SYSTEM, USER};

const char FREE    = ' ';
const char ENEMY   = 'E';
const char HERO    = 'R';
const char BETON   = 'X';
const char BRICK   = 'M';
const char FBRICK  = 'F';
const char HLADDER = 'Z';
const char LADDER  = 'H';
const char NUGGET  = 'N';
const char POLE    = 'T';
const char HOLE    = 'O';
const char USEDHOLE= 'U';

const char CANWALKLEFT  = 0x1;
const char CANWALKRIGHT = 0x2;
const char CANWALKUP    = 0x4;
const char CANWALKDOWN  = 0x8;
const char VISITED      = 0x10;

const char FIELDWIDTH   = 28;
const char FIELDHEIGHT  = 20;

const char VERTIKAL     = 0;
const char HORIZONTAL   = 1;

/* Action times ... */
#define	NSPEED		12
#define	MAXSPEED	NSPEED * 2
#define	MINSPEED	NSPEED / 4

#define	BEGINSPEED	NSPEED / 2
#define	NOVICESPEED	(3 * NSPEED) / 4
#define	CHAMPSPEED	(3 * NSPEED) / 2

typedef struct {
    int hwalk;
    int hfall;
    int ewalk;
    int efall;
    int ecaptive;
    int hole;
} Timing;

const int DIGDELAY = 200;

const int STEP = 4;

const double DROPNUGGETDELAY = 70.0;	// Enemy holds gold for avg. 12.5 cells.

enum Position		{RIGHTWALK1,RIGHTWALK2,RIGHTWALK3,RIGHTWALK4,
			 LEFTWALK1,LEFTWALK2,LEFTWALK3,LEFTWALK4,
			 RIGHTCLIMB1,RIGHTCLIMB2,RIGHTCLIMB3,RIGHTCLIMB4,
			 LEFTCLIMB1,LEFTCLIMB2,LEFTCLIMB3,LEFTCLIMB4,
			 CLIMB1,CLIMB2,
			 FALL1,FALL2};
enum Status		{STANDING,FALLING,WALKING,CLIMBING,CAPTIVE};
enum Direction		{RIGHT,LEFT,UP,DOWN,STAND};
enum SearchStrategy	{LOW,MEDIUM,HIGH};

// Keyboard action codes
enum KBAction		{KB_UP, KB_DOWN, KB_LEFT, KB_RIGHT,
			 KB_DIGLEFT, KB_DIGRIGHT, KB_STOP};

// Action codes when selecting a level or game for play or editing.
enum SelectAction	{SL_START, SL_ANY, SL_CREATE, SL_UPDATE, SL_SAVE,
			 SL_MOVE, SL_DELETE, SL_CR_GAME, SL_UPD_GAME};

#endif // KGRCONSTS_H
