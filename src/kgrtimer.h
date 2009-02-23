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

#include <QTime>
#include <QTimer>
#include <QList>

class KGrTimer : public QObject
{
    Q_OBJECT
public:
    KGrTimer (QObject * parent, int pTick = 20, float pScale = 1.0);
    ~KGrTimer();

    void pause();
    void resume();
    void step();

signals:
    void tick (bool missed, int pScaledTime);

private slots:
    void internalSlot();

private:
    QTime    t;
    QTimer * ticker;
    int      tickTime;
    int      scaledTime;
    int      tickCount;
    int      halfTick;
    int      expectedTime;
};

#endif // KGRTIMER_H
