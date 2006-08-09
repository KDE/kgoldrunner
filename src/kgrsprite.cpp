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

KGrSprite::KGrSprite( QGraphicsItem * parent, QGraphicsScene * scene )
    : QGraphicsPixmapItem(parent,scene)
{
    m_frames = new QList<QPixmap> ();
}

KGrSprite::~KGrSprite()
{
    m_frames->clear();
    delete m_frames;
}

void KGrSprite::addFrames ( const QPixmap& p, int tilewidth, int tileheight, int numframes )
{
    QPixmap   pm;
    QImage image = p.toImage ();
    for (int i = 0; i < numframes; i++) {
	pm = QPixmap::fromImage (image.copy (i * tilewidth, 0, tilewidth, tileheight));
	m_frames->append (pm);
    }
}

void KGrSprite::move(double x, double y, int frame)
{
    setPixmap(m_frames->at(frame));
    setPos(x,y);
}

void KGrSprite::setZ ( qreal z )
{
    setZValue ( z );
}

#include "kgrsprite.moc"
