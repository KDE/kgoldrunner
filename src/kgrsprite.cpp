/***************************************************************************
                         kgrsprite.cpp  -  description
                             -------------------
    begin                : Fri Aug 04 2006
    Copyright 2006 Mauricio Piacentini <mauricio@tabuleiro.com>
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

KGrSprite::KGrSprite( KGameCanvasAbstract* canvas  )
    : KGameCanvasPixmap(canvas)
{
    m_frame = 0;
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

void KGrSprite::move(double x, double y, int frame)
{
    if (m_frame!=frame) {
        m_frame = frame;
        setPixmap(m_frames->at(m_frame));
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
