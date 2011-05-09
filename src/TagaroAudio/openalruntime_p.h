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

#ifndef TAGARO_OPENALRUNTIME_P_H
#define TAGARO_OPENALRUNTIME_P_H

#include <QtCore/QHash>
#include <QtCore/QPointF>
//OpenAL includes (without "AL/" include directory; see FindOpenAL.cmake)
#include <al.h>
#include <alc.h>

namespace Tagaro {

class Sound;

///@internal
class PlaybackEvent
{
	public:
		//Creates and starts the playback. Also registers with the OpenALRuntime.
		PlaybackEvent(Tagaro::Sound* sound, const QPointF& pos);
		//Stops playback if it is still running.
		~PlaybackEvent();

		//Is playback still running?
		bool isRunning() const;
	private:
		ALuint m_source;
		bool m_valid;
};

typedef QList<Tagaro::PlaybackEvent*> PlaybackEventList;

///@internal
class OpenALRuntime
{
	public:
		OpenALRuntime();
		~OpenALRuntime();

		static Tagaro::OpenALRuntime* instance();

		void configureListener();
		void cleanupUnusedSources();

		//global properties
		QPointF m_listenerPos;
		qreal m_volume;
		//active sound and playback instances
		QHash<Tagaro::Sound*, Tagaro::PlaybackEventList> m_soundsEvents;
	private:
		ALCcontext* m_context;
		ALCdevice* m_device;
};

} //namespace Tagaro

#endif // TAGARO_OPENALRUNTIME_P_H
