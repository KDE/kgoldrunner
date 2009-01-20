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
#include "kgrconsts.h" // OBSOLESCENT - 11/1/09
#include "kgrglobals.h"

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

    void setTitle (const QString&);

    void setHeroVisible (bool);
    void moveHero (int x, int y, int frame);

    void moveEnemy (int, int, int, int, int);
    void deleteEnemySprites();

    void goToBlack();
    void fadeIn();
    void fadeOut();
    void updateScore (int score);
    void updateLives (int lives);

    QPixmap getPixmap (char type);

    bool changeTheme (const QString & themeFilepath);
    /**
     * setLevel is meant as a way to communicate that new graphics can
     * be used if multiple sets are available in the theme.
     */
    void setLevel (unsigned int level);

public slots:
    void animate          ();
    void paintCell        (const int row, const int col, const char type,
                           const int offset = 0);
    void setSpriteType    (const int id, const char type, int row, int col);
    void startAnimation   (const int id, const int row, const int col,
                           const int time,
                           const Direction dirn, const AnimationType type);
    void resynchAnimation (const int id, const int row, const int col,
                           const bool stop);
    void deleteAnimation  (const int id);

    void makeHeroSprite (int, int, int);
    void makeEnemySprite (int, int, int);

    inline void jumpHero (int i, int j, int frame) // OBSOLESCENT? - 9/1/09
                         { moveHero (i * bgw, j * bgh, frame); }

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

    KGameCanvasRectangle * drawRectangle (int x, int y, int w, int h);

    KGameCanvasPixmap * makeBorderElement (QList< QPixmap > frameTiles, 
                                           int x, int y, int which);

    KGameCanvasPixmap * makeDisplay (QList< QPixmap > tiles, int w);

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

    int bgw, bgh;			// Size of KGoldrunner 2 tile QPixmap.
    int imgW, imgH;			// Scaled size of KGr 3 tile QImage.
    int oldImgW, oldImgH;

    int goldEnemy;

    KGameCanvasPicture *m_spotLight;
    KGameCanvasText *m_scoreText;
    KGameCanvasText *m_livesText;
    KGameCanvasPixmap * m_scoreDisplay;
    KGameCanvasPixmap * m_livesDisplay;

    QTimeLine m_fadingTimeLine;

    KGrSprite * heroSprite;
    QList<KGrSprite *> * enemySprites;
    QList<KGameCanvasRectangle *> borderRectangles;
    QList<KGameCanvasPixmap *> borderElements;
    QColor colour;

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
    
    // Keep current score and lives 
    int lives;
    int score;
};
#endif // KGRCANVAS_H
// vi: set sw=4 :
