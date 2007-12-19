/***************************************************************************
 *                       kgrfigure.h  -  description                       *
 *                           -------------------                           *
    Copyright 2003 Marco Krüger <grisuji@gmx.de>
    Copyright 2003 Ian Wadham <ianw2@optusnet.com.au>
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

#ifndef KGRFIGURE_H
#define KGRFIGURE_H

// Obsolete - #include <iostream.h>
#include <iostream>

#include <QList>
#include <QTimer>
#include <stdlib.h>			// For random-number generator.

#include "kgrconsts.h"

class KGrCanvas;
class KGrObject;
class KGrEnemy;

class KGrFigure : public QObject
{
  Q_OBJECT
public:
  KGrFigure (int, int);
  virtual ~KGrFigure();

  // STATIC GLOBAL FLAGS.
  static bool variableTiming;		// More enemies imply less speed.
  static bool alwaysCollectNugget;	// Enemies always collect nuggets.
  static bool runThruHole;		// Enemy can run L/R through dug hole.
  static bool reappearAtTop;		// Enemies are reborn at top of screen.
  static SearchStrategy searchStrategy;	// Low, medium or high difficulty.

  static Timing fixedTiming;		// Original set of 6 KGr timing values.

  static Timing varTiming [6];		// Optional 6 sets of timing values,
					// dependent on number of enemies.
  int getx();
  int gety();
  Status getStatus();

  int getnuggets();
  void setNuggets(int n);
  void setPlayfield(KGrObject *(*p)[30][22]);
  void showFigure(); //zeigt Figur
  virtual void init(int,int);
  void eraseOldFigure();

protected:
  // STATIC GLOBAL VARIABLES.
  static int herox;
  static int heroy;

  static int speed;			// Adjustable game-speed factor.

  int x, y;
  int absx, absy;
  int relx, rely;			// Relative movement, in pixels.
  int mem_x,mem_y,mem_relx,mem_rely;
  int walkCounter;
  int nuggets;
  int actualPixmap;			// Array index of the pixmap to draw.
  bool alternateStepGraphics;
  QTimer *walkTimer;
  QTimer *fallTimer;

  KGrObject *(*playfield)[30][22];
  Status status;
  Direction direction;
  bool canWalkRight();
  bool canWalkLeft();
  virtual bool canWalkUp();
  bool canWalkDown();
  bool canStand();
  bool hangAtPole();
  virtual bool standOnEnemy()=0;
  void walkUp(int);
  void walkDown(int, int);
  void walkRight(int, int);
  void walkLeft(int, int);
  void initFall(int, int);

  bool walkFrozen;
  bool fallFrozen;
};

class KGrHero : public KGrFigure
{
  Q_OBJECT
public:
  KGrHero(KGrCanvas *, int , int);
  virtual ~KGrHero();
  bool started;
  void showFigure();
  void dig();
  void digLeft();
  void digRight();
  void startWalk();
  void setEnemyList(QList<KGrEnemy *> *);
  void init(int,int);
  void setKey(Direction);
  void setDirection(int, int);
  void start();
  void loseNugget();
  static int WALKDELAY;
  static int FALLDELAY;
  void setSpeed(int);
  void doStep();
  void showState (char);

private:
  QList<KGrEnemy *> *enemies;
  KGrCanvas * heroView;
  bool standOnEnemy();
  bool isInEnemy();
  bool isInside(int, int);

  Direction nextDir;
  void collectNugget();

  bool mouseMode;
  bool stopped;
  int mousex;
  int mousey;
  void setNextDir();

public slots:
  void walkTimeDone();
  void fallTimeDone();

signals:
  void gotNugget(int);
  void haveAllNuggets();
  void leaveLevel();
  void caughtHero();
};

class KGrEnemy : public KGrFigure
{
  Q_OBJECT
public:
  KGrEnemy (KGrCanvas *, int , int);
  virtual ~KGrEnemy();
  void showFigure();
  void startSearching();
  void setEnemyList(QList<KGrEnemy *> *);
  virtual void init(int,int);
  static int WALKDELAY;
  static int FALLDELAY;
  static int CAPTIVEDELAY;
  int enemyId;
  void doStep();
  void showState (char);

  static void makeReappearanceSequence();

private:
  KGrCanvas * enemyView;
  int birthX, birthY;
  int searchStatus;
  int captiveCounter;
  QTimer *captiveTimer;
  bool canWalkUp();
  QList<KGrEnemy *> *enemies;
  bool standOnEnemy();
  bool bumpingFriend();

  void startWalk();

  static int reappearIndex;
  static int reappearPos [FIELDWIDTH];
  void dieAndReappear();
  Direction searchbestway(int,int,int,int);
  Direction searchdownway(int,int);
  Direction searchupway(int,int);
  Direction searchleftway(int,int);
  Direction searchrightway(int,int);

  Direction lowSearchUp(int,int,int);
  Direction lowSearchDown(int,int,int);
  Direction lowGetHero(int,int,int);

  int  distanceUp (int, int, int);
  int  distanceDown (int, int, int);
  bool searchOK (int, int, int);
  int  canWalkLR (int, int, int);
  bool willNotFall (int, int);

  void collectNugget();
  void dropNugget();

  bool captiveFrozen;

public slots:
  void walkTimeDone();
  void fallTimeDone();
  void captiveTimeDone();

signals:
  void lostNugget();
  void trapped(int);
  void killed(int);
};

#endif // KGRFIGURE_H
