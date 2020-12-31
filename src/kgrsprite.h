/*
    SPDX-FileCopyrightText: 2006 Mauricio Piacentini <mauricio@tabuleiro.com>
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRSPRITE_H
#define KGRSPRITE_H


#include <KGameRenderedItem>


class KGrSprite : public KGameRenderedItem
{
public:
    explicit KGrSprite (KGameRenderer * renderer, QString & key,
                        const char type, const int tickTime = 20);
    ~KGrSprite();

    inline char     spriteType      ()        { return m_type; }
    inline QPointF  currentLoc      ()        { return QPointF (m_x, m_y); }
    inline int      currentFrame    ()        { return frame(); }
    inline void     setZ            (qreal z) { setZValue(z); }

    void move           (double x, double y, int frame);
    void animate        (bool missed);
    void setAnimation   (bool repeating, int x, int y, int startFrame,
                         int nFrames, int dx, int dy, int dt,
                         int nFrameChanges);
    void setCoordinateSystem    (int topLeftX, int topLeftY, int tileSize);
    void changeCoordinateSystem (int topLeftX, int topLeftY, int tileSize);

private:
    char   m_type;
    char   m_tickTime;
    bool   m_stationary;
    bool   m_repeating;
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
    double m_oldX;
    double m_oldY;
    int    m_oldFrame;

    int    m_topLeftX;
    int    m_topLeftY;
    int    m_tileSize;
};

#endif // KGRSPRITE_H
