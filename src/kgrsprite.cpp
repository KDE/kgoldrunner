/***************************************************************************
                         kgrsprite.cpp  -  description
                             -------------------
    begin                : Fri Aug 04 2006
    Copyright 2006 Mauricio Piacentini
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
#include <QImage>

KGrSprite::KGrSprite( KGameCanvasAbstract* canvas  )
    : KGameCanvasPixmap(canvas)
{
    m_frames = new QList<QPixmap> ();
    m_frame = 0;
}

KGrSprite::~KGrSprite()
{
    m_frames->clear();
    delete m_frames;
}

void KGrSprite::addFrames ( const QPixmap& p, int tilewidth, int tileheight, int numframes, double scale )
{
    QPixmap   pm;
    QImage image = p.toImage ();
    m_scale = scale;
    for (int i = 0; i < numframes; i++) {
        pm = QPixmap::fromImage (image.copy
			(i * tilewidth, 0, tilewidth, tileheight));
        m_frames->append (pm.scaledToHeight
			((int)(tileheight * scale), Qt::FastTransformation));
    }
}

void KGrSprite::move(double x, double y, int frame)
{
    if (m_frame!=frame) {
        m_frame = frame;
        setPixmap(m_frames->at(m_frame));
    }
    if ((m_loc.x()!=x) || (m_loc.y()!=y)) {
        m_loc.setX ((int)x);
        m_loc.setY ((int)y);
        moveTo ((int)(x * m_scale), (int)(y * m_scale));
    }
}

void KGrSprite::setZ (qreal /* z (unused) */)
{
    // Hero and enemy sprites are above other elements.
    raise();
}
