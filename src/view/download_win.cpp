#include "download_win.h"

#include <windowsx.h>
#include <Commctrl.h>

#include <algorithm>

#include "model/logger.h"

namespace FileDownloader
{
    // progress bar width
    static constexpr int PB_WIDTH = 400;
    //progress bar heihgt
    static constexpr int PB_HEIGHT = 20;
    static constexpr int DOWNLOAD_INFO_LABEL_HEIGHT = 40;
    static constexpr int AFTER_PB_SPACE = 10;
    static constexpr int DOWNLOAD_INFO_HEIGHT = PB_HEIGHT + DOWNLOAD_INFO_LABEL_HEIGHT + AFTER_PB_SPACE;
    static constexpr int BORDER_OFFSET = 15;

    static constexpr unsigned int UM_DOWNLOAD_FINISHED{ WM_APP + 1 };

    DownloadWin::DownloadWin( const std::vector< Downloader::DownloadInfo >& info ) noexcept
        : m_resourcesInfo( info )
    {
        InitializeCriticalSection( &m_lock );
    }

    DownloadWin::~DownloadWin() noexcept
    {
        DeleteCriticalSection( &m_lock );
        SendMessage( m_hWnd, WM_CLOSE, NULL, NULL );
    }

    BOOL DownloadWin::create( PCWSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu )
    {
        if( !registerClass() )
            return false;

        m_hWnd = CreateWindowEx( dwExStyle,
                                 getClassName(),
                                 lpWindowName,
                                 dwStyle,
                                 x,
                                 y,
                                 nWidth,
                                 nHeight,
                                 hWndParent,
                                 hMenu,
                                 GetModuleHandle( nullptr ),
                                 this );

        return m_hWnd ? true : false;
    }

    void DownloadWin::initWndClass( WNDCLASSEX& outWndClass ) const
    {
        outWndClass.hInstance = GetModuleHandle( nullptr );
        outWndClass.lpfnWndProc = windowProc;
        outWndClass.style = CS_HREDRAW | CS_VREDRAW;
        outWndClass.hbrBackground = reinterpret_cast< HBRUSH >( COLOR_WINDOW + 1 );
        outWndClass.lpszClassName = getClassName();
    }

    LRESULT DownloadWin::handleMessage( UINT msg, WPARAM wParam, LPARAM lParam )
    {
        switch( msg )
        {
            HANDLE_MSG( m_hWnd, WM_CREATE, onCreate );
            HANDLE_MSG( m_hWnd, WM_CLOSE, onClose );
            HANDLE_MSG( m_hWnd, WM_DESTROY, onDestroy );
            HANDLE_MSG( m_hWnd, WM_PAINT, onPaint );

            case UM_DOWNLOAD_FINISHED:
                onDownloadFinished();
                break;

            default:
                return DefWindowProc( m_hWnd, msg, wParam, lParam );
        }
        return 0L;
    }

    inline ATOM DownloadWin::registerClass() const
    {
        WNDCLASSEX wcex{};
        wcex.cbSize = sizeof( WNDCLASSEX );
        initWndClass( wcex );

        return RegisterClassEx( &wcex );
    }

    LRESULT DownloadWin::windowProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
        DownloadWin* pThis = nullptr;

        if( msg == WM_NCCREATE )
        {
            CREATESTRUCT* pCreate = reinterpret_cast< CREATESTRUCT* >( lParam );
            pThis = reinterpret_cast< DownloadWin* >( pCreate->lpCreateParams );
            SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast< LONG_PTR >( pThis ) );

