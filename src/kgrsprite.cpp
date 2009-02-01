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

#include "kgrsprite.h"

KGrSprite::KGrSprite (KGameCanvasAbstract* canvas, const char type)
    :
    KGameCanvasPixmap (canvas),

    m_frameOffset     (0),	// No offset at first (e.g. carrying no gold).
    m_type            (type),
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
    m_frame   = -1;		// Makes move() work OK if first frame is 0.
    m_loc.setX (-1);		// Makes move() work OK if first (x,y) is (0,0).
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
    int adjustedFrame = frame + m_frameOffset;	// e.g. Enemy carrying gold.
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

void KGrSprite::setAnimation (int x, int y, int startFrame, int nFrames,
                              int dx, int dy, int dt)
{
    m_stationary = false;	// Animation is ON now.
    m_x          = x;
    m_y          = y;
    m_startFrame = startFrame;
    m_nFrames    = nFrames;
    m_frameCtr   = 0;
    m_dx         = dx;
    m_dy         = dy;
    m_dt         = dt;
}

void KGrSprite::animate()
{
    if (m_stationary) {
        return;
    }
    if (m_frameCtr >= m_nFrames) {
        m_frameCtr = 0;
    }
    move (m_x, m_y, m_startFrame + m_frameCtr);
    m_frameCtr++;
    int STEP = 4;
    m_x = m_x + STEP * m_dx;
    m_y = m_y + STEP * m_dy;
}
