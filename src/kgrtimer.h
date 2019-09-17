/****************************************************************************
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                         *
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

#ifndef KGRTIMER_H
#define KGRTIMER_H

#include <QElapsedTimer>
#include <QTimer>

class KGrTimer : public QObject
{
    Q_OBJECT
public:
    explicit KGrTimer (QObject * parent, int pTick = 20, float pScale = 1.0);
    ~KGrTimer();

    void pause();
    void resume();
    void step();
    inline void setScale (const float pScale)
                         { scaledTime = (pScale * tickTime) + 0.5; }

Q_SIGNALS:
    /**
     * This signal powers the whole game. KGrLevelPlayer connects it to its
     * tick() slot.
     *
     * @param missed       If true, the QTimer has missed one or more ticks, due
     *                     to overheads elsewhere in Qt or the O/S. The game
     *                     catches up on the missed signal(s) and the graphics
     *                     view avoids painting any sprites until the catchup
     *                     is complete, thus saving further overheads. The
     *                     sprites may "jump" a little when this happens, but
     *                     at least the game stays on-time in wall-clock time.
     * @param pScaledTime  The number of milliseconds per tick. Usually this is
     *                     tickTime (= 20 msec), but it is less when the game is
     *                     slowed down or more when it is speeded up. If the
     *                     scaled time is 10 (beginner speed), the game will
     *                     take 2 ticks of 20 msec (i.e. 40 msec) to do what it
     *                     normally does in 20 msec.
     */
    void tick (bool missed, int pScaledTime);

private Q_SLOTS:
    void internalSlot();

private:
    QElapsedTimer    t;
    QTimer * ticker;
    int      tickTime;
    int      scaledTime;
    int      tickCount;
    int      halfTick;
    int      expectedTime;
};

#endif // KGRTIMER_H
