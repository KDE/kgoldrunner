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

#include <QPixmap>
#include <QList>

/**
 * The KGrPlayField is a KGameCanvasGroup managing the graphical representation
 * of the playfield.
 */
class KGrPlayField : public KGameCanvasGroup
{
public:
    /**
     * Constructor
     */
    explicit KGrPlayField (KGameCanvasAbstract* canvas = NULL);

    /**
     * Destructor
     */
    ~KGrPlayField();

    /** Set the tile at grid position (x, y) to be tilenum */
    void setTile (int x, int y, int tilenum);
    
    /** Set the background for the playground */
    void setBackground (const bool create, const QPixmap &background,
				const QPoint & tl);
    
    /** 
     * Set the tileset to use for the playfield and clear it to the background. 
     * \param[in] topLeft The origin relative to the canvas.
     * \param[in] h Playground height, in number of tiles
     * \param[in] v Playground width, in number of tiles
     * \param[in] tilewidth The width of a single tile.
     * \param[in] tileheight The height of a single tile.
     * \param tileset List of tiles.
     **/
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
