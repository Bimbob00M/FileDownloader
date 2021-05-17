#include "logger.hpp"

#include <Windows.h>

#include <cstdio>

namespace FileDownloader
{
    Logger::Logger()
    {
        InitializeCriticalSection( &m_lock );
    };

    Logger::~Logger()
    {
        DeleteCriticalSection( &m_lock );
    };

    bool Logger::init( const std::string& filename )
    {
        EnterCriticalSection( &m_lock );
        m_filename = filename;
        auto result = open();
        LeaveCriticalSection( &m_lock );
        return result;
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
