#pragma once

#include <Windows.h>

#include <string>
#include <memory>

#include "utility/utility.h"

namespace FileDownloader
{
    class Logger
    {
    public:
        Logger();
        ~Logger();

        static bool init( const std::string& filename );

        static bool info( const std::string& data );
        static bool error( const std::string& data );

    private:
        static std::string m_filename;
        static TFileHandle m_file;
        static CRITICAL_SECTION m_lock;

        static bool log( const std::string& logType, const std::string& data );
        static bool open();
    };

    static Logger g_log;
}