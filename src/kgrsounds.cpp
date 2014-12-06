/****************************************************************************
 *    Copyright 2011  Ian Wadham <iandw.au at gmail dot com>
 *    Copyright 2007 Luciano Montanaro <mikelima@cirulla.net>               *
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

#include "kgrsounds.h"

#include <QDebug>

KGrSounds::KGrSounds() : 
    QObject(),
    sounds()
{
    t.start();
}

KGrSounds::~KGrSounds()
{
}

int KGrSounds::loadSound (const QString &fileName)
{
    //qDebug() << "Loading sound" << fileName;
    sounds << (new KgSound (fileName));
    startTime << 0;
    return sounds.count() - 1;
}

void KGrSounds::setTimedSound (int i)
{
    startTime[i] = 1;
}

void KGrSounds::stopAllSounds()
{
    for (int i = 0; i < sounds.count(); i++) {
	sounds[i]->stop();
    }
}

void KGrSounds::reset()
{
    stopAllSounds();
    sounds.clear();
}

int KGrSounds::play (int effect)
{
    if (muted) return -1;

    // Delete all previously playing instances of this sound, but allow gold and
    // dig sounds to play for > 1 sec, so that rapid sequences of digging or
    // gold collection will have properly overlapping sounds.

    int  started    = startTime[effect];
    bool timedSound = (started != 0);
    int  current    = timedSound ? t.elapsed() : 0;

    if ((! timedSound) || ((current - started) > 1000)) {
        sounds[effect]->stop();
    }

    sounds[effect]->start();

    if (timedSound) {
        startTime[effect] = current;
    }
    return effect;
}

void KGrSounds::stop (int effect)
{
    if (muted) return;

    sounds[effect]->stop();
}

void KGrSounds::setMuted (bool mute)
{
    muted = mute;
    if (mute) {
	stopAllSounds();
    }
}

void KGrSounds::setVolume (int effect, qreal volume)
{
    sounds[effect]->setVolume (volume);
}


