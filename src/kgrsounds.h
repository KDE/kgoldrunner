/*
    SPDX-FileCopyrightText: 2011 Ian Wadham <iandw.au at gmail dot com>
    SPDX-FileCopyrightText: 2007 Luciano Montanaro <mikelima@cirulla.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRSOUNDS_H
#define KGRSOUNDS_H

#include <QObject>
#include <QVector>
#include <QElapsedTimer>

#include <KgSound>

class KGrSounds : public QObject
{
    Q_OBJECT
public:

    /**
     * Construct the KGrSounds class.
     */
    KGrSounds();
    ~KGrSounds() override;

    /**
     * Play a sound effect. 
     * This method returns a token that can be used to stop the sound if it is
     * still playing. Trying to stop a completed sound has no effect.
     */
    int play (int effect);

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
     * Set up a sound to have its latest start-time recorded.
     */
    void setTimedSound (int i);

    /** 
     * Stop sound and discard the loaded sound effects.
     */
    void reset();

    /**
     * Stop all sounds currently playing.
     */
    void stopAllSounds();

    void setMuted (bool mute);

    /**
     * Change the volume of one type of sound (e.g. footstep) by a given factor.
     * \param effect the effect number.
     * \param volume 0.0 for mute, > 1.0 to increase, < 1.0 to decrease.
     */
    void setVolume (int effect, qreal volume);

private:
    QVector<KgSound *> sounds;
    QVector<int>       startTime;	// Start times of timed sounds, else 0.
    bool               muted;
    QElapsedTimer              t;
};

#endif // KGRSOUNDS_H
