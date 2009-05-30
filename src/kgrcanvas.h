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

enum StartFrame     {RIGHTWALK1,  RIGHTWALK2,  RIGHTWALK3,  RIGHTWALK4,
                     RIGHTWALK5,  RIGHTWALK6,  RIGHTWALK7,  RIGHTWALK8,
                     LEFTWALK1,   LEFTWALK2,   LEFTWALK3,   LEFTWALK4,
                     LEFTWALK5,   LEFTWALK6,   LEFTWALK7,   LEFTWALK8,
                     RIGHTCLIMB1, RIGHTCLIMB2, RIGHTCLIMB3, RIGHTCLIMB4,
                     RIGHTCLIMB5, RIGHTCLIMB6, RIGHTCLIMB7, RIGHTCLIMB8,
                     LEFTCLIMB1,  LEFTCLIMB2,  LEFTCLIMB3,  LEFTCLIMB4,
                     LEFTCLIMB5,  LEFTCLIMB6,  LEFTCLIMB7,  LEFTCLIMB8,
                     CLIMB1,      CLIMB2,
                     FALL1,       FALL2};

class KGrCanvas : public KGameCanvasWidget
{
    Q_OBJECT
public:
    KGrCanvas (QWidget * parent, const double scale,
               const QString & systemDataDir);
    virtual ~KGrCanvas();

    void setBaseScale();

    void setTitle (const QString&);

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

    inline void setGoldEnemiesRule (bool showIt) { enemiesShowGold = showIt;}

public slots:
    void getMousePos       (int & i, int & j);
    void setMousePos       (const int, const int);
    void animate           (bool missed);
    void paintCell         (const int i, const int j, const char type,
                            const int offset = 0);

    int  makeSprite        (const char type, int i, int j);
    void startAnimation    (const int id, const bool repeating,
                            const int i, const int j, const int time,
                            const Direction dirn, const AnimationType type);
    // TODO - Implement this method, if it is really needed.
    // void resynchAnimation  (const int id, const int i, const int j,
                            // const bool stop);
    void gotGold           (const int spriteId, const int i, const int j,
                            const bool spriteHasGold, const bool lost = false);
    void showHiddenLadders (const QList<int> & ladders, const int width);
    void deleteSprite      (const int id);
    void deleteAllSprites  ();

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

    int emptySprites;
    QList<KGrSprite *> * sprites;

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

    bool enemiesShowGold;		// Show or conceal if enemies have gold.
    int  heroId;			// The hero's sprite ID.
};
#endif // KGRCANVAS_H
// vi: set sw=4 :
