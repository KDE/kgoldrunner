/****************************************************************************
 *    Copyright 2006  Mauricio Piacentini <mauricio@tabuleiro.com>          *
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                       *
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

#include "kgrsprite.h"
#include "kgrrenderer.h"

#include <QDebug>

KGrSprite::KGrSprite (KGameRenderer * renderer, QString & key,
                      const char type, const int tickTime)
    :
    KGameRenderedItem (renderer, key),

    m_type            (type),
    m_tickTime        (tickTime),
    m_stationary      (true),	// Animation is OFF at first.
    m_x               (0),
    m_y               (0),
    m_startFrame      (0),
    m_nFrames         (1),
    m_frameCtr        (0),
    m_dx              (0),
    m_dy              (0),
    m_dt              (0),
    m_oldX            (-1),
    m_oldY            (-1),
    m_oldFrame        (-1),
    m_topLeftX        (0),
    m_topLeftY        (0),
    m_tileSize        (1)
{
}

KGrSprite::~KGrSprite()
{
}

void KGrSprite::move (double x, double y, int frame)
{
    if (frame != m_oldFrame) {
        // Change the animation frame in KGameRenderedItem.
        setFrame (frame);
        m_oldFrame = frame;
    }
    if ((x != m_oldX) || (y != m_oldY)) {
        // Change the position in scene (and view) coordinates.
        setPos (m_topLeftX + (x + 1) * m_tileSize,
                m_topLeftY + (y + 1) * m_tileSize);
        m_oldX = x;
        m_oldY = y;
    }
    return;
}

void KGrSprite::setAnimation (bool repeating, int x, int y, int startFrame,
                        int nFrames, int dx, int dy, int dt, int nFrameChanges)
{
    m_stationary    = false;	// Animation is ON now.
    m_repeating     = repeating;
    m_x             = x;
    m_y             = y;
    m_startFrame    = startFrame;
    m_nFrames       = nFrames;
    m_frameCtr      = 0;
    m_dt            = dt;
    m_nFrameChanges = nFrameChanges;

    m_ticks         = ((double) dt / m_tickTime) + 0.5;
    m_dx            = (double) dx / m_ticks;
    m_dy            = (double) dy / m_ticks;
    m_frameTicks    = (double) m_ticks / nFrameChanges;
    m_frameChange   = 0.0;
    // qDebug() << "m_ticks" << m_ticks << "dx,dy,dt" << dx << dy << dt << "m_dx,m_dy" << m_dx << m_dy << "m_frameTicks" << m_frameTicks << "nFrames" << nFrames << "nFrameChanges" << nFrameChanges;
}

void KGrSprite::animate (bool missed)
{
    if (m_stationary) {
        return;
    }
    if (m_frameCtr >= m_nFrames) {
        m_frameCtr = 0;
        if (! m_repeating) {
            m_stationary = true;	// Stop after one set of frames.
            return;
        }
    }
    // qDebug() << missed << m_frameCtr << "=" << m_x << m_y << "frame" << m_startFrame + m_frameCtr << m_frameChange;

    // If the clock is running slow, skip an animation step.
    if (! missed) {
        move (m_x, m_y, m_startFrame + m_frameCtr);
    }

    // Calculate the next animation step.
    m_frameChange = m_frameChange + 1.0;
    if (m_frameChange + 0.001 > m_frameTicks) {
        m_frameChange = m_frameChange - m_frameTicks;
        m_frameCtr++;
    }
    m_x = m_x + m_dx;
    m_y = m_y + m_dy;
}

void KGrSprite::setCoordinateSystem (int topLeftX, int topLeftY, int tileSize)
{
    if (tileSize != m_tileSize) {
        setRenderSize (QSize (tileSize, tileSize));
    }
    m_tileSize = tileSize;
    m_topLeftX = topLeftX;
    m_topLeftY = topLeftY;
}

void KGrSprite::changeCoordinateSystem(int topLeftX, int topLeftY, int tileSize)
{
    setCoordinateSystem (topLeftX, topLeftY, tileSize);

    double x = m_oldX;
    double y = m_oldY;
    m_oldX = m_oldY = -1;	// Make it look like a change of position,
    move (x, y, m_oldFrame);	// to force a recalculation of view coords.
}
