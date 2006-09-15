/***************************************************************************
                         kgrscene.h  -  description
                             -------------------
    begin                : Fri Aug 04 2006
    Copyright 2006 Mauricio Piacentini
    Copyright 2006 Dmitry Suzdalev
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGRPLAYFIELD_H
#define KGRPLAYFIELD_H

#include "kgrgamecanvas.h"

#include <QObject>
#include <QPixmap>
#include <QList>
#include <QPainter>

class KGrPlayField : public KGrGameCanvasGroup
{
public:
    KGrPlayField ( KGrGameCanvasAbstract* canvas = NULL);
    ~KGrPlayField();
    void setTile( int x, int y, int tilenum );
    void setTiles( const QPixmap& p, int h, int v, int tilewidth, int tileheight, double scale );
    int  tile( int x, int y ) const;
    inline void setScale (double scale){ m_scale=scale;};
    inline double scale(){ return m_scale;};

private:
    QList<KGrGameCanvasPixmap *> m_tilesprites;
    QList<QPixmap> m_tileset;
    QList<int> m_tilenumbers;
    int m_tilew;
    int m_tileh;
    int m_numTilesH;
    int m_numTilesV;
    double m_scale;
};

#endif // KGRPLAYFIELD_H
