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

#pragma once

#include <atomic>
#include <ctime>
#include <fstream>
#include <mutex>
#include <thread>

namespace presenceplugin {

struct TrackInfo final
{
    std::string topLine() const;
    std::string bottomLine() const;

    std::string title;
    std::string album;
    std::string artist;
};

class DiscordPresence final
{
public:
    DiscordPresence();
    ~DiscordPresence();

    void trackStopped();
    void setTrackInfo( const TrackInfo& info );
    void setEndTime( std::time_t end );

private:
    void launch();
    void join();

    void main();

    void log( const std::string& msg );

    // requires held lock
    bool isPlaying() const;

    std::ofstream    m_Logger;
    std::atomic_bool m_StopRequest;
    std::thread      m_PresenceThread;

    std::mutex       m_Lock;
    bool             m_Dirty;
    TrackInfo        m_TrackInfo;
    std::time_t      m_EndTime;
};

}