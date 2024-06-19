#include <process.h>

#include <vector>

#include "model/downloader.h"
#include "model/logger.hpp"
#include "utility/framework.h"
#include "view/download_win.h"

int APIENTRY wWinMain( _In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE /*hPrevInstance*/,
                       _In_ LPWSTR    lpCmdLine,
                       _In_ int       nCmdShow )
{
    using namespace FileDownloader;
    Logger::getInstance().init( "log.txt" );

    int argc{};
    auto argv = CommandLineToArgvW( lpCmdLine, &argc );

    if( !argv )
    {
        MessageBox( nullptr,
                    L"Command arguments parse error. Cannot working.",
                    L"Invalid arguments",
                    MB_ICONERROR | MB_OK );
        return 1;
    }

    std::vector< Downloader::DownloadInfo > info;
    if( argc % 2 != 0 )
    {
        argc -= 1;
        MessageBox( nullptr,
                    L"An odd number of command argments were passed. An even number will be used.",
                    L"Invalid arguments",
                    MB_ICONWARNING | MB_OK );
    }

    for( int i = 0; i < argc; i += 2 )
    {
        std::wstring downloadInto = argv[i];
        std::wstring url = argv[i + 1];
        info.emplace_back( url, downloadInto );
    }

    FileDownloader::DownloadWin win( info );
    if( !win.create( L"FileDownloader", WS_DLGFRAME | WS_SYSMENU ) )
    {
        auto error = GetLastError();
        std::wstring errorMsg = L"Something went wrong." + std::to_wstring( error );
        MessageBox( nullptr,
                    errorMsg.c_str(),
                    L"Iternal error.",
                    MB_ICONERROR | MB_OK );
        return 1;
    }
    win.show( nCmdShow );
    win.update();

    MSG msg;
    while( GetMessage( &msg, nullptr, 0, 0 ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    return msg.wParam;
}
