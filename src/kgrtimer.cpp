/*
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kgrtimer.h"

#include "kgoldrunner_debug.h"

KGrTimer::KGrTimer (QObject * parent, int pTickTime, float pScale)
    :
    QObject      (parent),
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
    Q_EMIT tick (false, scaledTime);
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
        Q_EMIT tick ((timeOnClock >= (expectedTime + tickTime)), scaledTime);
    }
}

#include "moc_kgrtimer.cpp"
