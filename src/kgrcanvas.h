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

// $Log$
// $OldLog: kgoldrunnerwidget.h,v $
// Revision 1.4  2003/07/08 13:24:34  ianw
// Converted to KDE 3.1.1 and Qt 3.1.1
//

#ifndef KGRCANVAS_H
#define KGRCANVAS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qcursor.h>
#include <qcanvas.h>
#include <qlabel.h>
#include <qimage.h>
#include <qpainter.h>

class KGrCanvas : public QCanvasView
{
	Q_OBJECT
public:
	KGrCanvas (QWidget * parent = 0, const char *name = 0);
	virtual ~KGrCanvas();

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

	QCanvas * field;
	QCanvasView * fieldView;
	int scaleStep;			// Current scale-factor of canvas.
	int baseScale;			// Starting scale-factor of canvas.
	int baseFontSize;

	int cw, bw, lw, mw;		// Dimensions of the canvas border.
	QColor borderColor, textColor;	// Border colours.
	QLabel * title;

	int freebg, nuggetbg, polebg, ladderbg, hladderbg;
	int edherobg, edenemybg, betonbg, brickbg, fbrickbg;
	int bgw, bgh, bgd;
	QPixmap bgPix;

	QCanvasPixmapArray * heroArray;
	QCanvasPixmapArray * enemyArray;
	int goldEnemy;

	QCanvasSprite * heroSprite;
#ifdef QT3
	QPtrList<QCanvasSprite> * enemySprites;
#else
	QList<QCanvasSprite> * enemySprites;
#endif

	QCanvasEllipse * endItem1;

	void initView();
	void makeBorder();
	void makeTitle();
	QColor colour;
	void drawRectangle (int, int, int, int, int);
};

#endif // KGRCANVAS_H
