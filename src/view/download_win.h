#pragma once

#include <Windows.h>
#include <process.h>

#include <vector>

#include "model/downloader.h"

namespace FileDownloader
{
    typedef unsigned( WINAPI* PTHREAD_START ) ( void* );

    class DownloadWin
    {
        using DInfo = Downloader::DownloadInfo;
    public:
        DownloadWin( const std::vector< DInfo >& info ) noexcept;
        virtual ~DownloadWin() noexcept;

        BOOL create( PCWSTR lpWindowName,
                     DWORD dwStyle,
                     DWORD dwExStyle = 0,
                     int x = CW_USEDEFAULT,
                     int y = CW_USEDEFAULT,
                     int nWidth = CW_USEDEFAULT,
                     int nHeight = CW_USEDEFAULT,
                     HWND hWndParent = 0,
                     HMENU hMenu = 0 );

        inline BOOL show( int nCmdShow ) { return ShowWindow( m_hWnd, nCmdShow ); }
        inline BOOL update() { return UpdateWindow( m_hWnd ); }
        inline BOOL invalidateRect( const RECT* rect, bool erase = false ) { return InvalidateRect( m_hWnd, rect, erase ); };

        inline HWND   getHandle() const { return m_hWnd; }
        inline PCWSTR getClassName() const { return L"Main Window"; };

    protected:
        virtual void initWndClass( WNDCLASSEX& outWndClass ) const;

        virtual LRESULT handleMessage( UINT msg, WPARAM wParam, LPARAM lParam );

        virtual bool onCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
        virtual void onClose( HWND hWnd );
        virtual void onDestroy( HWND hWnd );
        virtual void onPaint( HWND hWnd );
        virtual void onDownloadFinished();

    private:
        struct DInfoWrapper
        {
            DInfo downloadInfo;
            Downloader* downloader{ nullptr };
            DownloadWin* pThis{ nullptr };

            DInfoWrapper( const DInfo& downloadInfo, Downloader* downloader,  DownloadWin* pThis )
                : downloader( downloader )
                , downloadInfo( downloadInfo )
                , pThis( pThis )
            {}

            ~DInfoWrapper()
            {
                downloader = nullptr;
                pThis = nullptr;
            }
        };

        struct DownloadingResults
        {
            std::wstring caption;
            std::wstring status;
        };

        const std::vector< DInfo >& m_resourcesInfo;
        std::vector< HANDLE > m_threads;
        std::vector< UINT > m_threadIDs;
        std::vector< std::unique_ptr<Downloader> > m_threadDownloaders;
        std::vector< HWND > m_progressBars;
        std::vector< DownloadingResults > m_results;

        CRITICAL_SECTION m_lock;

        HWND m_hWnd{ nullptr };

        HDC m_memDC{ nullptr };
        HBITMAP m_memBitmap{ nullptr };

        DownloadWin( const DownloadWin& ) = delete;
        DownloadWin& operator=( const DownloadWin& ) = delete;
        DownloadWin( DownloadWin&& ) = delete;
        DownloadWin& operator=( DownloadWin&& ) = delete;

        inline ATOM registerClass() const;
        static LRESULT windowProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
        bool download( const DInfo& info, Downloader& downloader );
        static DWORD WINAPI downloadInThread( PVOID param );
        void setDownloadingResults( const DownloadingResults result, size_t index );
    };

} // namespace FileDownloader