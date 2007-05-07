/***************************************************************************
                         kgrscene.h  -  description
                             -------------------
    begin                : Fri Aug 04 2006
    Copyright 2006 Mauricio Piacentini <mauricio@tabuleiro.com>
    Copyright 2006 Dmitry Suzdalev <dimsuz@gmail.com>
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

class KGrPlayField : public KGameCanvasGroup
{
public:
    explicit KGrPlayField (KGameCanvasAbstract* canvas = NULL);
    ~KGrPlayField();
    void setTile (int x, int y, int tilenum);
    void setBackground (const QImage * background, const QPoint & tl);
    void setTiles (QList<QPixmap> * tileset, const QPoint & topLeft,
	const int h, const int v, const int tilewidth, const int tileheight);

private:
    QList<KGameCanvasPixmap *> m_tilesprites;
    QList<QPixmap> * m_tileset;
    int m_tilew;
    int m_tileh;
    int m_numTilesH;
    int m_numTilesV;
    KGameCanvasPixmap * m_background;
};

#endif // KGRPLAYFIELD_H
