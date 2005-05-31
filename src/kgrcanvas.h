/***************************************************************************
                         kgrcanvas.h  -  description
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

#ifndef KGRCANVAS_H
#define KGRCANVAS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qcursor.h>
#include <q3canvas.h>
#include <qlabel.h>
#include <qimage.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QPixmap>
#include <QMouseEvent>
#include <Q3PtrList>

class KGrCanvas : public Q3CanvasView
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

	void updateCanvas ();
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

protected:
	void contentsMousePressEvent (QMouseEvent *);
	void contentsMouseReleaseEvent (QMouseEvent *);

private:
	QCursor * m;

	Q3Canvas * field;
	Q3CanvasView * fieldView;
	int scaleStep;			// Current scale-factor of canvas.
	int baseScale;			// Starting scale-factor of canvas.
	int baseFontSize;

	int border;			// Number of tiles allowed for border.
	int cw, bw, lw, mw;		// Dimensions (in pixels) of the border.
	QColor borderColor, textColor;	// Border colours.
	QLabel * title;			// Title and top part of border.
	Q3CanvasRectangle * borderB;	// Bottom part of border.
	Q3CanvasRectangle * borderL;	// Left-hand part of border.
	Q3CanvasRectangle * borderR;	// Right-hand part of border.

	int freebg, nuggetbg, polebg, ladderbg, hladderbg;
	int edherobg, edenemybg, betonbg, brickbg, fbrickbg;
	int bgw, bgh, bgd;
	QPixmap bgPix;

	Q3CanvasPixmapArray * heroArray;
	Q3CanvasPixmapArray * enemyArray;
	int goldEnemy;

	Q3CanvasSprite * heroSprite;
#ifdef QT3
	Q3PtrList<Q3CanvasSprite> * enemySprites;
#else
	QList<Q3CanvasSprite> * enemySprites;
#endif

	void initView();
	void makeTiles();
	void makeBorder();
	void makeTitle();
	QColor colour;
	Q3CanvasRectangle * drawRectangle (int, int, int, int, int);
	void changeColours (const char * colours []);
	void recolourObject (const char * object [], const char * colours []);
};

#endif // KGRCANVAS_H
