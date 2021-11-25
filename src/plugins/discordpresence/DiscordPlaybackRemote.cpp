//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004-2021 musikcube team
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the author nor the names of other contributors may
//      be used to endorse or promote products derived from this software
//      without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>

#include "DiscordPlaybackRemote.h"
#include "DiscordPresence.h"

static std::unique_ptr<presenceplugin::DiscordPresence> g_Presence;

namespace
{

std::string getStringFromTrack( musik::core::sdk::ITrack* track, const char* key )
{
    static std::vector<char> buf( 2048 );

    const int size{ track->GetString( key, &buf[ 0 ], buf.size() ) };
    if( size < 1 or size > buf.size() )
    {
        return {};
    }

    // -1 to not copy the trailing \0
    return { &buf[ 0 ], static_cast<std::size_t>( size ) - 1 };
}

} // namespace

DiscordPlaybackRemote::DiscordPlaybackRemote()
    : m_PlaybackService{}
{
    std::atexit( []
    {
        g_Presence.reset();
    } );
}

void DiscordPlaybackRemote::Release()
{
    delete this;
}

void DiscordPlaybackRemote::SetPlaybackService( musik::core::sdk::IPlaybackService* srv )
{
    m_PlaybackService = srv;
    if(srv)
    {
        g_Presence = std::make_unique<presenceplugin::DiscordPresence>();
    }
    else
    {
        g_Presence.reset();
    }
}

void DiscordPlaybackRemote::OnTrackChanged( musik::core::sdk::ITrack* track )
{
    Update();
}

void DiscordPlaybackRemote::OnPlaybackStateChanged( musik::core::sdk::PlaybackState state )
{
    if( not m_PlaybackService or not g_Presence )
    {
        return;
    }

    switch( state )
    {
    default:
        return;
    case musik::core::sdk::PlaybackState::Stopped:
    case musik::core::sdk::PlaybackState::Paused:
        g_Presence->trackStopped();
        return;
    case musik::core::sdk::PlaybackState::Playing:
        break;
    }

    Update();
}

void DiscordPlaybackRemote::OnPlaybackTimeChanged( double time )
{
    Update();
}

void DiscordPlaybackRemote::Update()
{
    if( not m_PlaybackService or not g_Presence )
    {
        return;
    }

    if( m_PlaybackService->GetPlaybackState() != musik::core::sdk::PlaybackState::Playing )
    {
        return;
    }

    musik::core::sdk::ITrack* trk{ m_PlaybackService->GetPlayingTrack() };
    if( not trk )
    {
        return;
    }

    if( trk->GetMetadataState() != musik::core::sdk::MetadataState::Loaded )
    {
        return;
    }

    presenceplugin::TrackInfo info;
    info.title = getStringFromTrack( trk, musik::core::sdk::track::Title );
    info.album = getStringFromTrack( trk, musik::core::sdk::track::Album );
    info.artist = getStringFromTrack( trk, musik::core::sdk::track::Artist );
    g_Presence->setTrackInfo( info );

    const double remaining{ m_PlaybackService->GetDuration() - m_PlaybackService->GetPosition() };
    g_Presence->setEndTime( std::time( nullptr ) + static_cast<std::time_t>( std::round( remaining ) ) );
}