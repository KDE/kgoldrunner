/****************************************************************************
 *    Copyright 2012  Ian Wadham <iandw.au@gmail.com>                       *
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

#include <KgTheme>
#include <KGameRenderedItem>

class KGrRenderer;
class KGameRenderer;

class KGrScene : public QGraphicsScene
{
    Q_OBJECT
public:
    KGrScene (QObject * parent = 0);
    ~KGrScene ();

    void            redrawScene ();
    void            drawBorder();

    KGrRenderer *   renderer () const { return m_renderer; }
public slots:
    void currentThemeChanged(const KgTheme *);
private:
    void loadBackground (const int level);
    void setTileSize (KGameRenderedItem * tile, const int tileSize);

    KGrRenderer         *   m_renderer;
    KGameRenderer       *   m_renderSet;
    KGameRenderedItem   *   m_background;

    int                     m_tilesWide;
    int                     m_tilesHigh;
    int                     m_tileSize;

    QVector<KGameRenderedItem *> m_tiles;
    QList< KGameRenderedItem * > borderElements;
};

#endif // KGRSCENE_H
