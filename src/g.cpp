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

#include "kgrglobals.h"
#include "kgrrenderer.h"
#include "g.h"

#include <QBrush>
#include <QGraphicsRectItem>
#include <KGameRenderedItem>

#include <QDebug>

// This is a list of tile-types and co-ordinates to be to be used in this test
struct Tile {const int i; const int j; const char type; };
static const Tile test[] = {
    {1, 1, BRICK},	// One brick at each corner of the level-layout.
    {1, 20, BRICK},
    {28, 1, BRICK},
    {28, 20, BRICK},
    {10, 10, BRICK},	// A block of 12 bricks.
    {10, 11, BRICK},
    {10, 12, BRICK},
    {10, 13, BRICK},
    {11, 10, BRICK},
    {11, 11, BRICK},
    {11, 12, BRICK},
    {11, 13, BRICK},
    {12, 10, BRICK},
    {12, 11, BRICK},
    {12, 12, BRICK},
    {12, 13, BRICK},
    {14, 10, CONCRETE},	// A block of 12 pieces of concrete.
    {14, 11, CONCRETE},
    {14, 12, CONCRETE},
    {14, 13, CONCRETE},
    {15, 10, CONCRETE},
    {15, 11, CONCRETE},
    {15, 12, CONCRETE},
    {15, 13, CONCRETE},
    {16, 10, CONCRETE},
    {16, 11, CONCRETE},
    {16, 12, CONCRETE},
    {16, 13, CONCRETE},
    {18, 11, BAR},	// A row of 6 bars.
    {19, 11, BAR},
    {20, 11, BAR},
    {21, 11, BAR},
    {22, 11, BAR},
    {23, 11, BAR},
    {20, 12, LADDER},	// A 6-piece ladder.
    {20, 13, LADDER},
    {20, 14, LADDER},
    {20, 15, LADDER},
    {20, 16, LADDER},
    {20, 17, LADDER},
    {14, 19, NUGGET},	// Some gold pieces.
    {15, 19, NUGGET},
    {16, 19, NUGGET},
    {17, 19, NUGGET},
    {20, 19, HERO},	// Extra tiles for the editor.
    {21, 19, ENEMY},
    {22, 19, FBRICK},
    {23, 19, HLADDER},
    {0, 0, FREE}
};

GS::GS (QObject * parent)
    :
    QGraphicsScene (parent),
    // Allow FIELDWIDTH * FIELDHEIGHT tiles for the KGoldruner level-layouts,
    // plus 2 more tile widths all around for text areas, frame and spillover
    // for mouse actions (to avoid accidental clicks affecting the desktop).
    m_tilesWide    (FIELDWIDTH  + 2 * 2),
    m_tilesHigh    (FIELDHEIGHT + 2 * 2),
    m_tileSize     (10)
{
    m_renderer     = new KGrRenderer (dynamic_cast<QGraphicsScene *>(this));

    // NOTE: We should be able to remove m_renderSet and m_renderActors from
    // this class, by adding methods such as getSpriteItem(...) to KGrRenderer.
    m_renderSet    = m_renderer->getSetRenderer();
    m_renderActors = m_renderer->getActorsRenderer();

    // View is always >= m_grid, leaving >=2 tile widths of border.  m_grid is
    // just for testing, to show the boundaries of the KGoldrunner display area.
    m_grid = new QGraphicsRectItem(-1, -1, m_tilesWide, m_tilesHigh);
    m_grid->setPen (QPen (Qt::white));
    addItem (m_grid);
}

GS::~GS()
{
}

