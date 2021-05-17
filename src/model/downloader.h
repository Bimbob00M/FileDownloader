#pragma once

#include <Windows.h>
#include <winhttp.h>

#include <atomic>
#include <string>
#include <functional>
#include <vector>

#include "utility/utility.h"

namespace FileDownloader
{
    class Downloader
    {
    public:
        enum class EStatus : byte { CONNECTING, DOWNLOADING, FINISHED, FAILED };

        struct DownloadInfo
        {
            std::wstring url;
            std::wstring downloadInto;
            std::vector<LPWSTR> acceptTypes{};

            DownloadInfo( const std::wstring& url, const std::wstring& destinationFilename, const std::vector<LPWSTR>& types = {} )
                : url{ url }
                , downloadInto{ destinationFilename }
                , acceptTypes{ types }
            {}
        };

        Downloader( const DownloadInfo& info ) noexcept;
        virtual ~Downloader() noexcept;

        bool startDownload();

        virtual bool createSession();
        virtual bool createConnection();
        virtual bool createRequest();

        void releaseHandles() noexcept;

        void setDownloadDataCallback( const std::function<bool( const BYTE* data, DWORD size )>& callback ) noexcept;
        void setDownloadProgressCallback( const std::function<bool( ULONGLONG fileSize, ULONGLONG totalBytesRead )>& callback ) noexcept;
        void setGotContentLengthCallback( const std::function<bool( ULONGLONG length )>& callback ) noexcept;

        EStatus getStatusCode() const noexcept { return m_status; }
        std::wstring getStatus() const noexcept;
        std::wstring getFileName() const noexcept { return m_fileToDownloadInto; }

        DWORD getReadBufferSize() const noexcept { return m_readBufferSize; }

        bool isAborted() const noexcept { return m_abortFlag; }
        void abort() noexcept { m_abortFlag = true; }

    private:
        std::vector<LPCWSTR> m_acceptTypes{};

        std::wstring m_hostName; 
        std::wstring m_object;

        std::wstring m_urlToDownload; //The URL to startDownload
        std::wstring m_fileToDownloadInto; //Where to startDownload it to

        const std::wstring m_httpVerb{ L"GET" }; //The HTTP verb to use (always would be "GET")
        std::wstring m_httpVersion; //The HTTP version to use (e.g. "HTTP/1.0", "HTTP/1.1" etc)
        std::wstring m_referrer; //Allows you to customize the HTTP_REFERRER header

        std::atomic< bool > m_abortFlag{ false };

        DWORD m_requestFlags{ 0 };
        const DWORD m_readBufferSize{ 1024 };

        TFileHandle m_file{ nullptr };

        HINTERNET m_hInternetSession{ nullptr };
        HINTERNET m_hHttpConnection{ nullptr };
        HINTERNET m_hRequest{ nullptr };

        EStatus m_status{ EStatus::CONNECTING };

        std::function<bool( const BYTE* data, DWORD size )> m_onDownLoadDataCallback{ nullptr };
        std::function<bool( ULONGLONG fileSize, ULONGLONG totalBytesRead )> m_onDownLoadProgressCallback{ nullptr };
        std::function<bool( ULONGLONG length )> m_onGotContentLengthCallback{ nullptr };

//-----------functions-----------------------------

        Downloader( const Downloader& ) = delete;
        Downloader( Downloader&& ) = delete;

        Downloader& operator=( const Downloader& ) = delete;
        Downloader& operator=( Downloader&& ) = delete;

        static bool queryStatusNumber( HINTERNET hInternet, DWORD flag, DWORD& outCode ) noexcept;
        static bool queryStatusCode( HINTERNET hInternet, DWORD& outCode ) noexcept;
        static bool queryContentLength( HINTERNET hInternet, ULONGLONG& nCode );

        virtual bool onDownloadData( const BYTE* data, DWORD size ) const noexcept;
        virtual bool onDownloadProgress( ULONGLONG fileSize, ULONGLONG totalBytesRead ) const noexcept;
        virtual bool onGotContentLength( ULONGLONG length ) const noexcept;

        inline void setStatus( EStatus status ) noexcept { m_status = status; }
        
    };
}