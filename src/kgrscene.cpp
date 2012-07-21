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

#include <QDebug>
#include <QGraphicsView>
#include <kdebug.h>

#include <KGlobal>
#include <KConfig>
#include <KConfigGroup>

#include "kgrscene.h"
#include "kgrglobals.h"
#include "kgrrenderer.h"

KGrScene::KGrScene (QObject * parent)
    :
    QGraphicsScene (parent),
    // Allow FIELDWIDTH * FIELDHEIGHT tiles for the KGoldruner level-layouts,
    // plus 2 more tile widths all around for text areas, frame and spillover
    // for mouse actions (to avoid accidental clicks affecting the desktop).
    m_background    (0),
    m_tilesWide     (FIELDWIDTH  + 2 * 2),
    m_tilesHigh     (FIELDHEIGHT + 2 * 2),
    m_tileSize      (10)
{
    m_renderer      = new KGrRenderer (this);

    setBackgroundBrush (m_renderer->borderColor());

    if (m_renderer->hasBorder())
        drawBorder();
}

KGrScene::~KGrScene()
{
}

void KGrScene::redrawScene ()
{
    // Calculate what size of tile will fit in the view.
    QSize size = views().at(0)->size();
    int tileSize = qMin (size.width()/m_tilesWide, size.height()/m_tilesHigh);

    qreal unusedX = (size.width()  - m_tilesWide * tileSize)/2.0;
    qreal unusedY = (size.height() - m_tilesHigh * tileSize)/2.0;
    unusedX = unusedX / tileSize;	// Fraction of a tile at L and R.
    unusedY = unusedY / tileSize;	// Fraction of a tile at T and B.
    setSceneRect (-1.0 - unusedX, -1.0 - unusedY,
                  m_tilesWide + 2.0 * unusedX, m_tilesHigh + 2.0 * unusedY);

    loadBackground(1); // Test level 1.

    // Checks wether the background and all the other visual elements need to be
    // resized.
    if (tileSize != m_tileSize) {
	m_background->setRenderSize (QSize ((m_tilesWide - 4) * tileSize,
                                            (m_tilesHigh - 4) * tileSize));
	m_background->setScale (1.0 / tileSize);

        if (!borderElements.isEmpty()) {
            foreach (KGameRenderedItem * item, borderElements)
                setTileSize (item, tileSize);
        }
    }

    m_background->setPos (1, 1);
}

void KGrScene::currentThemeChanged (const KgTheme *)
{
    if (m_renderer->hasBorder())
        drawBorder();

    // Fill the scene (and view) with the new background color.  Do this even if
    // the background has no border, to avoid ugly white rectangles appearing
    // if rendering and painting is momentarily a bit slow.
    setBackgroundBrush (m_renderer->borderColor());
    redrawScene();
}

void KGrScene::loadBackground (const int level)
{
    // NOTE: The background picture can be the same size as the level-layout (as
    // in the Egypt theme) OR it can be the same size as the entire viewport.
    if (m_tiles.count() == 0) {
	// In this example the background is fitted into the level-layout.
	m_background = m_renderer->getBackground (level, m_background);
        addItem(m_background);
    }
}

void KGrScene::setTileSize (KGameRenderedItem * tile, const int tileSize)
{
    tile->setRenderSize (QSize (tileSize, tileSize));
    tile->setScale (1.0 / tileSize);
}

void KGrScene::drawBorder()
{
    foreach (KGameRenderedItem * item, borderElements) {
        removeItem (item);
        delete item;
    }

    borderElements.clear();
    borderElements = m_renderer->borderTiles();

    KGameRenderedItem * item;

    // Top-left corner.
    item = borderElements.at(0);
    addItem (item);
    item->setPos(0, 0);

    // Top-right corner.
    item = borderElements.at(1);
    addItem (item);
    item->setPos(FIELDWIDTH + 1, 0);

    // Bottom-left corner.
    item = borderElements.at(2);
    addItem (item);
    item->setPos(0, FIELDHEIGHT + 1);

    // Bottom-right corner.
    item = borderElements.at(3);
    addItem (item);
    item->setPos(FIELDWIDTH + 1, FIELDHEIGHT + 1);

    // Defines either the x or y coordinate of a border tile. In each loop its
    // value follows the sequence {1, 2, 3, ...}.
    int pos;

    // Adds the top side tiles to the scene.
    for (int i = TOP_BORDER_BEGIN; i <= TOP_BORDER_END; i++) {
        item    = borderElements.at(i);
        pos     = (i - TOP_BORDER_BEGIN) + 1;

        addItem (item);
        item->setPos (pos, 0);
    }

    // Adds the bottom side tiles to the scene.
    for (int i = BOTTOM_BORDER_BEGIN; i <= BOTTOM_BORDER_END; i++) {
        item    = borderElements.at(i);
        pos     = (i - BOTTOM_BORDER_BEGIN) + 1;

        addItem (item);
        item->setPos (pos, FIELDHEIGHT + 1);
    }

    // Adds the left side tiles to the scene.
    for (int i = LEFT_BORDER_BEGIN; i <= LEFT_BORDER_END; i++) {
        item    = borderElements.at(i);
        pos     = (i - LEFT_BORDER_BEGIN) + 1;

        addItem (item);
        item->setPos (0, pos);
    }

    // Adds the right side tiles to the scene.
    for (int i = RIGHT_BORDER_BEGIN; i <= RIGHT_BORDER_END; i++) {
        item    = borderElements.at(i);
        pos     = (i - RIGHT_BORDER_BEGIN) + 1;

        addItem (item);
        item->setPos (FIELDWIDTH + 1, pos);
    }
}

#include "kgrscene.moc"
