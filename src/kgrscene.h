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

class KGrView;
class KGrRenderer;
class KGameRenderer;

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

public slots:
    int  makeSprite         (const char type, int i, int j);
    void paintCell          (const int i, const int j, const char type);

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
    void setTileSize    (KGameRenderedItem * tile, const int tileSize);

    KGrView             *   m_view;
    KGrRenderer         *   m_renderer;
    KGameRenderedItem   *   m_background;

    QGraphicsTextItem   *   m_scoreText;
    QGraphicsTextItem   *   m_livesText;

    QGraphicsPixmapItem *   m_scoreDisplay;
    QGraphicsPixmapItem *   m_livesDisplay;

    int                     m_tilesWide;
    int                     m_tilesHigh;
    int                     m_tileSize;

    bool                    m_sizeChanged;
    bool                    m_themeChanged;

    // The visible elements of the scenario (tiles and borders), excluding the
    // background picture.
    QVector <KGameRenderedItem *> m_tiles;
    // The type of each tile stored in m_tiles.
    QByteArray m_tileTypes;
};

#endif // KGRSCENE_H
