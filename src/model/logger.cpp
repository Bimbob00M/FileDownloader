#include "logger.h"

#include <Windows.h>

#include <cstdio>

namespace FileDownloader
{
    std::string Logger::m_filename;
    TFileHandle Logger::m_file;
    CRITICAL_SECTION Logger::m_lock;

    Logger::Logger()
    {
        InitializeCriticalSection( &m_lock );
    }

    Logger::~Logger()
    {
        DeleteCriticalSection( &m_lock );
    }

    bool Logger::init( const std::string& filename )
    {
        EnterCriticalSection( &m_lock );
        m_filename = filename;
        auto result = open();
        LeaveCriticalSection( &m_lock );
        return result;
    }

    bool Logger::info( const std::string& data )
    {
        return log( "Info", data );
    }

    bool Logger::error( const std::string& data )
    {
        return log( "Error", data );
    }

    bool Logger::log( const std::string& logType, const std::string& data )
    {
        EnterCriticalSection( &m_lock );

        if( !m_file || data.empty() || logType.empty() )
        {
            LeaveCriticalSection( &m_lock );
            return false;
        }

        std::string formated( "[" + logType + "]\t" + data + "\r\n" );

        if( 0 == fwrite( formated.data(), formated.size(), size_t( 1 ), m_file.get() ) )
        {
            LeaveCriticalSection( &m_lock );
            return false;
        }
        LeaveCriticalSection( &m_lock );
        return true;
    }

    bool Logger::open()
    {
        FILE* newFile = nullptr;
        if( ( fopen_s( &newFile, m_filename.c_str(), "w+b" ) != 0 ) )
            return false;

        m_file = TFileHandle( newFile );
        return true;
    }

}
