/***************************************************************************
 *   Copyright 2010 Stefan Majewsky <majewsky@gmx.net>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License          *
 *   version 2 as published by the Free Software Foundation                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef TAGARO_AUDIOSCENE_H
#define TAGARO_AUDIOSCENE_H

#include <QtCore/QPointF>

namespace Tagaro {

/**
 * @class Tagaro::AudioScene audioscene.h <Tagaro/AudioScene>
 *
 * This class exposes general properties of the audio playback context. Actual
 * sounds are represented in this context by Tagaro::Sound instances.
 *
 * The audio scene basically consists of a listener. The position of this
 * listener is relevant when sounds are played at certain positions: The audio
 * channels will then be balanced to make the sound appear to come from that
 * direction.
 *
 * Because there can only be one listener, all methods in this class are static.
 */
class AudioScene
{
	public:
		///@return the position of the listener
		static QPointF listenerPos();
		///Sets the position of the listener. The default is (0.0, 0.0), the
		///point of origin.
		static void setListenerPos(const QPointF& pos);
		///@return the master volume for sounds outputted by TagaroAudio
		static qreal volume();
		///Sets the master volume for sounds outputted by TagaroAudio. The
		///default is 1.0, which means no volume change, compared to the
		///original sounds. 0.0 means that all sounds are muted.
		static void setVolume(qreal volume);
	private:
		class Private;
		//prohibit instantiation etc.
		AudioScene();
		Q_DISABLE_COPY(AudioScene)
};

} //namespace Tagaro

#endif // TAGARO_AUDIOSCENE_H
