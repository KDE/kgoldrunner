/***************************************************************************
                         kgrobject.h  -  description
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

#ifndef KGROBJECT_H
#define KGROBJECT_H

// Obsolete - #include <iostream.h>
#include <iostream>

#include <qtimer.h>
#include <stdlib.h> // for random

class KGrCanvas;

class KGrObject : public QObject
{
  Q_OBJECT
public:
  KGrObject (char objType);
  virtual ~KGrObject();

  // STATIC GLOBAL FLAGS.
  static bool frozen;		// Game play halted (use ESCAPE key).
  static bool bugFixed;		// Dynamic bug fix turned on (key B, if halted).
  static bool logging;		// Log printing turned on.

  char whatIam();
  int searchValue;
  bool blocker; // Beton or Brick -> TRUE
  void showState (int, int);

protected:
  KGrCanvas * objectView;
  int xpos;
  int ypos;
  char iamA;
};

class KGrEditable : public KGrObject
{
  Q_OBJECT
public:
  KGrEditable (char editType);
  virtual ~KGrEditable ();
  void setType (char);
};

class KGrFree : public KGrObject
{ Q_OBJECT
public:
  KGrFree (char objType, int i, int j, KGrCanvas * view);
  virtual ~KGrFree();
  void setNugget(bool);

protected:
  char theRealMe;	// Set to FREE or HLADDER, even when "iamA == NUGGET".
};

class KGrBrick : public KGrObject
{
  Q_OBJECT
public:
  KGrBrick (char objType, int i, int j, KGrCanvas * view);
  virtual ~KGrBrick();
  void dig(void);
  void useHole();
  void unUseHole();
  static int speed;	// Digging & repair speed (copy of KGrFigure::speed).
  static int HOLETIME;	// Number of timing cycles for a hole to remain open.
  void doStep();
  void showState (int, int);

protected slots:
  void timeDone(void);

private:
  int dig_counter;
  int hole_counter;
  bool holeFrozen;
  QTimer *timer;
};

class KGrHladder : public KGrFree
{
  Q_OBJECT
public:
  // BUG FIX - Ian W., 21/6/01 - must inherit "setNugget()" from "KGrFree".
  KGrHladder (char objType, int i, int j, KGrCanvas * view);
  virtual ~KGrHladder();
  void showLadder();
};

#endif // KGROBJECT_H
