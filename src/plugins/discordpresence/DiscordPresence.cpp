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

#include <chrono>
#include <sstream>

#include "./discord/discord.h"

#include "DiscordPresence.h"

namespace presenceplugin {

std::string TrackInfo::topLine() const
{
    if( title.empty() )
    {
        return "<unknown song>";
    }

    std::ostringstream ostr;
    ostr << "»" << title << "«";
    return ostr.str();
}

std::string TrackInfo::bottomLine() const
{
    std::ostringstream ostr;

    ostr << "ʙʏ ";
    if( artist.empty() )
    {
        ostr << "<unknown artist>";
    }
    else
    {
        ostr << artist;
    }

    if( not album.empty() )
    {
        ostr << " ꜰʀᴏᴍ »" << album << "«";
    }

    return ostr.str();
}

DiscordPresence::DiscordPresence()
    : m_Logger      {} // add path here for debugging
    , m_StopRequest {}
    , m_Dirty       {}
    , m_EndTime     { std::numeric_limits<std::time_t>::max() }
{
    launch();
}

DiscordPresence::~DiscordPresence()
{
    join();
}

void DiscordPresence::trackStopped()
{
    setEndTime( std::numeric_limits<std::time_t>::min() );
}

void DiscordPresence::setTrackInfo( const TrackInfo& info )
{
    std::lock_guard<std::mutex> lock{ m_Lock };

    if( info.title != m_TrackInfo.title )
    {
        m_TrackInfo.title = info.title;
        m_Dirty = true;
    }

    if( info.album != m_TrackInfo.album )
    {
        m_TrackInfo.album = info.album;
        m_Dirty = true;
    }

    if( info.artist != m_TrackInfo.artist )
    {
        m_TrackInfo.artist = info.artist;
        m_Dirty = true;
    }
}

void DiscordPresence::setEndTime( std::time_t end )
{
    std::lock_guard<std::mutex> lock{ m_Lock };
    const bool playing{ isPlaying() };
    m_EndTime = end;
    m_Dirty = true;
}

void DiscordPresence::launch()
{
    // External thread only
    log( "Launching!" );
    m_PresenceThread = std::thread{ [ this ]{ main(); } };
}

void DiscordPresence::join()
{
    // External thread only
    if( m_PresenceThread.joinable() )
    {
        m_StopRequest.store( true );
        log( "Joining!" );
        m_PresenceThread.join();
    }
}

void DiscordPresence::main()
{
    log( "Enter main()" );

    const auto flags{ static_cast<std::uint64_t>( discord::CreateFlags::NoRequireDiscord ) };
    discord::Core* core;
    const auto result{ discord::Core::Create( 913136921096618005, flags, &core ) };
    if( not core )
    {
        log( std::string{ "Error " } + std::to_string( static_cast<int>( result ) ) );
        return;
    }

    log( "Connected, entering main loop" );
    while( not m_StopRequest.load() )
    {
        bool updateActivity{};
        discord::Activity activity{};
        {
            std::lock_guard<std::mutex> lock{ m_Lock };
            if( m_Dirty )
            {
                activity.SetType( discord::ActivityType::Listening );
                log( "update activity details:" );
                if( isPlaying() )
                {
                    log( m_TrackInfo.topLine() );
                    log( m_TrackInfo.bottomLine() );
                    activity.SetDetails( m_TrackInfo.topLine().c_str() );
                    activity.SetState( m_TrackInfo.bottomLine().c_str() );
                    activity.GetTimestamps().SetEnd( static_cast<discord::Timestamp>( m_EndTime ) );
                }
                else
                {
                    log( "no track." );
                    activity.SetDetails( "... nothing right now!" );
                    activity.SetState( "https://musikcube.com/" );
                }
                activity.GetAssets().SetLargeImage( "musikcube" );
                activity.GetAssets().SetLargeText( "musikcube — Open Source Audio Player" );
                updateActivity = true;
                m_Dirty = false;
            }
        }

        if( updateActivity )
        {
            log( "update activity" );
            core->ActivityManager().UpdateActivity( activity, [ this ]( discord::Result result )
            {
                if( result == discord::Result::Ok )
                {
                    log( "successful update" );
                }
                else
                {
                    log( "failed!" );
                }
            } );
        }

        core->RunCallbacks();
        std::this_thread::sleep_for( std::chrono::milliseconds{ 125 } );
    }
    log( "Exit main()" );
}

void DiscordPresence::log( const std::string& msg )
{
    if( m_Logger.good() )
    {
        m_Logger << msg << std::endl;
        m_Logger.flush();
    }
}

bool DiscordPresence::isPlaying() const
{
    // requires held lock!
    const std::time_t cur{ std::time( nullptr ) };
    return cur < m_EndTime;
}

}