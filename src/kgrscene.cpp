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

#include "kgrscene.h"

KGrScene::KGrScene( QObject *parent )
    : QGraphicsScene(parent)
{
}

KGrScene::~KGrScene()
{
    //Clear all stored data
    while (!m_tilesprites.isEmpty())
            delete m_tilesprites.takeFirst();
    m_tileset.clear();
}

void KGrScene::setTile( int x, int y, int tilenum )
{
    //Cache tilenum (for tile(x,y) as QGraphicsItem Data
    m_tilesprites.at(y*m_numTilesH + x)->setData(0,tilenum);
    //Update the sprite pixmap using our tileset cache
    m_tilesprites.at(y*m_numTilesH + x)->setPixmap(m_tileset.at(tilenum));
}

void KGrScene::setTiles( const QPixmap& p, int h, int v, int tilewidth, int tileheight )
{
    QPixmap   pm;
    m_tilew = tilewidth;
    m_tileh = tileheight;
    m_numTilesH = h;
    m_numTilesV = v;

    //Find out how many tiles in our tileset pixmap
    int tilesetw = p.width();
    int tilecount = tilesetw/tilewidth;

    //Clear previously cached tile data
    while (!m_tilesprites.isEmpty())
            delete m_tilesprites.takeFirst();
    m_tileset.clear();

    //Now store our tileset as a list of Pixmaps, one for each tile
    QImage image = p.toImage ();
    for(int i=0;i<tilecount;++i)
    {
        QImage image = p.toImage ();
        pm = QPixmap::fromImage (image.copy (i * tilewidth, 0, tilewidth, tileheight));
	m_tileset.append(pm);
    }

    //Create the list of tile sprites, arranged as a grid
    int totaltiles = m_numTilesH*m_numTilesV;
    for(int i=0;i<totaltiles;++i)
    {
	QGraphicsPixmapItem * thissprite = new QGraphicsPixmapItem(0, this);
	thissprite->setPos((i % m_numTilesH)*m_tilew, (i/m_numTilesH)*m_tileh);
	thissprite->setPixmap(m_tileset.at(0));

	//Tilenumber is stored in the item data cache for quick lookup (for tile(x,y))
	thissprite->setData(0,0);

	//Finally, store the item in our tilesprite list
	m_tilesprites.append(thissprite);
    }
}

int KGrScene::tile( int x, int y ) const
{
    if(x >= m_numTilesH || x < 0 || y >= m_numTilesV || y < 0)
        return 0;
    //we stored the tilenumber as custom data in the sprite
    return m_tilesprites.at(y*m_numTilesH + x)->data(0).toInt();
}

void KGrScene::mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent )
{
    //KGrCanvas original code caught this directly, as mouse events
    //were previously sent to QCanvasView. For simplicity we simply
    //emit a signal that will be caught by the original implementation
    emit mouseClick(mouseEvent->button());
}

void KGrScene::mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent )
{
    //KGrCanvas original code caught this directly, as mouse events
    //were previously sent to QCanvasView. For simplicity we simply
    //emit a signal that will be caught by the original implementation
    emit mouseLetGo(mouseEvent->button());
}

void KGrScene::wheelEvent ( QGraphicsSceneWheelEvent * wheelEvent )
{
    // This method should prevent wheel events from reaching the QGraphicsScene and moving it
}

void KGrScene::keyPressEvent ( QKeyEvent * keyEvent ) 
{
    // This method prevents keys from reaching the QGraphicsScene and moving it
}

void KGrScene::keyReleaseEvent ( QKeyEvent * keyEvent ) 
{

    // This method prevents keys from reaching the QGraphicsScene and moving it
}

#include "kgrscene.moc"

