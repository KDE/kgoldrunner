/***************************************************************************
                         kgrcanvas.h  -  description
                             -------------------
    begin                : Wed Jan 23 2002
    Copyright 2002 Marco Krüger <grisuji@gmx.de>
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
#include "kgrconsts.h"

#include <qcursor.h>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QList>
#include <QTime> // IDW
#include <QTimeLine>

#include "kgrtheme.h"


class KGrCanvas : public KGameCanvasWidget
{
    Q_OBJECT
public:
    KGrCanvas (QWidget * parent, const double scale,
                            const QString & systemDataDir);
    virtual ~KGrCanvas();

    QPoint getMousePos();
    void setMousePos (int, int);

    void setBaseScale();

    void paintCell (int, int, char, int offset = 0);
    void setTitle (const QString&);

    void makeHeroSprite (int, int, int);
    void setHeroVisible (bool);
    void moveHero (int, int, int);

    void makeEnemySprite (int, int, int);
    void moveEnemy (int, int, int, int, int);
    void deleteEnemySprites();

    void goToBlack();
    void fadeIn();
    void fadeOut();

    QPixmap getPixmap (char type);

    bool changeTheme (const QString & themeFilepath);
    /**
     * setLevel is meant as a way to communicate that new graphics can
     * be used if multiple sets are available in the theme.
     */
    void setLevel (unsigned int level);

signals:
    void mouseClick (int);
    void mouseLetGo (int);
    void fadeFinished();

protected:
    virtual void mousePressEvent (QMouseEvent * mouseEvent);
    virtual void mouseReleaseEvent (QMouseEvent * mouseEvent);
    virtual void resizeEvent (QResizeEvent * event);
    virtual QSize sizeHint() const;

private slots:
    void drawSpotLight (qreal value);

private:
    bool firstSceneDrawn;		// Set AFTER the initial resize events.

    QCursor * m;			// Mouse cursor.
    KGrPlayField * playfield;		// Array of tiles where runners can run.

    int scaleStep;			// Current scale-factor of canvas.
    int baseScale;			// Starting scale-factor of canvas.
    int baseFontSize;

    int nCellsW;			// Number of tiles horizontally.
    int nCellsH;			// Number of tiles vertically.
    int border;				// Number of tiles allowed for border.
    int lineDivider;			// Fraction of a tile for inner border.
    QPoint topLeft;			// Top left point of the tile array.

    QLabel * title;			// Title and top part of border.

    int freebg, nuggetbg, polebg, ladderbg, hladderbg;
    int edherobg, edenemybg, betonbg, brickbg, fbrickbg;
    int bgw, bgh;			// Size of KGoldrunner 2 tile QPixmap.
    int imgW, imgH;			// Scaled size of KGr 3 tile QImage.
    int oldImgW, oldImgH;

    int goldEnemy;

    KGameCanvasPicture *m_spotLight;
    QTimeLine m_fadingTimeLine;

    KGrSprite * heroSprite;
    QList<KGrSprite *> * enemySprites;
    QList<KGameCanvasRectangle *> borderRectangles;
    QList<KGameCanvasPixmap *> borderElements;

    void initView();

    /**
     * Load background appropriate for current level
     */
    void loadBackground();

    void drawTheScene (bool changePixmaps);
    void makeBorder();
    void makeTitle();
    KGrTheme::TileType tileForType(char type);
    int tileNumber(KGrTheme::TileType type, int x, int y);

    QColor colour;
    KGameCanvasRectangle * drawRectangle (int x, int y, int w, int h);

    KGameCanvasPixmap * makeBorderElement (QList< QPixmap > frameTiles, int x, int y, int which);

    QList<QPixmap> * tileset;

    QList<QPixmap> * heroFrames;
    QList<QPixmap> * enemyFrames;
    KGrTheme theme;

    // IDW - Temporary ... should use a more general playfield (grid) idea.
    KGrTheme::TileType tileNo [FIELDWIDTH] [FIELDHEIGHT];
    unsigned char randomOffsets [FIELDWIDTH] [FIELDHEIGHT];

    int resizeCount;			// =0 until the main window has resized.
    QTime t; // IDW
    unsigned int level;
};
#endif // KGRCANVAS_H
// vi: set sw=4 :
