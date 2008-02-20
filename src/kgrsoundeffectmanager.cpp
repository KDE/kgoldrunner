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
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "kgrsoundeffectmanager.h"

KGrSoundEffectManager::KGrSoundEffectManager() :
    soundSamples(), playingSounds()
{

}

int KGrSoundEffectManager::loadSound (const QString &fileName)
{
    soundSamples << fileName;
    return soundSamples.count();
}

void KGrSoundEffectManager::stopAllSounds()
{
    foreach (Phonon::MediaObject *o, playingSounds) {
	o->stop();
    }
    playingSounds.clear();
}

void KGrSoundEffectManager::reset()
{
    stopAllSounds();
    soundSamples.clear();
}


// vi: set sw=4 cino=\:0g0 :

