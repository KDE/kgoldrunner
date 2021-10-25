/*
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRTIMER_H
#define KGRTIMER_H

#include <QElapsedTimer>
#include <QTimer>

class KGrTimer : public QObject
{
    Q_OBJECT
public:
    explicit KGrTimer (QObject * parent, int pTick = 20, float pScale = 1.0);
    ~KGrTimer() override;

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
