/***************************************************************************
                         kgrscene.cpp  -  description
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

#include "kgrplayfield.h"

KGrPlayField::KGrPlayField( KGameCanvasAbstract* canvas )
    : KGameCanvasGroup(canvas)
{
	show();
}

KGrPlayField::~KGrPlayField()
{
    //Clear all stored data
    while (!m_tilesprites.isEmpty())
            delete m_tilesprites.takeFirst();
    m_tileset.clear();
    m_tilenumbers.clear();
}

void KGrPlayField::setTile( int x, int y, int tilenum )
{
    //Cache tilenum (for tile(x,y)
    m_tilenumbers[y*m_numTilesH + x] = tilenum;
    //Update the sprite pixmap using our tileset cache
    m_tilesprites.at(y*m_numTilesH + x)->setPixmap(m_tileset.at(tilenum));
}

void KGrPlayField::setTiles( const QPixmap& p, int h, int v, int tilewidth, int tileheight, double scale )
{
    QPixmap   pm;
    m_tilew = tilewidth*scale;
    m_tileh = tileheight*scale;
    m_numTilesH = h;
    m_numTilesV = v;

    //Find out how many tiles in our tileset pixmap
    int tilesetw = p.width();
    int tilecount = tilesetw/tilewidth;

    //Clear previously cached tile data
    while (!m_tilesprites.isEmpty())
            delete m_tilesprites.takeFirst();
    m_tileset.clear();
    m_tilenumbers.clear();

    //Now store our tileset as a list of Pixmaps, one for each tile
    QImage image = p.toImage ();
    for(int i=0;i<tilecount;++i)
    {
        QImage image = p.toImage ();
        pm = QPixmap::fromImage (image.copy (i * tilewidth, 0, tilewidth, tileheight));
	m_tileset.append(pm.scaledToHeight ( m_tileh, Qt::FastTransformation ));
    }

    //Create the list of tile sprites, arranged as a grid
    int totaltiles = m_numTilesH*m_numTilesV;
    for(int i=0;i<totaltiles;++i)
    {
	KGrGameCanvasPixmap * thissprite = new KGameCanvasPixmap(this);
	thissprite->moveTo((i % m_numTilesH)*m_tilew, (i/m_numTilesH)*m_tileh);
	thissprite->setPixmap(m_tileset.at(0));
	thissprite->show();

	//Finally, store the item in our tilesprite list
	m_tilesprites.append(thissprite);
	m_tilenumbers.append(0);
    }
}

int KGrPlayField::tile( int x, int y ) const
{
    if(x >= m_numTilesH || x < 0 || y >= m_numTilesV || y < 0)
        return 0;
    //we stored the tilenumber as custom data in the sprite
    return m_tilenumbers.at(y*m_numTilesH + x);
}


#include "kgrplayfield.moc"

