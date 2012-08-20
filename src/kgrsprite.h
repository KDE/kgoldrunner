/****************************************************************************
 *    Copyright 2006  Mauricio Piacentini <mauricio@tabuleiro.com>          *
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                       *
 *                                                                          *
 *    This program is free software; you can redistribute it and/or         *
 *    modify it under the terms of the GNU General Public License as        *
 *    published by the Free Software Foundation; either version 2 of        *
 *    the License, or (at your option) any later version.                   *
 *                                                                          *
 *    This program is distributed in the hope that it will be useful,       *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *    GNU General Public License for more details.                          *
 *                                                                          *
 *    You should have received a copy of the GNU General Public License     *
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ****************************************************************************/

#ifndef KGRSPRITE_H
#define KGRSPRITE_H

#include <QPixmap>
#include <QList>

#include <KGameRenderedItem>

class KGrRenderer;

class KGrSprite
{
public:
    explicit KGrSprite (KGrRenderer *   renderer,
                        const char      type        = ' ',
                        const int       tickTime    = 20);
    ~KGrSprite();

    inline char     spriteType      ()              { return m_type; }
    inline QPointF  currentLoc      ()              { return m_item->pos(); }
    inline int      currentFrame    ()              { return m_item->frame(); }
    inline void     setZ            (qreal z)       { m_item->setZValue(z); }
    inline void     setFrameOffset  (int offset)    { m_frameOffset = offset; }

    void move           (double x, double y, int frame);
    void animate        (bool missed);
    void setAnimation   (bool repeating, int x, int y, int startFrame,
                         int nFrames, int dx, int dy, int dt,
                         int nFrameChanges);

private:
    int    m_frameOffset;	// For extra animation frames (e.g. gold enemy).

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

    KGrRenderer         * m_renderer;
    KGameRenderedItem   * m_item;
};

#endif // KGRSPRITE_H
