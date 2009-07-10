/****************************************************************************
 *    Copyright 2006  Mauricio Piacentini <mauricio@tabuleiro.com>          *
 *    Copyright 2006  Dmitry Suzdalev <dimsuz@gmail.com>                    *
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

#include "kgrplayfield.h"

#include <KDebug>

KGrPlayField::KGrPlayField (KGameCanvasAbstract* canvas)
    : KGameCanvasGroup (canvas), m_tileset (0), m_background (0)
{
    show();
}

KGrPlayField::~KGrPlayField()
{
    // Clear all stored data.
    while (!m_tilesprites.isEmpty()) 
        delete m_tilesprites.takeFirst();
    delete m_background;
}

void KGrPlayField::setTile (int x, int y, int tilenum)
{
    Q_ASSERT (m_tileset);
    // Update the sprite pixmap using our tileset cache.
    if ((m_background != 0) && (tilenum == 0)) {
        m_tilesprites.at(y*m_numTilesH + x)->hide();
    }
    else {
        m_tilesprites.at(y*m_numTilesH + x)->setPixmap (m_tileset->at(tilenum));
        m_tilesprites.at(y*m_numTilesH + x)->show();
        m_tilesprites.at(y*m_numTilesH + x)->raise();
    }
}

void KGrPlayField::setBackground (const bool create, const QPixmap &background,
                                        const QPoint & tl)
{
    if (create) {
        delete m_background;				// Create a background
        m_background = 0;				// from tile zero
        if (!background.isNull()) {			// or a QImage.
            m_background = new KGameCanvasPixmap (this);
            m_background->moveTo (tl.x(), tl.y());
            m_background->setPixmap (background);
            m_background->show();
        }
    }
    else {
        m_background->moveTo (tl.x(), tl.y());		// Move an existing bg.
    }
}

void KGrPlayField::setTiles (QList<QPixmap> * tileset, const QPoint & topLeft,
        const int h, const int v, const int tilewidth, const int tileheight)
{
    QPixmap   pm;
    m_tilew = tilewidth;
    m_tileh = tileheight;
    m_numTilesH = h;
    m_numTilesV = v;

    Q_ASSERT (tileset);
    // Clear previously cached tile data.
    while (! m_tilesprites.isEmpty())
        delete m_tilesprites.takeFirst();

    // Now store our tileset as a list of Pixmaps, one for each tile.
    m_tileset = tileset;

    // Create the list of tile sprites in the playfield, arranged as a grid.
    int totaltiles = m_numTilesH * m_numTilesV;
    for (int i=0; i < totaltiles; ++i) {
        KGameCanvasPixmap * thissprite = new KGameCanvasPixmap (this);
        thissprite->moveTo ((i % m_numTilesH) * m_tilew + topLeft.x(),
                           (i / m_numTilesH) * m_tileh + topLeft.y());
        if (m_background == 0) {
            // If no background pixmap, fill the cell with tile zero.
            thissprite->setPixmap (m_tileset->at (0));
            thissprite->show();
        }

        //Finally, store the item in our tilesprite list
        m_tilesprites.append (thissprite);
    }
}
