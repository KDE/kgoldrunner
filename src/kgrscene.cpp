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

#include <QDebug>
#include <kdebug.h>

#include <KGlobal>
#include <KConfig>
#include <KConfigGroup>

#include "kgrview.h"
#include "kgrscene.h"
#include "kgrglobals.h"
#include "kgrrenderer.h"

KGrScene::KGrScene      (KGrView * view)
    :
    QGraphicsScene      (view),
    // Allow FIELDWIDTH * FIELDHEIGHT tiles for the KGoldruner level-layouts,
    // plus 2 more tile widths all around for text areas, frame and spillover
    // for mouse actions (to avoid accidental clicks affecting the desktop).
    m_view              (view),
    m_background        (0),
    m_scoreText         (0),
    m_livesText         (0),
    m_scoreDisplay      (0),
    m_livesDisplay      (0),
    m_tilesWide         (FIELDWIDTH  + 2 * 2),
    m_tilesHigh         (FIELDHEIGHT + 2 * 2),
    m_tileSize          (10),
    m_themeChanged      (true)
{
    m_tiles.fill        (0,     m_tilesWide * m_tilesHigh);
    m_tileTypes.fill    (FREE,  m_tilesWide * m_tilesHigh);

    m_renderer  = new KGrRenderer (this);
}

KGrScene::~KGrScene()
{
}

void KGrScene::redrawScene ()
{
    if (m_sizeChanged) {
        // Calculate what size of tile will fit in the view.
        QSize size      = m_view->size();
        int tileSize    = qMin (size.width()  / m_tilesWide,
                                size.height() / m_tilesHigh);

        qreal unusedX   = (size.width()  - m_tilesWide * tileSize)/2.0;
        qreal unusedY   = (size.height() - m_tilesHigh * tileSize)/2.0;

        unusedX         = unusedX / tileSize; // Fraction of a tile at L and R.
        unusedY         = unusedY / tileSize; // Fraction of a tile at T and B.

        setSceneRect (-1.0 - unusedX, -1.0 - unusedY,
                      m_tilesWide + 2.0 * unusedX, m_tilesHigh + 2.0 * unusedY);

        foreach (KGameRenderedItem * tile, m_tiles) {
            if (tile) {
                setTileSize (tile, tileSize);
            }
        }

        m_sizeChanged   = false;
        m_tileSize      = tileSize;
    }

    // NOTE: This is for testing only. As long as the scene is not connected to
    // the game engine loadBackground() will behave this way. 
    loadBackground (1);

    if (m_themeChanged) {
        // Fill the scene (and view) with the new background color.  Do this
        // even if the background has no border, to avoid ugly white rectangles
        // appearing if rendering and painting is momentarily a bit slow.
        setBackgroundBrush (m_renderer->borderColor());
        drawBorder();

        // Redraw the all the tiles, except for borders and tiles of type FREE.
        for (int i = 1; i <= FIELDWIDTH; i++) {
            for (int j = 1; j <= FIELDHEIGHT; j++) {
                int index = i * m_tilesHigh + j;
                paintCell (i, j, m_tileTypes[index]);
            }
        }

        m_themeChanged = false;
    }
}

void KGrScene::changeTheme()
{
    m_themeChanged = true;
    redrawScene();
}

void KGrScene::changeSize()
{
    m_sizeChanged = true;
    redrawScene();
}

void KGrScene::loadBackground (const int level)
{
    // NOTE: The background picture can be the same size as the level-layout (as
    // in the Egypt theme) OR it can be the same size as the entire viewport.
    // In this example the background is fitted into the level-layout.
    m_background = m_renderer->getBackground (level, m_background);

    m_background->setRenderSize (QSize ((m_tilesWide - 4) * m_tileSize,
                                        (m_tilesHigh - 4) * m_tileSize));
    m_background->setPos    (1, 1);
    // Keep the background behind the level layout.
    m_background->setZValue (-1);
    m_background->setScale  (1.0 / m_tileSize);
}

void KGrScene::setTileSize (KGameRenderedItem * tile, const int tileSize)
{
    tile->setRenderSize (QSize (tileSize, tileSize));
    tile->setScale (1.0 / tileSize);
}

void KGrScene::setBorderTile (const QString spriteKey, const int x, const int y)
{
    int index               = x * m_tilesHigh + y;
    KGameRenderedItem * t   = m_renderer->getBorderItem (spriteKey,
                                                         m_tiles.at(index));
    m_tiles[index]          = t;

    if (t) {
        setTileSize (t, m_tileSize);
        t->setPos (x, y);
    }
}

void KGrScene::drawBorder()
{
    // Corners.
    setBorderTile ("frame-topleft", 0, 0);
    setBorderTile ("frame-topright", FIELDWIDTH + 1, 0);
    setBorderTile ("frame-bottomleft", 0, FIELDHEIGHT + 1);
    setBorderTile ("frame-bottomright", FIELDWIDTH + 1, FIELDHEIGHT + 1);

    // Upper side.
    for (int i = 1; i <= FIELDWIDTH; i++)
        setBorderTile ("frame-top", i, 0);

    // Lower side.
    for (int i = 1; i <= FIELDWIDTH; i++)
        setBorderTile ("frame-bottom", i, FIELDHEIGHT + 1);

    // Left side.
    for (int i = 1; i <= FIELDHEIGHT; i++)
        setBorderTile ("frame-left", 0, i);

    // Right side.
    for (int i = 1; i <= FIELDHEIGHT; i++)
        setBorderTile ("frame-right", FIELDWIDTH + 1, i);
}

void KGrScene::paintCell (const int i, const int j, const char type)
{
    int index               = i * m_tilesHigh + j;
    KGameRenderedItem * t   = m_renderer->getTileItem (type, m_tiles.at(index));
    m_tiles[index]          = t;
    m_tileTypes[index]      = type;

    if (t) {
        setTileSize (t, m_tileSize);
        t->setPos   (i, j);
    }
}

int KGrScene::makeSprite (const char type, int i, int j)
{
    int id;
    int index = i * m_tilesHigh + j;

    KGameRenderedItem * sprite = 0;

    if (m_tiles.at(index) == 0) {
        // Occupy the slot corresponding to the given coordinates case it's
        // free.
        id = index;
    }
    else if (m_tiles.lastIndexOf(0) >= 0) {
        // Otherwise, occupy the last empty slot.
        id = m_tiles.lastIndexOf(0);
    }
    else {
        // Case the list is completely occupied, add one free slot to it.
        id = m_tiles.size();

        m_tiles.reserve     (id + 1);
        m_tileTypes.reserve (id + 1);
    }

    sprite          = m_renderer->getTileItem (type, m_tiles.at(id));
    m_tiles[id]     = sprite;
    m_tileTypes[id] = type;

    setTileSize     (sprite, m_tileSize);
    sprite->setPos  (i, j);

    // TODO: Set the starting frame of the sprite.
    switch (type) {
    case HERO:
        sprite->setZValue (1);
        break;
    case ENEMY:
        sprite->setZValue (2);
        break;
    case BRICK:
        paintCell (i, j, FREE);
        break;
    default:
        break;
    }

    return id;
}

#include "kgrscene.moc"
