#pragma once

#include <Windows.h>
#include <winhttp.h>

#include <cstdio>
#include <sstream>
#include <string>
#include <memory>

namespace FileDownloader
{
    struct FileDeleter { void operator()( FILE* file ); };
    using  TFileHandle = std::unique_ptr<FILE, FileDeleter>;

    struct InternetDeleter { void operator()( HINTERNET* internet ); };
    using  TInternetHandle = std::unique_ptr<HINTERNET, InternetDeleter>;

    namespace Utility
    {
        template <typename... Ts>
        std::string Concat( Ts&&... args )
        {
            std::ostringstream oss;
            ( oss << ... << std::forward<Ts>( args ) );
            return oss.str();
        }
    }
        
} //namespace FileDownloader