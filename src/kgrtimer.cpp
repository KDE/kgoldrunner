/****************************************************************************
 *    Copyright 2009  Ian Wadham <ianwau@gmail.com>                         *
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

#include "kgrtimer.h"

KGrTimer::KGrTimer (QObject * parent, int pTickTime = 20)
    :
    QObject      (parent),
    t            (QTime()),
    ticker       (new QTimer (parent)),
    tickTime     (pTickTime),
    tickCount    (0),
    halfTick     (pTickTime / 2),
    expectedTime (0)
{
    connect (ticker, SIGNAL (timeout ()), this, SLOT (internalSlot()));
    ticker->start (tickTime);
    t.start();
}

KGrTimer::~KGrTimer()
{
    ticker->stop();
    // TODO - Delete the list-contents.
}

void KGrTimer::start (int id, int interval, int signalNum, float scale)
{
    // TODO - Implement "scale" option for speeding up or slowing down the game.
    int found = -1;
    int n     = -1;
    Timer * timer;

    foreach (timer, timers) {
        n++;
        if ((timer->id == id) && (timer->signalNum == signalNum)) {
            found = n;
        }
        else if ((timer->id < 0) && (found < 0)) {
            found = n;
        }
    }

    if (found < 0) {
        timers.append (new Timer);
        found = timers.count() - 1;
    }

    timer = timers.at (found);
    timer->id         = id;
    timer->signalNum  = signalNum;
    timer->active     = true;
    timer->finishTime = t.elapsed() + interval;
    timer->interval   = interval;
}

void KGrTimer::stop (int id, int signalNum)
{
    foreach (Timer * timer, timers) {
        if ((timer->id == id) && (timer->signalNum == signalNum)) {
            timer->active = false;
        }
    }
}

void KGrTimer::remove (int id, int signalNum)
{
    foreach (Timer * timer, timers) {
        if ((timer->id == id) && (timer->signalNum == signalNum)) {
            timer->id = -1;
        }
    }
}

void KGrTimer::pause()
{
    // TODO - If midway between two ticks, allow for that.
    // TODO - Maybe set a "pausing" flag and do the pause in internalSlot().
    // TODO - Maybe forget this, if we are not going to use emitSignals().
    ticker->stop();
}

void KGrTimer::resume()
{
    ticker->start (tickTime);
    t.start();
    expectedTime = 0;
}

void KGrTimer::step()
{
    tickCount++;
    expectedTime = expectedTime + tickTime;
    emit tick (false);
    emitSignals (expectedTime, expectedTime);
}

void KGrTimer::internalSlot()
{
    // Check whether the QTimer::timeout() signal is on-time.
    int timeOnClock = t.elapsed();

    // If the signal is too early, ignore it.  If it is on-time +/-halfTick,
    // trigger an internal signal.  If it is late, trigger more internal
    // signals, in order to "catch up".
    
    while (timeOnClock > (expectedTime + halfTick)) {
        tickCount++;
        expectedTime = expectedTime + tickTime;
        emit tick (timeOnClock >= (expectedTime + tickTime));
        emitSignals (expectedTime, timeOnClock);
    }
}

void KGrTimer::emitSignals (int internalTime, int timeOnClock)
{
    int testTime = internalTime + halfTick;
    foreach (Timer * timer, timers) {
        if ((timer->id >= 0) && (timer->active)) {
            while (testTime > timer->finishTime) {
                timer->finishTime += timer->interval;
                int  error  = timeOnClock - timer->finishTime;
                bool missed = (error > timer->interval);
                switch (timer->signalNum) {
                case 1:
                    emit signal_1 (timer->id, error, missed);
                    break;
                case 2:
                    emit signal_2 (timer->id, error, missed);
                    break;
                default:
                    break;
                }
            } // while
        } // if active
    } // foreach
}

#include "kgrtimer.moc"
