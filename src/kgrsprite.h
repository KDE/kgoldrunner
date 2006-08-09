/***************************************************************************
                         kgrsprite.h  -  description
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

#ifndef KGRSPRITE_H
#define KGRSPRITE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QList>

class KGrSprite : public QGraphicsPixmapItem
{
public:
    KGrSprite ( QGraphicsItem * parent = 0, QGraphicsScene * scene = 0  );
    ~KGrSprite();
    void move(double x, double y, int frame);
    void setZ ( qreal z );
    void addFrames ( const QPixmap& p, int tilewidth, int tileheight, int numframes );

private:
    QList<QPixmap> * m_frames;
};

#endif // KGRSPRITE_H
