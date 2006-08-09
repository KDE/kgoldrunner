/***************************************************************************
                         kgrcanvas.h  -  description
                             -------------------
    begin                : Wed Jan 23 2002
    Copyright 2002 Marco Krger
    Copyright 2002 Ian Wadham <ianw@netspace.net.au>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGRCANVAS_H
#define KGRCANVAS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kgrscene.h"
#include "kgrsprite.h"

#include <qcursor.h>
#include <QLabel>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>
#include <QList>
#include <QGraphicsView>
#include <QGraphicsRectItem>

class KGrCanvas : public QGraphicsView
{
	Q_OBJECT
public:
	KGrCanvas (QWidget * parent = 0, const char *name = 0);
	virtual ~KGrCanvas();

	void changeLandscape (const QString & name);

	QPoint getMousePos ();
	void setMousePos (int, int);

	bool changeSize (int);
	void setBaseScale ();

	//void updateCanvas ();
	void paintCell (int, int, char, int offset = 0);
	void setTitle (QString);

	void makeHeroSprite (int, int, int);
	void setHeroVisible (bool);
	void moveHero (int, int, int);

	void makeEnemySprite (int, int, int);
	void moveEnemy (int, int, int, int, int);
	void deleteEnemySprites();

	QPixmap getPixmap (char type);

signals:
	void mouseClick (int);
	void mouseLetGo (int);

public slots:
	void contentsMouseClick (int);
	void contentsMouseLetGo (int);

private:
	QCursor * m;

	KGrScene * field;
	//Q3CanvasView * fieldView;
	int scaleStep;			// Current scale-factor of canvas.
	int baseScale;			// Starting scale-factor of canvas.
	int baseFontSize;

	int border;			// Number of tiles allowed for border.
	int cw, bw, lw, mw;		// Dimensions (in pixels) of the border.
	QColor borderColor, textColor;	// Border colours.
	QLabel * title;			// Title and top part of border.
	QGraphicsRectItem * borderT;	// Bottom part of border.
	QGraphicsRectItem * borderB;	// Bottom part of border.
	QGraphicsRectItem * borderL;	// Left-hand part of border.
	QGraphicsRectItem * borderR;	// Right-hand part of border.

	int freebg, nuggetbg, polebg, ladderbg, hladderbg;
	int edherobg, edenemybg, betonbg, brickbg, fbrickbg;
	int bgw, bgh, bgd;
	QPixmap bgPix;

	int goldEnemy;

	KGrSprite * heroSprite;
	QList<KGrSprite *> * enemySprites;

	void initView();
	void makeTiles();
	void makeBorder();
	void makeTitle();
	QColor colour;
	QGraphicsRectItem * drawRectangle (int, int, int, int, int);
	void changeColours (const char * colours []);
	void recolourObject (const char * object [], const char * colours []);
};

#endif // KGRCANVAS_H
