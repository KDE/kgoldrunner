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

#ifndef KGRSCENE_H
#define KGRSCENE_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QList>
#include <QPainter>

class KGrScene : public QGraphicsScene
{
    Q_OBJECT
public:
    KGrScene ( QObject* parent = 0 );
    ~KGrScene();
    void setTile( int x, int y, int tilenum );
    void setTiles( const QPixmap& p, int h, int v, int tilewidth, int tileheight );
    int  tile( int x, int y ) const;
signals:
    void mouseClick (int);
    void mouseLetGo (int);

protected:
    void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    void mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    void keyPressEvent ( QKeyEvent * keyEvent );
    void keyReleaseEvent ( QKeyEvent * keyEvent );

private:
    QList<QGraphicsPixmapItem *> m_tilesprites;
    QList<QPixmap> m_tileset;
    int m_tilew;
    int m_tileh;
    int m_numTilesH;
    int m_numTilesV;
};

#endif // KGRSCENE_H
