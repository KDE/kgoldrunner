/****************************************************************************
 *    Copyright 2012 Roney Gomes <roney477@gmail.com>                       *
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

/**
 * @class KGrScene  kgrscene.h
 * @short The QGraphicsScene that represents KGoldrunner on the screen.
 *
 * In the KGoldrunner scene, the KGoldrunner level-layouts use tile-coordinates
 * that run from (1, 1) to (28, 20). To simplify programming, these are exactly
 * the same as the cell co-ordinates used in the game-engine (or model).
 *
 * The central grid has internal coordinates running from (-1, -1) to (30, 22),
 * making 32x24 spaces. The empty space around the level-layout (2 cells wide
 * all around) is designed to absorb over-enthusiastic mouse actions, which
 * could otherwise cause accidents on other windows or the surrounding desktop.
 *
 * Rows -1, 0, 29 and 30 usually contain titles and scores, which could have
 * fractional co-ordinates. Row 0, row 21, column 0 and column 29 can also
 * contain border-tiles (as in the Egyptian theme).
 *
 * The hero and enemies (sprites) will have fractional co-ordinates as they move
 * from one cell to another. Other graphics items (tiles) always have whole
 * number co-ordinates (e.g. bricks, ladders and concrete). Empty spaces have
 * no graphic item and the background shows through them.
 *
 * The width and height of the view will rarely be an exact number of tiles, so
 * there will be unused strips outside the 32x24 tile spaces. The sceneRect() is
 * set larger than 24 tiles by 32 tiles to allow for this and its top left
 * corner has negative fractional co-ordinates that are calculated but never
 * actually used. Their purpose is to ensure that (1, 1) is always the top left
 * corner of the KGoldrunner level layout, whatever the size/shape of the view.
 */
#ifndef KGRSCENE_H
#define KGRSCENE_H

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>

#include <KgTheme>
#include <KGameRenderedItem>

#include "kgrglobals.h"

class KGrView;
class KGrSprite;
class KGrRenderer;
class KGameRenderer;

enum StartFrame     {RIGHTWALK1 = 1,  RIGHTWALK2,  RIGHTWALK3,  RIGHTWALK4,
                     RIGHTWALK5,  RIGHTWALK6,  RIGHTWALK7,  RIGHTWALK8,
                     LEFTWALK1,   LEFTWALK2,   LEFTWALK3,   LEFTWALK4,
                     LEFTWALK5,   LEFTWALK6,   LEFTWALK7,   LEFTWALK8,
                     RIGHTCLIMB1, RIGHTCLIMB2, RIGHTCLIMB3, RIGHTCLIMB4,
                     RIGHTCLIMB5, RIGHTCLIMB6, RIGHTCLIMB7, RIGHTCLIMB8,
                     LEFTCLIMB1,  LEFTCLIMB2,  LEFTCLIMB3,  LEFTCLIMB4,
                     LEFTCLIMB5,  LEFTCLIMB6,  LEFTCLIMB7,  LEFTCLIMB8,
                     CLIMB1,      CLIMB2,
                     FALL1,       FALL2,
                     DIGBRICK1 = 1, DIGBRICK2, DIGBRICK3, DIGBRICK4,
                     DIGBRICK5,
                     DIGBRICK6, DIGBRICK7, DIGBRICK8, DIGBRICK9};

class KGrScene : public QGraphicsScene
{
    Q_OBJECT
public:
    KGrScene                (KGrView * view);
    ~KGrScene               ();

    /*
     * Redraw the scene whenever the current theme has changed.
     */
    void changeTheme        ();

    /*
     * Redraw the scene whenever the scene is resized.
     */
    void changeSize         ();

    /*
     * Get the current size of the squared region occupied by a single visual
     * element (characters, ladders, bricks etc.).
     */
    QSize tileSize          () const { return QSize (m_tileSize, m_tileSize); }

    /*
     * Get a pointer to the scene's renderer.
     */
    KGrRenderer * renderer  () const { return m_renderer; }

    inline void setGoldEnemiesRule (bool showIt) { enemiesShowGold = showIt; }

public slots:
    int  makeSprite         (const char type, int i, int j);

    void animate            (bool missed);
    void gotGold            (const int spriteId, const int i, const int j,
                             const bool spriteHasGold, const bool lost = false);
    void showHiddenLadders  (const QList<int> & ladders, const int width);
    void deleteSprite       (const int id);
    void deleteAllSprites   ();

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
    void paintCell          (const int i, const int j, const char type);

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

    void setMousePos (const int i, const int j);
    void getMousePos (int & i, int & j);

private:
    /*
     * Actions performed whenever the viewport is resized or a different theme
     * is loaded.
     */
    void redrawScene    ();

    /*
     * Load and set the size and position of the KGameRenderedItem's which make
     * up the border layout.
     */
    void drawBorder     ();

    /*
     * Load and set the size and position of the background image for the
     * current level.
     *
     * @param level The current level.
     */
    void loadBackground (const int level);

    /*
     * Add a new element, with coordinates x and y, to the border-layout.
     *
     * @param spriteKey The sprite key of the requested item.
     * @param x         The item's x coordinate.
     * @param y         The item's y coordinate.
     */
    void setBorderTile  (const QString spriteKey, const int x, const int y);

    /*
     * Resize a game's visual element.
     *
     * @param tile      The element to be resized.
     * @param tileSize  The new size.
     */
    void setTile        (KGameRenderedItem * tile, const int tileSize,
                         const int i, const int j);

    KGrView             *   m_view;
    KGrRenderer         *   m_renderer;
    KGameRenderedItem   *   m_background;

    QGraphicsTextItem   *   m_scoreText;
    QGraphicsTextItem   *   m_livesText;

    QGraphicsPixmapItem *   m_scoreDisplay;
    QGraphicsPixmapItem *   m_livesDisplay;

    int                     m_heroId;
    int                     m_tilesWide;
    int                     m_tilesHigh;
    int                     m_tileSize;

    bool                    m_sizeChanged;
    bool                    m_themeChanged;

    // The animated sprites for dug bricks, hero and enemies.
    QList <KGrSprite *> m_sprites;

    // The visible elements of the scenario (tiles and borders), excluding the
    // background picture and the animated sprites.
    QVector <KGameRenderedItem *> m_tiles;

    // The type of each tile stored in m_tiles.
    QByteArray m_tileTypes;

    bool enemiesShowGold;		// Show or conceal if enemies have gold.

    int m_topLeftX;
    int m_topLeftY;

    QCursor * m_mouse;
};

#endif // KGRSCENE_H
