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

#include <config.h>
#include <kgamecanvas.h>

#include "kgrplayfield.h"
#include "kgrsprite.h"

#include <qcursor.h>
#include <QLabel>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>
#include <QList>


class KGrCanvas : public KGameCanvasWidget
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
	void setTitle (const QString&);

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

protected:
	virtual void mousePressEvent ( QMouseEvent * mouseEvent );
	virtual void mouseReleaseEvent ( QMouseEvent * mouseEvent );

private:
	QCursor * m;

	KGrPlayField * playfield;
	int scaleStep;			// Current scale-factor of canvas.
	int baseScale;			// Starting scale-factor of canvas.
	int baseFontSize;

	int border;			// Number of tiles allowed for border.
	int cw, bw, lw, mw;		// Dimensions (in pixels) of the border.
	QColor borderColor, textColor;	// Border colours.
	QLabel * title;			// Title and top part of border.

	int freebg, nuggetbg, polebg, ladderbg, hladderbg;
	int edherobg, edenemybg, betonbg, brickbg, fbrickbg;
	int bgw, bgh, bgd;
	QPixmap bgPix;

	int goldEnemy;

	KGrSprite * heroSprite;
	QList<KGrSprite *> * enemySprites;
	QList<KGameCanvasRectangle *> borderRectangles;

	void initView();
	void makeTiles();
	void makeBorder();
	void makeTitle();
	QColor colour;
	KGameCanvasRectangle * drawRectangle (int, int, int, int, int);
	void changeColours (const char * colours []);
	void recolourObject (const char * object [], const char * colours []);
};

#endif // KGRCANVAS_H
