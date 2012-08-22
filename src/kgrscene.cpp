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
#include "kgrsprite.h"
#include "kgrrenderer.h"

const StartFrame animationStartFrames [nAnimationTypes] = {
                 RIGHTWALK1,    LEFTWALK1,  RIGHTCLIMB1,    LEFTCLIMB1,
                 CLIMB1,        CLIMB1,     FALL1,          FALL2,
                 DIGBRICK1,	// Start frame for OPEN_BRICK.
                 DIGBRICK6};	// Start frame for CLOSE_BRICK.

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
    m_heroId            (0),
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
        foreach (KGrSprite * sprite, m_sprites) {
            setTileSize (sprite, tileSize);
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
    int spriteId;
    KGrSprite * sprite = m_renderer->getSpriteItem (type, TickTime);

    if (m_sprites.count(0) > 0 &&
        ((spriteId = m_sprites.lastIndexOf (0)) >= 0)) {
        // Re-use a slot previously occupied by a transient member of the list.
        m_sprites[spriteId] = sprite;
    }
    else {
        // Otherwise, add to the end of the list.
        spriteId = m_sprites.count();
        m_sprites.append (sprite);
    }

    int frame1 = animationStartFrames [FALL_L];

    switch (type) {
    case HERO:
        m_heroId = spriteId;
        sprite->setZ (1);
        break;
    case ENEMY:
        sprite->setZ (2);
        break;
    case BRICK:
        frame1 = animationStartFrames [OPEN_BRICK];

        // The hero and enemies must be painted in front of dug bricks.
        sprite->setZ (0);

        // Erase the brick-image so that animations are visible in all themes.
        paintCell (i, j, FREE);
        break;
    default:
        break;
    }

    sprite->setFrame (frame1);
    setTileSize (sprite, m_tileSize);
    addItem (sprite);		// The sprite can be correctly rendered now.
    sprite->move (i, j, frame1);
    return spriteId;
}

void KGrScene::animate (bool missed)
{
    foreach (KGrSprite * sprite, m_sprites) {
        if (sprite != 0) {
            sprite->animate (missed);
        }
    }
}

void KGrScene::startAnimation (const int id, const bool repeating,
                                const int i, const int j, const int time,
                                const Direction dirn, const AnimationType type)
{
    // TODO - Put most of this in helper code, based on theme parameters.
    int dx              = 0;
    int dy              = 0;
    int frame           = animationStartFrames [type];
    int nFrames         = 8;
    int nFrameChanges   = 4;

    switch (dirn) {
    case RIGHT:
        dx    = +1;
        break;
    case LEFT:
        dx    = -1;
        break;
    case DOWN:
        dy = +1;
        if ((type == FALL_R) || (type == FALL_L)) {
            nFrames = 1;
        }
        else {
            nFrames = 2;
        }
        break;
    case UP:
        dy = -1;
        nFrames = 2;
        break;
    case STAND:
        switch (type) {
        case OPEN_BRICK:
            nFrames = 5;
            break;
        case CLOSE_BRICK:
            nFrames = 4;
            break;
        default:
            // Show a standing hero or enemy, using the previous StartFrame.
            nFrames = 0; 
            break;
        }
        break;
    default:
        break;
    }

    // TODO - Generalise nFrameChanges = 4, also the tick time = 20 new sprite.
    m_sprites.at(id)->setAnimation (repeating, i, j, frame, nFrames, dx, dy,
                                    time, nFrameChanges);
}

void KGrScene::gotGold (const int spriteId, const int i, const int j,
                        const bool spriteHasGold, const bool lost)
{
    // Hide collected gold or show dropped gold, but not if the gold was lost.
    if (! lost) {
        paintCell (i, j, (spriteHasGold) ? FREE : NUGGET);
    }

    // If the rules allow, show whether or not an enemy sprite is carrying gold.
    if (enemiesShowGold && (m_sprites.at(spriteId)->spriteType() == ENEMY)) {
        m_sprites.at(spriteId)->setSpriteKey (spriteHasGold ? "gold_enemy"
                                                            : "enemy");
    }
}

void KGrScene::showHiddenLadders (const QList<int> & ladders, const int width)
{
    foreach (int offset, ladders) {
        int i = offset % width;
        int j = offset / width;
        paintCell (i, j, LADDER);
    }
}

void KGrScene::deleteSprite (const int spriteId)
{
    QPointF loc     = m_sprites.at(spriteId)->currentLoc();
    bool   brick    = (m_sprites.at(spriteId)->spriteType() == BRICK);

    delete m_sprites.at(spriteId);
    m_sprites [spriteId] = 0;

    if (brick) {
        // Dug-brick sprite erased: restore the tile that was at that location.
        paintCell (loc.x(), loc.y(), BRICK);
    }
}

void KGrScene::deleteAllSprites()
{
    qDeleteAll(m_sprites);
    m_sprites.clear();
}

#include "kgrscene.moc"
