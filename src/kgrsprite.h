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

#include <kgamecanvas.h>

#include <QPixmap>
#include <QList>


class KGrSprite : public KGameCanvasPixmap
{
public:
    explicit KGrSprite ( KGameCanvasAbstract* canvas = NULL);
    ~KGrSprite();
    void move(double x, double y, int frame);
    void setZ ( qreal z );
    void addFrames (QList<QPixmap> * frames, const QPoint & topLeft,
						const double scale);
    inline QPoint currentLoc() { return m_loc; }
    inline void clearFrames() { if (m_frames) m_frames->clear();}
    inline int currentFrame(){ return m_frame;}
    inline void setScale (double scale){ m_scale=scale;}
    inline double scale(){ return m_scale;}

private:
    QList<QPixmap> * m_frames;
    double    m_scale;
    int m_frame;
    QPoint m_loc;		// Location relative to top-left of playfield.
    int m_tlX;			// X co-ordinate of top-left.
    int m_tlY;			// Y co-ordinate of top-left.
};

#endif // KGRSPRITE_H
