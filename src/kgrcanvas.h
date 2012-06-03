/****************************************************************************
 *    Copyright 2002  Marco Krüger <grisuji@gmx.de>                         *
 *    Copyright 2002  Ian Wadham <iandw.au@gmail.com>                       *
 *    Copyright 2006  Mauricio Piacentini <mauricio@tabuleiro.com>          *
 *    Copyright 2007  Mauricio Piacentini <mauricio@tabuleiro.com>          *
 *    Copyright 2007  Luciano Montanaro <mikelima@cirulla.net>              *
 *    Copyright 2008  Luciano Montanaro <mikelima@cirulla.net>              *
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                       *
 *                                                                          *
 *    This program is free software; you can redistribute it and/or         *
 *    modify it under the terms of the GNU General Public License as        *
 *    published by the Free Software Foundation; either version 2 of        *
 *    the License, or (at your option) any later version.                   *
 *                                                                          *
 *    This program is distributed in the hope that it will be useful,       *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *    GNU General Public License for more details.                          *
 *                                                                          *
 *    You should have received a copy of the GNU General Public License     *
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ****************************************************************************/

#ifndef KGRCANVAS_H
#define KGRCANVAS_H

#include "kgrglobals.h"

#include "kgrtheme.h"

#include <QPixmap>
#include <QList>
#include <QTime> // For IDW testing and performance monitoring.
#include <QTimeLine>

#define USE_UNSTABLE_LIBKDEGAMESPRIVATE_API
#include <libkdegamesprivate/kgamecanvas.h>

class KGrSprite;
class KGrPlayField;

class QCursor;
class QLabel;
class QMouseEvent;

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
    KGrCanvas (QWidget * parent, const double scale);
    virtual ~KGrCanvas();

    void setBaseScale();

    void setTitle (const QString&);

    void goToBlack();
    void fadeIn();
    void fadeOut();
    void updateScore (int score);
    void updateLives (int lives);
    void makeReplayMessage();
    void showReplayMessage (bool onOff);

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

    /**
     * Requests the view to display a particular type of tile at a particular
     * cell, or make it empty and show the background (tileType = FREE). Used
     * when loading level-layouts and also to make gold disappear/appear, hidden
     * ladders appear or cells to be painted by the game editor.  If there was
     * something in the cell already, tileType = FREE acts as an erase function.
     *
     * @param i            The column-number of the cell to paint.
     * @param i            The row-number of the cell to paint.
     * @param tileType     The type of tile to paint (gold, brick, ladder, etc).
     */
    void paintCell         (const int i, const int j, const char type);

    int  makeSprite        (const char type, int i, int j);

    /**
     * Requests the view to display an animation of a runner or dug brick at a
     * particular cell, cancelling and superseding any current animation.
     *
     * @param spriteId     The ID of the sprite (dug brick).
     * @param repeating    If true, repeat the animation (false for dug brick).
     * @param i            The column-number of the cell.
     * @param j            The row-number of the cell.
     * @param time         The time in which to traverse one cell.
     * @param dirn         The direction of motion (always STAND for dug brick).
     * @param type         The type of animation (run, climb, open/close brick).
     */
    void startAnimation    (const int id, const bool repeating,
                            const int i, const int j, const int time,
                            const Direction dirn, const AnimationType type);

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
     * Load a background appropriate for the current level or, when initially
     * loading * a theme (lev = -1), load all backgrounds, so that they  are all
     * cached after the first SVG load, thus avoiding SVG reloads on later runs.
     */
    void loadBackground (const int lev);

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

    QLabel * m_replayText;		// Message for demos and replays.

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
