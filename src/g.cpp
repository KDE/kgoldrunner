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

struct Tile {const int i; const int j; const char type; };
static const Tile test[] = {
    {2, 2, BRICK},	// One brick at each corner of the level-layout.
    {2, 21, BRICK},
    {29, 2, BRICK},
    {29, 21, BRICK},
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
    {20, 13, LADDER},	// A 6-piece ladder.
    {20, 14, LADDER},
    {20, 15, LADDER},
    {20, 16, LADDER},
    {20, 17, LADDER},
    {20, 18, LADDER},
    {14, 20, NUGGET},	// Some gold pieces.
    {15, 20, NUGGET},
    {16, 20, NUGGET},
    {17, 20, NUGGET},
    {20, 20, HERO},	// Extra tiles for the editor.
    {21, 20, ENEMY},
    {22, 20, FBRICK},
    {23, 20, HLADDER},
    {0, 0, FREE}
};

GS::GS (QObject * parent)
    :
    QGraphicsScene (parent),
    m_tilesWide    (32),
    m_tilesHigh    (24),
    m_tileSize     (10)
{
    m_renderer     = new KGrRenderer (this);
    m_renderSet    = m_renderer->getSetRenderer();
    m_renderActors = m_renderer->getActorsRenderer();

    // IDW test.
    m_grid = new QGraphicsRectItem(0, 0, m_tilesWide * m_tileSize,
                                         m_tilesHigh * m_tileSize);
    addItem (m_grid);
    for (int lev = 10; lev <= 13; lev++) {
	qDebug() << "Background" << lev << m_renderer->getBackgroundKey(lev);
    }
}

GS::~GS()
{
}

void GS::redrawScene (QSize size)
{
    // Centre the tile grid in the scene.
    int tileSize = qMin (size.width()/m_tilesWide, size.height()/m_tilesHigh);
    qDebug() << "View size" << size << "tile size" << tileSize << size.width() << "/" << m_tilesWide << "=" << size.width()/m_tilesWide << "|" << size.height() << "/" << m_tilesHigh << "=" << size.height()/m_tilesHigh;

    setSceneRect (0, 0, size.width(), size.height());
    m_gridTopLeft = QPoint ((size.width()  - m_tilesWide * tileSize)/2,
                            (size.height() - m_tilesHigh * tileSize)/2);
    qDebug() << "Top left is" << m_gridTopLeft;
    m_grid->setRect ((size.width()  - m_tilesWide * tileSize)/2,
                     (size.height() - m_tilesHigh * tileSize)/2,
                     m_tilesWide * tileSize, m_tilesHigh * tileSize);

    // NOTE: The background picture can be the same size as the level-layout, as
    // in this example (Egypt theme), OR the same size as the entire viewport.
    if (m_tiles.count() == 0) {
	QString pixmapKey = m_renderer->getPixmapKey (BACKDROP);
	m_background = new KGameRenderedItem (m_renderSet, pixmapKey);
	addItem (m_background);
	qDebug() << "BACKGROUND pixmap key" << pixmapKey;
    }
    if (tileSize != m_tileSize) {
	m_background->setRenderSize (QSize ((m_tilesWide - 4) * tileSize,
                                            (m_tilesHigh - 4) * tileSize));
    }
    m_background->setPos (m_gridTopLeft.x() + 2 * tileSize, 
			  m_gridTopLeft.y() + 2 * tileSize);

    if (m_tiles.count() == 0) {
	m_tileSize = tileSize;
	loadTestItems();
	return;
    }

    redrawTestItems (tileSize);
    m_tileSize = tileSize;
}

