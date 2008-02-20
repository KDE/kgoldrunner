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

#ifndef KGRSOUNDEFFECTS_H
#define KGRSOUNDEFFECTS_H

#include <Phonon/MediaObject>
#include <Phonon/MediaSource>

class KGrSoundEffectManager
{
public:

    KGrSoundEffectManager();

    /**
     * play a sound effect. 
     * This method returns a token that can be used to stop the sound if it is
     * still playing. Trying to stop a completed sound has no effect.
     */
    int play (int effect, bool looping = false);

    /**
     * Stop playing the sound associated with the given token 
     */
    void stop (int token);

    /** 
     * load a sound sample.
     * a token is returned to use to play back the sample.
     */
    int loadSound (const QString &fileName);

    /** 
     * discard the loaded sound effects.
     */
    void reset();

    /**
     * Stop all sounds currently playing
     */
    void stopAllSounds();
   
private:
    QVector< Phonon::MediaSource > soundSamples;
    QMap <int, Phonon::MediaObject *> playingSounds;
    int playingToken;
};

#endif // KGRSOUNDEFFECTS_H
// vi: set sw=4 cino=\:0g0 :
