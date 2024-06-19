#pragma once

#include <process.h>

#include <vector>

#include "model/downloader.h"
#include "base_window.h"

namespace FileDownloader
{
    typedef unsigned( WINAPI* PTHREAD_START ) ( void* );

    class DownloadWin : public BaseWindow
    {
        using DInfo = Downloader::DownloadInfo;
    public:
        DownloadWin( const std::vector< DInfo >& info ) noexcept;
        ~DownloadWin() noexcept;

        DownloadWin( const DownloadWin& ) = delete;
        DownloadWin& operator=( const DownloadWin& ) = delete;
        DownloadWin( DownloadWin&& ) = delete;
        DownloadWin& operator=( DownloadWin&& ) = delete;

        virtual PCWSTR getClassName() const { return L"MainWindow"; }

    protected:
        virtual LRESULT handleMessage( UINT msg, WPARAM wParam, LPARAM lParam ) override;

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
                : downloader{ downloader }
                , downloadInfo{ downloadInfo }
                , pThis{ pThis }
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
            std::wstring percent;
        };

        const std::vector< DInfo >& m_resourcesInfo;
        std::vector< HANDLE > m_threads;
        std::vector< UINT > m_threadIDs;
        std::vector< std::unique_ptr<Downloader> > m_threadDownloaders;
        std::vector< HWND > m_progressBars;
        std::vector< DownloadingResults > m_results;

        CRITICAL_SECTION m_lock;

        HDC m_memDC{ nullptr };
        HBITMAP m_memBitmap{ nullptr };

        bool download( const DInfo& info, Downloader& downloader );
        static DWORD WINAPI downloadInThread( PVOID param );

        inline void setDownloadingResults( const DownloadingResults result, size_t index );
        inline DownloadingResults& getDownloadingResults( size_t index ) { return m_results[index]; };
    };

} // namespace FileDownloader