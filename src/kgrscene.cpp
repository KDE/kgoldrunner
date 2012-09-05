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
#include <KLocalizedString>

#include <QFont>

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
    m_level             (1),
    m_title             (0),
    m_livesText         (0),
    m_scoreText         (0),
    m_hasHintText       (0),
    m_pauseResumeText   (0),
    m_scoreDisplay      (0),
    m_livesDisplay      (0),
    m_heroId            (0),
    m_tilesWide         (FIELDWIDTH  + 2 * 2),
    m_tilesHigh         (FIELDHEIGHT + 2 * 2),
    m_tileSize          (10),
    m_themeChanged      (true),
    m_topLeftX          (0),
    m_topLeftY          (0),
    m_mouse             (new QCursor())
{
    setItemIndexMethod(NoIndex);

    m_tiles.fill        (0,     m_tilesWide * m_tilesHigh);
    m_tileTypes.fill    (FREE,  m_tilesWide * m_tilesHigh);

    m_renderer  = new KGrRenderer (this);

    m_frame = addRect (0, 0, 100, 100);		// Create placeholder for frame.
    m_frame->setVisible (false);

    m_title = new QGraphicsSimpleTextItem();
    addItem (m_title);

    m_livesText = new QGraphicsSimpleTextItem();
    addItem (m_livesText);

    m_scoreText = new QGraphicsSimpleTextItem();
    addItem (m_scoreText);

    m_hasHintText = new QGraphicsSimpleTextItem();
    addItem (m_hasHintText);

    m_pauseResumeText = new QGraphicsSimpleTextItem();
    addItem (m_pauseResumeText);
}

KGrScene::~KGrScene()
{
    delete m_mouse;
}

