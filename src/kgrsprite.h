/***************************************************************************
                         kgrsprite.h  -  description
                             -------------------
    begin                : Fri Aug 04 2006
    Copyright 2006 Mauricio Piacentini <mauricio@tabuleiro.com>
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
    explicit KGrSprite
              (KGameCanvasAbstract* canvas = NULL, const char type = ' ',
               const int tickTime = 20);
    ~KGrSprite();

    void move (double x, double y, int frame);
    void setZ (qreal z);
    void addFrames (QList<QPixmap> * frames, const QPoint & topLeft,
                                                const double scale);
    inline char spriteType() { return m_type;}
    inline QPoint currentLoc() { return m_loc; }
    inline void clearFrames() { if (m_frames) m_frames->clear();}
    inline int currentFrame(){ return m_frame;}
    inline void setScale (double scale){ m_scale=scale;}
    inline double scale(){ return m_scale;}
    inline void setFrameOffset (int offset) { m_frameOffset = offset;}

    void setAnimation (int x, int y, int startFrame, int nFrames,
                       int dx, int dy, int dt, int nFrameChanges);
    void animate();

private:
    QList<QPixmap> * m_frames;
    double m_scale;
    int    m_frame;
    int    m_frameOffset;	// For extra animation frames (e.g. gold enemy).

    QPoint m_loc;		// Location relative to top-left of playfield.
    int    m_tlX;		// X co-ordinate of top-left.
    int    m_tlY;		// Y co-ordinate of top-left.

    char   m_type;
    char   m_tickTime;
    bool   m_stationary;
    double m_x;
    double m_y;
    int    m_startFrame;
    int    m_nFrames;
    int    m_frameCtr;
    double m_dx;
    double m_dy;
    int    m_dt;
    int    m_nFrameChanges;
    int    m_ticks;
    double m_frameTicks;
    double m_frameChange;
};

#endif // KGRSPRITE_H