            pThis->m_hWnd = hWnd;
        }
        else
        {
            pThis = reinterpret_cast< DownloadWin* >( GetWindowLongPtr( hWnd, GWLP_USERDATA ) );
        }

        if( pThis )
        {
            return pThis->handleMessage( msg, wParam, lParam );
        }
        else
        {
            return DefWindowProc( hWnd, msg, wParam, lParam );
        }
    }


    bool DownloadWin::onCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
    {
        if( m_resourcesInfo.empty() )
            return false;

        InitCommonControls();

        if( HDC hWinDC = GetDC( m_hWnd ) )
        {
            if( m_memDC = CreateCompatibleDC( hWinDC ) )
            {
                RECT clientRect;
                GetClientRect( m_hWnd, &clientRect );
                if( m_memBitmap = CreateCompatibleBitmap( hWinDC, clientRect.right, clientRect.bottom ) )
                {
                    DeleteObject( SelectObject( m_memDC, m_memBitmap ) );
                }
                ReleaseDC( m_hWnd, hWinDC );
            }
        }

        for( size_t i = 0; i < m_resourcesInfo.size(); ++i )
        {

            auto progBar = CreateWindowEx( 0,
                                           PROGRESS_CLASS,
                                           reinterpret_cast< LPTSTR >( 0 ),
                                           WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                                           BORDER_OFFSET,
                                           BORDER_OFFSET + DOWNLOAD_INFO_LABEL_HEIGHT + i * DOWNLOAD_INFO_HEIGHT,
                                           PB_WIDTH,
                                           PB_HEIGHT,
                                           m_hWnd,
                                           0,
                                           GetModuleHandle( nullptr ),
                                           NULL );
            if( !progBar )
                return false;

            m_progressBars.push_back( progBar );
        }

        constexpr int winWidth = BORDER_OFFSET * 3 + PB_WIDTH;
        const int winHeight = BORDER_OFFSET * 3 + DOWNLOAD_INFO_HEIGHT * m_progressBars.size() + PB_HEIGHT;

        SetWindowPos( m_hWnd,
                      nullptr,
                      0, 0, winWidth, winHeight,
                      SWP_NOMOVE | SWP_NOZORDER );

        m_threadIDs.resize( m_resourcesInfo.size() );
        m_threadDownloaders.reserve( m_resourcesInfo.size() );
        m_results.resize( m_resourcesInfo.size() );

        int index = 0;
        for( const auto& info : m_resourcesInfo )
        {
            m_threadDownloaders.emplace_back( std::make_unique<Downloader>( info ) );
            auto infoWrapper = new DInfoWrapper( info, m_threadDownloaders[index].get(), this );  //thread must free it
            auto thread = reinterpret_cast< HANDLE >( _beginthreadex( nullptr,
                                                                      0,
                                                                      reinterpret_cast< PTHREAD_START >( &DownloadWin::downloadInThread ),
                                                                      reinterpret_cast< PVOID >( const_cast< DInfoWrapper* >( infoWrapper ) ),
                                                                      0,
                                                                      &m_threadIDs[index] ) );
            if( !thread )
                return false;
            m_threads.push_back( thread );
            ++index;
            Sleep( 2 );
        }
        return true;
    }

    void DownloadWin::onClose( HWND hWnd )
    {
        show( SW_HIDE );

        for( auto& downloader : m_threadDownloaders )
        {
            downloader.get()->abort();
        }
        WaitForMultipleObjects( m_threads.size(), m_threads.data(), true, INFINITE );
        if( m_hWnd )
            DestroyWindow( hWnd );
    }

    void DownloadWin::onDestroy( HWND hWnd )
    {
        for( auto& progressBar : m_progressBars )
        {
            if( progressBar )
                DestroyWindow( progressBar );
        }

        for( auto& thread : m_threads )
        {
            if( thread )
                CloseHandle( thread );
        }

        DeleteObject( m_memDC );
        DeleteObject( m_memBitmap );

        PostQuitMessage( 0 );
    }

    void DownloadWin::onPaint( HWND hWnd )
    {
        PAINTSTRUCT ps;
        BeginPaint( hWnd, &ps );

        const int width = ps.rcPaint.right - ps.rcPaint.left;
        const int height = ps.rcPaint.bottom - ps.rcPaint.top;

        auto dc = m_memDC ? m_memDC : ps.hdc;

        HBRUSH bkgnd = GetStockBrush( WHITE_BRUSH );
        FillRect( dc, &ps.rcPaint, bkgnd );
        DeleteObject( bkgnd );

        for( int i = 0; i < m_results.size(); ++i )
        {
            int x = BORDER_OFFSET + 5;
            int y = BORDER_OFFSET + i * ( DOWNLOAD_INFO_HEIGHT );

            const auto& caption = m_results[i].caption;
            const auto& status = m_results[i].status;

            TextOut( dc, x, y, caption.c_str(), caption.size() );
            TextOut( dc, x, y + 20, status.c_str(), status.size() );

        }

        if( m_memDC )
            BitBlt( ps.hdc, 0, 0, width, height, m_memDC, 0, 0, SRCCOPY );

        EndPaint( hWnd, &ps );
    }

    void DownloadWin::onDownloadFinished()
    {
        auto result = WaitForMultipleObjects( m_threads.size(), m_threads.data(), true, 500 );
        if( result == WAIT_OBJECT_0 )
        {
            Sleep( 500 );
            SendMessage( m_hWnd, WM_CLOSE, NULL, NULL );
        }
    }

    bool DownloadWin::download( const DInfo& info, Downloader& downloader )
    {
        auto criticalSection = [&]( const auto& lambda )
        {
            EnterCriticalSection( &m_lock );

            auto currentId = GetCurrentThreadId();
            auto it = std::find( m_threadIDs.begin(), m_threadIDs.end(), currentId );
            if( it != m_threadIDs.end() )
            {
                auto index = std::distance( m_threadIDs.begin(), it );
                lambda( index );
            }
            else
            {
                LeaveCriticalSection( &m_lock );
                return false;
            }
            LeaveCriticalSection( &m_lock );
            return true;
        };

        downloader.setGotContentLengthCallback(
            [&]( ULONGLONG lenght )
            {
                return criticalSection(
                    [&]( auto index )
                    {
                        if( lenght > 0 )
                        {
                            auto constentLenght = lenght / downloader.getReadBufferSize();
                            PostMessage( m_progressBars[index], PBM_SETRANGE32, 0, lenght );
                        }
                        else
                        {
                            SetWindowPos( m_progressBars[index],
                                          nullptr,
                                          0, 0, 0, 0,
                                          SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER );
                        }
                    } );
            } );

        downloader.setDownloadProgressCallback(
            [&]( ULONGLONG fileSize, ULONGLONG bytesRead )
            {
                return criticalSection(
                    [&]( auto index )
                    {
                        int x = BORDER_OFFSET + 5;
                        int y = BORDER_OFFSET + index * ( DOWNLOAD_INFO_HEIGHT );

                        std::wstring caption( downloader.getFileName() + L" - " + downloader.getStatus() );
                        std::wstring status( L"Status: " + downloader.getStatus() );
                        auto code = downloader.getStatusCode();
                        if( code == Downloader::EStatus::DOWNLOADING ||
                            code == Downloader::EStatus::FINISHED )
                        {
                            if( fileSize )
                            {
                                auto percent = static_cast< int >( ( static_cast< float >( bytesRead ) / fileSize ) * 100 );
                                status.append( L" - " + std::to_wstring( percent ) + L"%" );

                            }
                        }
                        invalidateRect( nullptr, true );

                        setDownloadingResults( { caption, status }, index );

                        PostMessage( m_progressBars[index], PBM_SETPOS, bytesRead, 0 );
                    } );
            } );

        auto currentId = GetCurrentThreadId();
        auto wcharString = downloader.getFileName();
        Logger::info( "Thread #" +
                      std::to_string( currentId ) +
                      "\t downloading " +
                      std::string( wcharString.begin(), wcharString.end() ) );

        auto result = downloader.startDownload();

        wcharString = downloader.getStatus();
        auto resultLogMsg = std::string( "Thread #" +
                                         std::to_string( currentId ) +
                                         "\t Resulst message:\t" +
                                         std::string( wcharString.begin(), wcharString.end() ) );
        if( !result )
            Logger::error( resultLogMsg );
        else
            Logger::info( resultLogMsg );

        PostMessage( m_hWnd, UM_DOWNLOAD_FINISHED, NULL, NULL );
        return result;
    }

    DWORD WINAPI DownloadWin::downloadInThread( PVOID param )
    {
        auto data = reinterpret_cast< DownloadWin::DInfoWrapper* >( param );
        if( data && data->pThis && data->downloader )
            data->pThis->download( data->downloadInfo, *data->downloader );

        delete data;
        return 0;
    }

    void DownloadWin::setDownloadingResults( const DownloadWin::DownloadingResults result, size_t index )
    {
        m_results[index] = result;
    }

}