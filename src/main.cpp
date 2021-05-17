#include <process.h>

#include <vector>

#include "model/downloader.h"
#include "model/logger.h"
#include "utility/framework.h"
#include "view/download_win.h"

int APIENTRY wWinMain( _In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE /*hPrevInstance*/,
                       _In_ LPWSTR    lpCmdLine,
                       _In_ int       nCmdShow )
{
    using namespace FileDownloader;
    Logger::init( "log.txt" );
    
    int argc = 0;
    auto argv = CommandLineToArgvW( lpCmdLine, &argc );

    if( !argv )
        return 1;

    std::vector< Downloader::DownloadInfo > info;
    size_t pairedParamsNum = ( argc % 2 ) == 0 ? argc : argc - 1;
    for( size_t i = 0; i < pairedParamsNum; i += 2 )
    {
        std::wstring downloadInto = argv[i];
        std::wstring url = argv[i + 1];
        info.emplace_back( url, downloadInto );
    }

    FileDownloader::DownloadWin win( info );
    if( !win.create( L"FileDownloader", WS_DLGFRAME | WS_SYSMENU ) )
    {
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