void GS::redrawScene (QSize size)
{
    // Calculate what size of tile will fit in the view.
    int tileSize = qMin (size.width()/m_tilesWide, size.height()/m_tilesHigh);
    //qDebug() << "View size" << size << "tile size" << tileSize;

    qreal unusedX = (size.width()  - m_tilesWide * tileSize)/2.0;
    qreal unusedY = (size.height() - m_tilesHigh * tileSize)/2.0;
    unusedX = unusedX / tileSize;	// Fraction of a tile at L and R.
    unusedY = unusedY / tileSize;	// Fraction of a tile at T and B.
    setSceneRect (-1.0 - unusedX, -1.0 - unusedY,
                  m_tilesWide + 2.0 * unusedX, m_tilesHigh + 2.0 * unusedY);
    //qDebug() << "UNUSED" << unusedX << unusedY << "SCENE RECT" << sceneRect();

    // NOTE: The background picture can be the same size as the level-layout (as
    // in the Egypt theme) OR it can be the same size as the entire viewport.
    if (m_tiles.count() == 0) {
	// In this example the background is fitted into the level-layout.
	QString pixmapKey = m_renderer->getBackgroundKey (1); // Test level 1.
	m_background = new KGameRenderedItem (m_renderSet, pixmapKey);
	addItem (m_background);
	qDebug() << "BACKGROUND pixmap key" << pixmapKey;
    }
    if (tileSize != m_tileSize) {
	m_background->setRenderSize (QSize ((m_tilesWide - 4) * tileSize,
                                            (m_tilesHigh - 4) * tileSize));
	m_background->setScale (1.0 / tileSize);
    }
    m_background->setPos (1, 1);

    // Initially, when the grid is empty, add some tiles (for testing only).
    if (m_tiles.count() == 0) {
	m_tileSize = tileSize;
	loadTestItems();
	return;
    }

    // Redraw the scene with the new view-size.
    redrawTestItems (tileSize);
    m_tileSize = tileSize;
}

void GS::loadTestItems()
{
    m_tiles.fill (0, m_tilesWide * m_tilesHigh);
    qDebug() << "Tile count" << m_tiles.count() << "width" << m_tilesWide
	     << "height" << m_tilesHigh;

    int index = 0;
    while (test[index].type != FREE) {
	paintCell (test[index].i, test[index].j, test[index].type);
	index++;
    }
    m_hero   = new KGameRenderedItem (m_renderActors, "hero");
    addItem (m_hero);
    setTileSize (m_hero, m_tileSize);
    m_hero->setPos (8, 3);
    m_hero->setFrame (22);

    m_enemy1 = new KGameRenderedItem (m_renderActors, "enemy");
    addItem (m_enemy1);
    setTileSize (m_enemy1, m_tileSize);
    m_enemy1->setPos (8, 4);
    m_enemy1->setFrame (22);

    m_enemy2 = new KGameRenderedItem (m_renderActors, "gold_enemy");
    addItem (m_enemy2);
    setTileSize (m_enemy2, m_tileSize);
    m_enemy2->setPos (8, 5);
    m_enemy2->setFrame (22);

    m_brick = new KGameRenderedItem (m_renderSet, "brick");
    addItem (m_brick);
    setTileSize (m_brick, m_tileSize);
    m_brick->setPos (8, 6);
    m_brick->setFrame (1);
    qDebug() << "GS::loadTestItems() DONE";
}

void GS::redrawTestItems (const int tileSize)
{
    if (tileSize != m_tileSize) {
        // Re-size tiles.
        for (int i = 0; i < m_tilesWide; i++) {
            for (int j = 0; j < m_tilesHigh; j++) {
                int index = i * m_tilesHigh + j;
                if (m_tiles.at(index) != 0) {
                    setTileSize (m_tiles.at(index), tileSize);
                }
            }
        }

        // Re-size sprites.
        setTileSize (m_hero,   tileSize);
        setTileSize (m_enemy1, tileSize);
        setTileSize (m_enemy2, tileSize);
        setTileSize (m_brick,  tileSize);
    }

    // Test-animate sprites.
    m_hero->setFrame (m_hero->frame() + 1);
    m_enemy1->setFrame (m_enemy1->frame() + 1);
    m_enemy2->setFrame (m_enemy2->frame() + 1);
    m_brick->setFrame (m_brick->frame() + 1);
}

void GS::paintCell (const int i, const int j, const char type)
{
    int index = i * m_tilesHigh + j;
    KGameRenderedItem * t = m_renderer->getTileItem (type, m_tiles.at(index));
    m_tiles[index] = t;
    if (t) {		// t = 0 if tile was deleted (type FREE).
        setTileSize (t, m_tileSize);
	t->setPos (i, j);
    }
}

void GS::setTileSize (KGameRenderedItem * tile, const int tileSize)
{
    tile->setRenderSize (QSize (tileSize, tileSize));
    tile->setScale (1.0 / tileSize);
}

// Minimal QGraphicsView code.
GV::GV (QWidget * parent)
    :
    QGraphicsView (parent),
    m_scene       (new GS (parent))
{
    setScene (m_scene);
}

GV::~GV()
{
}

void GV::resizeEvent(QResizeEvent* event)
{
    if (scene() != 0) {
        m_scene->redrawScene (event->size());
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
    QGraphicsView::resizeEvent(event);
}

#include "g.moc"
