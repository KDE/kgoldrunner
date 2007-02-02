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

#include <kgamecanvas.h>

#include <QObject>
#include <QPixmap>
#include <QList>
#include <QPainter>

#include <KSvgRenderer>

class KGrPlayField : public KGameCanvasGroup
{
public:
    explicit KGrPlayField ( KGameCanvasAbstract* canvas = NULL );
    ~KGrPlayField();
    void setTile( int x, int y, int tilenum );
    void setTiles (const QImage * background, const QImage & image,
				int h, int v, int tilewidth, int tileheight);
    int  tile( int x, int y ) const;
    inline void setScale (double scale){ m_scale=scale;};
    inline double scale(){ return m_scale;};

private:
    QList<KGameCanvasPixmap *> m_tilesprites;
    QList<QPixmap> m_tileset;
    QList<int> m_tilenumbers;
    int m_tilew;
    int m_tileh;
    int m_numTilesH;
    int m_numTilesV;
    double m_scale;
    KGameCanvasPixmap * backdrop;
};

#endif // KGRPLAYFIELD_H
