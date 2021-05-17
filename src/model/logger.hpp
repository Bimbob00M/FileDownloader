#pragma once

#include <Windows.h>

#include <string>
#include <memory>

#include "utility/utility.hpp"

namespace FileDownloader
{
    class Logger
    {
    public:
        static Logger& getInstance()
        {
            static Logger singleton;
            return singleton;
        }

        Logger();
        ~Logger();

        bool init( const std::string& filename );

        template <typename... Ts>
        bool info( Ts&&... args );
        template <typename... Ts>
        bool error( Ts&&... args );

    private:
        std::string m_filename;
        TFileHandle m_file;
        CRITICAL_SECTION m_lock;

        template <typename... Ts>
        bool log( Ts&&... args );

        bool open();

        Logger( const Logger& ) = delete;
        Logger( Logger&& ) = delete;
        Logger& operator=( const Logger& ) = delete;
        Logger& operator=( Logger&& ) = delete;
    };

    template<typename ...Ts>
    inline bool Logger::info( Ts && ...args )
    {
        return log( "[Info]\t", args... );;
    }

    template<typename ...Ts>
    inline bool Logger::error( Ts && ...args )
    {
        return log( "[Error]\t", args... );;
    }

    template <typename... Ts>
    bool Logger::log( Ts&&... args )
    {
        EnterCriticalSection( &m_lock );

        if( !m_file || sizeof...( args ) == 0 )
        {
            LeaveCriticalSection( &m_lock );
            return false;
        }

        auto conscatedLogMsg = Utility::Concat( args..., "\r\n" );
        if( 0 == fwrite( conscatedLogMsg.data(), conscatedLogMsg.size(), size_t( 1 ), m_file.get() ) )
        {
            LeaveCriticalSection( &m_lock );
            return false;
        }
        LeaveCriticalSection( &m_lock );
        return true;
    }

}




