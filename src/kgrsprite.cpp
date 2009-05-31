/***************************************************************************
 *                       kgrsprite.cpp  -  description                     *
 *                           -------------------                           *
 *  begin                : Fri Aug 04 2006                                 *
 *  Copyright 2006 Mauricio Piacentini <mauricio@tabuleiro.com>            *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <KDebug>

#include "kgrsprite.h"

KGrSprite::KGrSprite (KGameCanvasAbstract * canvas, const char type,
                      const int tickTime)
    :
    KGameCanvasPixmap (canvas),

    m_frame           (-1),	// Make move() change pixmap if first frame = 0.
    m_frameOffset     (0),	// No offset at first (e.g. carrying no gold).
    m_loc             (-1, -1),	// Make move() change pos if first pos = (0,0).
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
    m_dt              (0)
{
}

KGrSprite::~KGrSprite()
{
}

void KGrSprite::addFrames (QList<QPixmap> * frames, const QPoint & topLeft,
                                const double scale)
{
    m_frames = frames;
    m_scale = scale;
    m_tlX = topLeft.x();
    m_tlY = topLeft.y();
}

void KGrSprite::move (double x, double y, int frame)
{
    // Adjust the frame-number if the sprite is an enemy carrying gold and the
    // caller is not already using an adjusted frame number.  The value of
    // m_frameOffset is either 0 or the number of the first gold-carrying frame.

    int adjustedFrame = (frame < m_frameOffset) ? frame + m_frameOffset : frame;
    if (m_frame != adjustedFrame) {
        m_frame = adjustedFrame;
        setPixmap (m_frames->at (m_frame));
    }
    if ((m_loc.x() != x) || (m_loc.y() != y)) {
        m_loc.setX ((int)x);
        m_loc.setY ((int)y);
        moveTo ((int)(x * m_scale) + m_tlX, (int)(y * m_scale) + m_tlY);
    }
}

void KGrSprite::setZ (qreal /* z (unused) */)
{
    // Hero and enemy sprites are above other elements.
    raise();
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
    // kDebug() << "m_ticks" << m_ticks << "dx,dy,dt" << dx << dy << dt << "m_dx,m_dy" << m_dx << m_dy << "m_frameTicks" << m_frameTicks;
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
    // kDebug() << m_frameCtr << "=" << m_x << m_y << "frame" << m_startFrame + m_frameCtr << m_frameChange;

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
