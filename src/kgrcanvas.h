/***************************************************************************
                         kgrcanvas.h  -  description
                             -------------------
    begin                : Wed Jan 23 2002
    Copyright 2002 Marco Krger
    Copyright 2002 Ian Wadham <ianw2@optusnet.com.au>
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
#include <QTime> // IDW

#include <KSvgRenderer>

class KGrCanvas : public KGameCanvasWidget
{
	Q_OBJECT
public:
	KGrCanvas (QWidget * parent, const double scale,
				const QString & systemDataDir);
	virtual ~KGrCanvas();

	QPoint getMousePos ();
	void setMousePos (int, int);

	void setBaseScale ();

	void paintCell (int, int, char, int offset = 0);
	void setTitle (const QString&);

	void makeHeroSprite (int, int, int);
	void setHeroVisible (bool);
	void moveHero (int, int, int);

	void makeEnemySprite (int, int, int);
	void moveEnemy (int, int, int, int, int);
	void deleteEnemySprites();

	QPixmap getPixmap (char type);

	void changeTheme (const QString & themeFilepath);

signals:
	void mouseClick (int);
	void mouseLetGo (int);

protected:
	virtual void mousePressEvent ( QMouseEvent * mouseEvent );
	virtual void mouseReleaseEvent ( QMouseEvent * mouseEvent );
	virtual void resizeEvent (QResizeEvent * event);
	virtual QSize sizeHint() const;

private:
	QCursor * m;			// Mouse cursor.
	KGrPlayField * playfield;	// Array of tiles where runners can run.

	int scaleStep;			// Current scale-factor of canvas.
	int baseScale;			// Starting scale-factor of canvas.
	int baseFontSize;

	int nCellsW;			// Number of tiles horizontally.
	int nCellsH;			// Number of tiles vertically.
	int border;			// Number of tiles allowed for border.
	int lineDivider;		// Fraction of a tile for inner border.
	QPoint topLeft;			// Top left point of the tile array.

	QColor borderColor, textColor;	// Border colours.
	QLabel * title;			// Title and top part of border.

	int freebg, nuggetbg, polebg, ladderbg, hladderbg;
	int edherobg, edenemybg, betonbg, brickbg, fbrickbg;
	int bgw, bgh;			// Size of KGoldrunner 2 tile QPixmap.
	int imgW, imgH;			// Scaled size of KGr 3 tile QImage.
	int oldImgW, oldImgH;

	int goldEnemy;

	KGrSprite * heroSprite;
	QList<KGrSprite *> * enemySprites;
	QList<KGameCanvasRectangle *> borderRectangles;

	void initView();
	void drawTheScene (bool changePixmaps);
	void makeTiles (bool changePixmaps);
	void makeBorder();
	void makeTitle();

	QColor colour;
	KGameCanvasRectangle * drawRectangle (int x, int y, int w, int h);
	void changeColours (const char * colours []);
	void recolourObject (const char * object [], const char * colours []);

	QList<QPixmap> * tileset;
	void appendSVGTile (QImage & img, QPainter & q, const QString & name);

	QList<QPixmap> * heroFrames;
	QList<QPixmap> * enemyFrames;
	void appendXPMFrames (const QImage & image,
		QList<QPixmap> * frames, const int nFrames);
	void appendSVGFrames (const QString & elementPattern,
		QList<QPixmap> * frames, const int nFrames);

	KSvgRenderer svg;
	QString picsDataDir;
	QString filepathSVG;
	QString m_themeFilepath;
        short themeDrawBorder;
	void loadSVGTheme();

	enum GraphicsType {NONE, SVG, XPM, PNG};
	GraphicsType tileGraphics;
	GraphicsType backgroundGraphics;
	GraphicsType runnerGraphics;

	// IDW - Temporary ... should use a more general playfield (grid) idea.
	int tileNo [FIELDWIDTH] [FIELDHEIGHT];
	int resizeCount;		// IDW - Temporary, for qDebug() logs.
	QTime t; // IDW
};

#endif // KGRCANVAS_H
