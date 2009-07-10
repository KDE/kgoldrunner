/****************************************************************************
 *    Copyright 2007  Luciano Montanaro <mikelima@cirulla.net>              *
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

#ifndef KGRSOUNDBANK_H
#define KGRSOUNDBANK_H

#include <Phonon/MediaObject>
#include <Phonon/MediaSource>

class KGrSoundBank : public QObject
{
    Q_OBJECT
public:

    /**
     * Construct the KGrSoundBank class.
     * \param number - how many audio channels are required
     */
    KGrSoundBank(int number);
    ~KGrSoundBank();

    /**
     * Play a sound effect. 
     * This method returns a token that can be used to stop the sound if it is
     * still playing. Trying to stop a completed sound has no effect.
     */
    int play (int effect, bool looping = false);

    /**
     * Stop playing the sound associated with the given token.
     */
    void stop (int token);

    /** 
     * Load a sound sample.
     * A token is returned to use to play back the sample.
     */
    int loadSound (const QString &fileName);

    /** 
     * Stop sound and discard the loaded sound effects.
     */
    void reset();

    /**
     * Stop all sounds currently playing.
     */
    void stopAllSounds();

    /**
     * Set volume for the sound effects.
     * \param volume the playing volume. 0.0 means mute, 1.0 means full volume.
     */
    void setEffectsVolume (double volume);
   
    /**
     * Set volume for the background music.
     * \param volume the playing volume. 0.0 means mute, 1.0 means full volume.
     */
    void setMusicVolume (double volume);
   
    void setMuted (bool mute);

private slots:
    void freeChannels();
private:
    int freeAChannel();

private:
    QVector< Phonon::MediaSource > soundSamples;
    QVector< Phonon::MediaObject * > channels;
    QVector< int > tokens;

    int currentToken;
    bool muted;
};

#endif // KGRSOUNDBANK_H
// vi: set sw=4 cino=\:0g0 :
