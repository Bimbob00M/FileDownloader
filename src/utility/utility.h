#pragma once

#include <Windows.h>
#include <winhttp.h>

#include <cstdio>
#include <memory>

namespace FileDownloader
{
    struct FileDeleter { void operator()( FILE* file ); };
    using  TFileHandle = std::unique_ptr<FILE, FileDeleter>;

    struct InternetDeleter { void operator()( HINTERNET* internet ); };
    using  TInternetHandle = std::unique_ptr<HINTERNET, InternetDeleter>;
        
} //namespace FileDownloader