void KGrScene::redrawScene ()
{
    if (m_sizeChanged) {
        // Calculate what size of tile will fit in the view.
        QSize size      = m_view->size();
        int tileSize    = qMin (size.width()  / m_tilesWide,
                                size.height() / m_tilesHigh);
        m_topLeftX   = (size.width()  - m_tilesWide * tileSize)/2.0;
        m_topLeftY   = (size.height() - m_tilesHigh * tileSize)/2.0;
        setSceneRect   (0, 0, size.width(), size.height());

        int index = 0;
        foreach (KGameRenderedItem * tile, m_tiles) {
            if (tile) {
                setTile (tile, tileSize, index/m_tilesHigh, index%m_tilesHigh);
            }
            index++;
        }
        foreach (KGrSprite * sprite, m_sprites) {
	    if (sprite) {
                sprite->changeCoordinateSystem
                        (m_topLeftX, m_topLeftY, tileSize);
            }
        }

        m_tileSize = tileSize;
        m_sizeChanged = false;
    }

    // Re-draw text, background and frame if either scene-size or theme changes.

    // Resize and draw texts for title, score, lives, hasHint and pauseResume.
    setTextFont (m_title, 0.6);
    setTitle (m_title->text());
    placeTextItems();

    // Resize and draw different backgrounds, depending on the level and theme.
    loadBackground (m_level);

    if (m_renderer->hasBorder()) {
        // There are border tiles in the theme, so do not draw a frame.
        m_frame->setVisible (false);
    }
    else {
        // There are no border tiles, so draw a frame around the board.
        setFrame();
    }

    if (m_themeChanged) {
        // Fill the scene (and view) with the new background color.  Do this
        // even if the background has no border, to avoid ugly white rectangles
        // appearing if rendering and painting is momentarily a bit slow.
        setBackgroundBrush (m_renderer->borderColor());

	// Erase border tiles (if any) and draw new ones, if new theme has them.
        drawBorder();

        // Redraw all the tiles, except for borders and tiles of type FREE.
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

void KGrScene::setTitle (const QString & newTitle)
{
    if (! m_title) return;

    m_title->setText (newTitle);
    QRectF r = m_title->boundingRect();		// Centre the title.
    m_title->setPos ((sceneRect().width() - r.width())/2,
                      m_topLeftY + (m_tileSize - r.height())/2);
}

void KGrScene::placeTextItems()
{
    setTextFont (m_livesText, 0.5);
    setTextFont (m_scoreText, 0.5);
    setTextFont (m_hasHintText, 0.5);
    setTextFont (m_pauseResumeText, 0.5);

    qreal totalWidth = 0.0;
    QRectF r = m_livesText->boundingRect();
    qreal x = m_topLeftX + 2 * m_tileSize;
    qreal y = sceneRect().height() - m_topLeftY - (m_tileSize + r.height())/2;

    m_livesText->setPos (x, y);
    totalWidth += r.width();

    r = m_scoreText->boundingRect();
    m_scoreText->setPos (x + totalWidth, y);
    totalWidth += r.width();

    r = m_hasHintText->boundingRect();
    m_hasHintText->setPos (x + totalWidth, y);
    totalWidth += r.width();

    r = m_pauseResumeText->boundingRect();
    m_pauseResumeText->setPos (x + totalWidth, y);
    totalWidth += r.width();

    qreal spacing = ((m_tilesWide - 4) * m_tileSize - totalWidth) / 3.0;
    if (spacing < 0.0)
        return;

    m_scoreText->moveBy (spacing, 0.0);
    m_hasHintText->moveBy (2.0 * spacing, 0.0);
    m_pauseResumeText->moveBy (3.0 * spacing, 0.0);
}

void KGrScene::showLives (long lives)
{
    if (m_livesText)
        m_livesText->setText (i18n("Lives: %1", QString::number(lives)
                                                .rightJustified(3, '0')));
}

void KGrScene::showScore (long score)
{
    if (m_scoreText)
        m_scoreText->setText (i18n("Score: %1", QString::number(score)
                                                .rightJustified(7, '0')));
}

void KGrScene::setHasHintText (const QString & msg)
{
    if (m_hasHintText)
        m_hasHintText->setText (msg);
}

void KGrScene::setPauseResumeText (const QString & msg)
{
    if (m_pauseResumeText)
        m_pauseResumeText->setText (msg);
}

void KGrScene::setLevel (unsigned int level)
{
    m_level = level;
    loadBackground (level);	// Load background for level.
}

void KGrScene::loadBackground (const int level)
{
    // NOTE: The background picture can be the same size as the level-layout (as
    // in the Egypt theme) OR it can be the same size as the entire viewport.
    // In this example the background is fitted into the level-layout.
    m_background = m_renderer->getBackground (level, m_background);

    m_background->setRenderSize (QSize ((m_tilesWide - 4) * m_tileSize,
                                        (m_tilesHigh - 4) * m_tileSize));
    m_background->setPos    (m_topLeftX + 2 * m_tileSize,
                             m_topLeftY + 2 * m_tileSize);
    // Keep the background behind the level layout.
    m_background->setZValue (-1);
}

void KGrScene::setTile (KGameRenderedItem * tile, const int tileSize,
                        const int i, const int j)
{
    tile->setRenderSize (QSize (tileSize, tileSize));
    tile->setPos (m_topLeftX + (i+1) * tileSize, m_topLeftY + (j+1) * tileSize);
}

void KGrScene::setBorderTile (const QString spriteKey, const int x, const int y)
{
    int index               = x * m_tilesHigh + y;
    KGameRenderedItem * t   = m_renderer->getBorderItem (spriteKey,
                                                         m_tiles.at(index));
    m_tiles[index]          = t;

    if (t) {
        setTile (t, m_tileSize, x, y);
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

void KGrScene::setFrame()
{
    int w = 0.05 * m_tileSize + 0.5;
    w = w < 1 ? 1 : w;
    m_frame->setRect (
	m_topLeftX + (2 * m_tileSize) - (3 * w),
	m_topLeftY + (2 * m_tileSize) - (3 * w),
	FIELDWIDTH  * m_tileSize + 6 * w,
	FIELDHEIGHT * m_tileSize + 6 * w);
    kDebug() << "FRAME WIDTH" << w << "tile size" << m_tileSize << "rectangle" << m_frame->rect();
    QPen pen = QPen (m_renderer->textColor());
    pen.setWidth (w);
    m_frame->setPen (pen);
    m_frame->setVisible (true);
}

void KGrScene::paintCell (const int i, const int j, const char type)
{
    int index               = i * m_tilesHigh + j;
    KGameRenderedItem * t   = m_renderer->getTileItem (type, m_tiles.at(index));
    m_tiles[index]          = t;
    m_tileTypes[index]      = type;

    if (t) {
        setTile (t, m_tileSize, i, j);
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
    sprite->setCoordinateSystem (m_topLeftX, m_topLeftY, m_tileSize);
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

void KGrScene::setMousePos (const int i, const int j)
{
    m_mouse->setPos (m_view->mapToGlobal (QPoint (
                     m_topLeftX + (i + 1) * m_tileSize + m_tileSize/2,
                     m_topLeftY + (j + 1) * m_tileSize + m_tileSize/2)));
}

void KGrScene::getMousePos (int & i, int & j)
{
    QPoint pos = m_view->mapFromGlobal (m_mouse->pos());
    i = pos.x();
    j = pos.y();
    if (! m_view->isActiveWindow()) {
        i = -2;
	j = -2;
	return;
    }
    // IDW TODO - Check for being outside scene. Use saved m_width and m_height.

    i = (i - m_topLeftX)/m_tileSize - 1;
    j = (j - m_topLeftY)/m_tileSize - 1;

    // Make sure i and j are within the KGoldrunner playing area.
    i = (i < 1) ? 1 : ((i > FIELDWIDTH)  ? FIELDWIDTH  : i);
    j = (j < 1) ? 1 : ((j > FIELDHEIGHT) ? FIELDHEIGHT : j);
}

void KGrScene::setTextFont (QGraphicsSimpleTextItem * t, double fontFraction)
{
    QFont f;
    f.setPixelSize ((int) (m_tileSize * fontFraction + 0.5));
    f.setWeight (QFont::Bold);
    f.setStretch (QFont::Expanded);
    t->setBrush (m_renderer->textColor());
    t->setFont (f);
}

#include "kgrscene.moc"
