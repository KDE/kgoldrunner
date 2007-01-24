/***************************************************************************
                       kgrplayfield.cpp  -  description
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

#include <QtDebug>
#include "kgrplayfield.h"

KGrPlayField::KGrPlayField( KGameCanvasAbstract* canvas )
    : KGameCanvasGroup(canvas)
{
    backdrop = 0;
    show();
}

KGrPlayField::~KGrPlayField()
{
    //Clear all stored data
    while (!m_tilesprites.isEmpty())
            delete m_tilesprites.takeFirst();
    m_tileset.clear();
    m_tilenumbers.clear();
    delete backdrop;
}

void KGrPlayField::setTile( int x, int y, int tilenum )
{
    //Cache tilenum (for tile(x,y)
    m_tilenumbers[y*m_numTilesH + x] = tilenum;
    //Update the sprite pixmap using our tileset cache
    // IDW inserted
    if ((backdrop != 0) && (tilenum == 0)) {
	m_tilesprites.at(y*m_numTilesH + x)->hide();
    }
    else {
	m_tilesprites.at(y*m_numTilesH + x)->setPixmap(m_tileset.at(tilenum));
	m_tilesprites.at(y*m_numTilesH + x)->show();
	m_tilesprites.at(y*m_numTilesH + x)->raise();
    }
    // IDW deleted
    // m_tilesprites.at(y*m_numTilesH + x)->setPixmap(m_tileset.at(tilenum));
}

void KGrPlayField::setTiles (const QImage * background, const QImage & image,
				int h, int v, int tilewidth, int tileheight)
{
    qDebug () << "setTiles() called";
    QPixmap   pm;
    m_tilew = tilewidth; // (int)(tilewidth  * scale);
    m_tileh = tileheight; // (int)(tileheight * scale);
    m_numTilesH = h;
    m_numTilesV = v;

    qDebug () << "deleting backdrop";
    delete backdrop;
    backdrop = 0;
    qDebug () << "backdrop deleted";
    if (background != 0) {
	qDebug () << "Set backdrop";
	backdrop = new KGameCanvasPixmap (this);
	backdrop->moveTo (0, 0);
	backdrop->setPixmap (QPixmap::fromImage (*background));
	backdrop->show();
	qDebug () << "Backdrop OK";
    }

    //Find out how many tiles in our tileset pixmap
    int tilesetw = image.width();
    int tilecount = tilesetw/tilewidth;
    qDebug() << "Tile count:" << tilecount;

    //Clear previously cached tile data
    while (!m_tilesprites.isEmpty())
            delete m_tilesprites.takeFirst();
    m_tileset.clear();
    m_tilenumbers.clear();
    qDebug() << "Previous tile set deleted OK ...";

    //Now store our tileset as a list of Pixmaps, one for each tile
    for(int i=0;i<tilecount;++i)
    {
        pm = QPixmap::fromImage (image.copy (i * tilewidth, 0,
					tilewidth, tileheight));
	m_tileset.append (pm);
    }
    qDebug() << "Tile set created OK ...";

    //Create the list of tile sprites, arranged as a grid
    int totaltiles = m_numTilesH*m_numTilesV;
    for(int i=0;i<totaltiles;++i)
    {
	KGameCanvasPixmap * thissprite = new KGameCanvasPixmap(this);
	thissprite->moveTo((i % m_numTilesH)*m_tilew, (i/m_numTilesH)*m_tileh);
	if (backdrop == 0) {
	    thissprite->setPixmap(m_tileset.at(0));
	    thissprite->show();
	}

	//Finally, store the item in our tilesprite list
	m_tilesprites.append(thissprite);
	m_tilenumbers.append(0);
    }
    qDebug() << "Tile sprites list set OK ...";
}

int KGrPlayField::tile( int x, int y ) const
{
    if(x >= m_numTilesH || x < 0 || y >= m_numTilesV || y < 0)
        return 0;
    //we stored the tilenumber as custom data in the sprite
    return m_tilenumbers.at(y*m_numTilesH + x);
}