void GS::loadTestItems()
{
    m_tiles.fill (0, m_tilesWide * m_tilesHigh);

    int index = 0;
    while (test[index].type != FREE) {
	qDebug() << "Tile" << index << test[index].type << "at" <<
	    test[index].i << test[index].j;
	paintCell (test[index].i, test[index].j, test[index].type);
	index++;
    }
    m_hero   = new KGameRenderedItem (m_renderActors, "hero");
    addItem (m_hero);
    m_hero->setRenderSize (QSize (m_tileSize, m_tileSize));
    m_hero->setPos (m_gridTopLeft.x() + 8 * m_tileSize,
			    m_gridTopLeft.y() + 3 * m_tileSize);
    m_hero->setFrame (22);
    m_enemy1 = new KGameRenderedItem (m_renderActors, "enemy");
    addItem (m_enemy1);
    m_enemy1->setRenderSize (QSize (m_tileSize, m_tileSize));
    m_enemy1->setPos (m_gridTopLeft.x() + 8 * m_tileSize,
			    m_gridTopLeft.y() + 4 * m_tileSize);
    m_enemy1->setFrame (22);
    m_enemy2 = new KGameRenderedItem (m_renderActors, "gold_enemy");
    addItem (m_enemy2);
    m_enemy2->setRenderSize (QSize (m_tileSize, m_tileSize));
    m_enemy2->setPos (m_gridTopLeft.x() + 8 * m_tileSize,
			    m_gridTopLeft.y() + 5 * m_tileSize);
    m_enemy2->setFrame (22);
    m_brick = new KGameRenderedItem (m_renderSet, "brick");
    addItem (m_brick);
    m_brick->setRenderSize (QSize (m_tileSize, m_tileSize));
    m_brick->setPos (m_gridTopLeft.x() + 8 * m_tileSize,
			    m_gridTopLeft.y() + 6 * m_tileSize);
    m_brick->setFrame (1);
}

void GS::redrawTestItems (const int tileSize)
{
    // Re-size and re-position tiles.
    for (int i = 0; i < m_tilesWide; i++) {
	for (int j = 0; j < m_tilesHigh; j++) {
	    int t = i * m_tilesHigh + j;
	    if (m_tiles.at(t) != 0) {
		if (tileSize != m_tileSize) {
		    m_tiles.at(t)->setRenderSize (QSize (tileSize, tileSize));
		}
		m_tiles.at(t)->setPos (m_gridTopLeft.x() + i * tileSize,
			               m_gridTopLeft.y() + j * tileSize);
		qDebug() << "Tile" << m_tiles.at(t)->pixmap().size() <<
                            "at" << m_tiles.at(t)->pos();
	    }
	}
    }

    // Re-size, re-position and test-animate sprites.
    if (tileSize != m_tileSize) {
	m_hero->setRenderSize   (QSize (tileSize, tileSize));
	m_enemy1->setRenderSize (QSize (tileSize, tileSize));
	m_enemy2->setRenderSize (QSize (tileSize, tileSize));
	m_brick->setRenderSize  (QSize (tileSize, tileSize));
    }
    m_hero->setPos (m_gridTopLeft.x() + 8 * tileSize,
                                m_gridTopLeft.y() + 3 * tileSize);
    m_hero->setFrame (m_hero->frame() + 1);
    m_enemy1->setPos (m_gridTopLeft.x() + 8 * tileSize,
                                m_gridTopLeft.y() + 4 * tileSize);
    m_enemy1->setFrame (m_enemy1->frame() + 1);
    m_enemy2->setPos (m_gridTopLeft.x() + 8 * tileSize,
                                m_gridTopLeft.y() + 5 * tileSize);
    m_enemy2->setFrame (m_enemy2->frame() + 1);
    m_brick->setPos (m_gridTopLeft.x() + 8 * tileSize,
                                m_gridTopLeft.y() + 6 * tileSize);
    m_brick->setFrame (m_brick->frame() + 1);
}

void GS::paintCell (const int i, const int j, const char type)
{
    int offset = i * m_tilesHigh + j;
    qDebug() << "Offset" << offset << "i,j" << i << j;
    if (m_tiles.at(offset) != 0) {
	// TODO: Delete this tile?  Replace it with another type?
    }
    QString pixmapKey = m_renderer->getPixmapKey (type);
    if ((type == HERO) || (type == ENEMY)) {
	// Stationary pixmaps from the Actors file, used only by the editor.
	m_tiles[offset] = new KGameRenderedItem (m_renderActors, pixmapKey);
    }
    else {
	// Pixmaps from the Set file, used in level-layouts and in the editor.
	m_tiles[offset] = new KGameRenderedItem (m_renderSet, pixmapKey);
    }
    addItem (m_tiles.at(offset));
    m_tiles.at(offset)->setRenderSize (QSize (m_tileSize, m_tileSize));
    m_tiles.at(offset)->setPos (m_gridTopLeft.x() + (i + 1) * m_tileSize,
                                m_gridTopLeft.y() + (j + 1) * m_tileSize);
    qDebug() << "Tile" << type << "i,j" << i << j << "size" << m_tileSize <<
                "at" << m_tiles.at(offset)->pos();
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

#include "g.moc"
