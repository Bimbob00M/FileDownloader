#include "utility.hpp"

#include <cstdio>

namespace FileDownloader
{
    void FileDeleter::operator()( FILE* file )
    {
        if( file )
        {
            fclose( file );
            file = nullptr;
        }
    }

    void InternetDeleter::operator()( HINTERNET* internet )
    {
        if( internet )
        {
            WinHttpCloseHandle( internet );
            internet = nullptr;
        }
    }

} // namespace FileDownloader