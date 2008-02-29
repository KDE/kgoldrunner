/*
    Copyright 2007 Luciano Montanaro <mikelima@cirulla.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the 
    Free Software Foundation, Inc., 
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "kgrsoundbank.h"
#include <kdebug.h>

KGrSoundBank::KGrSoundBank (int number) : 
    QObject(),
    soundSamples(), 
    currentToken (0)
{
    for (int i = 0; i < number; i++) {
	channels << Phonon::createPlayer (Phonon::GameCategory);
	tokens << -1;
	connect (channels[i], SIGNAL (finished()), this, SLOT (freeChannels()));
    }
}

KGrSoundBank::~KGrSoundBank()
{
    for(int i = 0; i < channels.count(); i++) {
	delete channels[i];
	tokens[i] = -1;
    }
}

int KGrSoundBank::loadSound (const QString &fileName)
{
    kDebug() << "loading sound" << fileName;
    soundSamples << fileName;
    return soundSamples.count() - 1;
}

void KGrSoundBank::stopAllSounds()
{
    for (int i = 0; i < channels.count(); i++) {
	channels[i]->stop();
	tokens[i] = -1;
    }
}

void KGrSoundBank::reset()
{
    stopAllSounds();
    soundSamples.clear();
}

int KGrSoundBank::play (int effect, bool looping)
{
    if (muted) return -1;

    static int firstFreeChannel = 0;
    // Find a free channel
    int i = firstFreeChannel++;
    firstFreeChannel %= channels.count();
    while (i < tokens.count()) {
	if (tokens[i] == -1) break;
	i++;
    }
    
    // If no channel is found, return
    if (i >= channels.count()) return -1;

    // Else play sound and return its token
    channels[i]->setCurrentSource (soundSamples[effect]);
    channels[i]->play();
    tokens[i] = ++currentToken;
    kDebug() << "Playing sound" << soundSamples[effect].fileName() << 
	"with token" << currentToken << "on channel" << i;
    return currentToken;
}

void KGrSoundBank::freeChannels()
{
    for (int i = 0; i < channels.count(); i++) {
	if (channels[i]->state() == Phonon::StoppedState) {
	    tokens[i] = -1;
	    //kDebug() << "Channel" << i << "is free";
	}
    }
}

void KGrSoundBank::stop (int token)
{
    if (muted) return;

    int i = 0;
    while (i < tokens.count()) {
	if (tokens[i] == token) break;
	i++;
    }

    // The sound with the associated token is not present, it either has
    // stopped or the caller is confused.
    if (i >= channels.count()) {
	kDebug() << "sound with token" << currentToken << "cannot be found";
	return;
    }

    channels[i]->stop();
    tokens[i] = -1;
    kDebug() << "Stopping sound with token" << currentToken << "on channel" << i;
}

void KGrSoundBank::setMuted (bool mute)
{
    muted = mute;
    if (mute) {
	stopAllSounds();
    }
}

// vi: set sw=4 cino=\:0g0 :

