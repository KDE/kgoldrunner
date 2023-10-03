/*
    SPDX-FileCopyrightText: 2011 Ian Wadham <iandw.au at gmail dot com>
    SPDX-FileCopyrightText: 2007 Luciano Montanaro <mikelima@cirulla.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kgrsounds.h"

#include "kgoldrunner_debug.h"

KGrSounds::KGrSounds() : 
    QObject(),
    sounds()
{
    t.start();
}

KGrSounds::~KGrSounds()
{
    qDeleteAll(sounds);
}

int KGrSounds::loadSound (const QString &fileName)
{
    //qCDebug(KGOLDRUNNER_LOG) << "Loading sound" << fileName;
    sounds << (new KGameSound (fileName));
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

#include "moc_kgrsounds.cpp"
