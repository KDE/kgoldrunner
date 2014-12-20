/****************************************************************************
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

#include "kgrtimer.h"

#include <QDebug>

KGrTimer::KGrTimer (QObject * parent, int pTickTime, float pScale)
    :
    QObject      (parent),
    t            (QTime()),
    ticker       (new QTimer (parent)),
    tickTime     (pTickTime),
    tickCount    (0),
    halfTick     (pTickTime / 2),
    expectedTime (0)
{
    setScale (pScale);
    connect(ticker, &QTimer::timeout, this, &KGrTimer::internalSlot);
    ticker->start (tickTime);
    t.start();
}

KGrTimer::~KGrTimer()
{
    ticker->stop();
}

void KGrTimer::pause()
{
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
    emit tick (false, scaledTime);
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
        emit tick ((timeOnClock >= (expectedTime + tickTime)), scaledTime);
    }
}


