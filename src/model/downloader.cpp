#include "model/downloader.h"

#include <cwchar>
#include <fstream>
#include <vector>

#include "model/logger.hpp"

namespace FileDownloader
{
    Downloader::Downloader( const DownloadInfo& info ) noexcept
        : m_acceptTypes{ info.acceptTypes.begin(), info.acceptTypes.end() }
        , m_urlToDownload{ info.url }
        , m_fileToDownloadInto{ info.downloadInto }
    {

        FILE* newFile{ nullptr };
        std::string downloadIntoFile( info.downloadInto.begin(), info.downloadInto.end() );
        if( fopen_s( &newFile, downloadIntoFile.c_str(), "w+b" ) == 0 )
            m_file = TFileHandle( newFile );

        URL_COMPONENTS urlComp{};
        urlComp.dwStructSize = sizeof( URL_COMPONENTS );

        urlComp.dwHostNameLength = DWORD( -1 );
        urlComp.dwUrlPathLength = DWORD( -1 );

        LPCWSTR pwszUrl1 = m_urlToDownload.c_str();
        if( WinHttpCrackUrl( pwszUrl1, m_urlToDownload.size(), 0, &urlComp ) )
        {
            m_hostName = std::wstring( urlComp.lpszHostName, urlComp.dwHostNameLength );
            m_object = std::wstring( urlComp.lpszUrlPath, urlComp.dwUrlPathLength );
        }
    }

    Downloader::~Downloader() noexcept
    {
        releaseHandles();
    }

    bool Downloader::startDownload()
    {
        releaseHandles();

        setStatus( EStatus::CONNECTING );

        if( !createSession() || !createConnection() || !createRequest() )
        {
            setStatus( EStatus::FAILED );
            return false;
        }

        const auto contextValue = reinterpret_cast< DWORD_PTR >( this );
        if( !WinHttpSendRequest( m_hRequest,
                                 WINHTTP_NO_ADDITIONAL_HEADERS,
                                 0,
                                 WINHTTP_NO_REQUEST_DATA,
                                 0,
                                 0,
                                 contextValue ) )
        {
            setStatus( EStatus::FAILED );
            return false;
        }


        if( !WinHttpReceiveResponse( m_hRequest, nullptr ) )
        {
            setStatus( EStatus::FAILED );
            return false;
        }

        //Handle the status code in the response
        DWORD statusCode = 0;
        if( !queryStatusCode( m_hRequest, statusCode ) )
        {
            setStatus( EStatus::FAILED );
            return false;
        }
        if( statusCode != HTTP_STATUS_OK )
        {
            setStatus( EStatus::FAILED );
            return false;
        }

        //Get the length of the file
        ULONGLONG contentLength{ 0 };
        queryContentLength( m_hRequest, contentLength );
        onGotContentLength( contentLength );

        setStatus( EStatus::DOWNLOADING );

        //Now do the actual read of the file
        DWORD bytesRead{ 0 };
        std::vector<BYTE> readBuffer;
        readBuffer.resize( m_readBufferSize );
        ULONGLONG totalBytesRead{ 0 };
        do
        {
            if( !WinHttpReadData( m_hRequest, readBuffer.data(), m_readBufferSize, &bytesRead ) )
            {
                setStatus( EStatus::FAILED );
                return false;
            }
            else
            {
                if( bytesRead )
                {
                    if( !onDownloadData( readBuffer.data(), bytesRead ) )
                        return false;

                    totalBytesRead += bytesRead;
                }

                if( totalBytesRead == contentLength )
                    setStatus( EStatus::FINISHED );

                onDownloadProgress( contentLength, totalBytesRead );
            }
            Sleep( 5 );
        }
        while( bytesRead && !isAborted() );

        //Free up the internet handles now that we are finished with them
        releaseHandles();

        if( isAborted() )
        {
            setStatus( EStatus::ABORTED );
        }
        else
            setStatus( EStatus::FINISHED );

        return true;
    }

    bool Downloader::createSession()
    {
        //Create the Internet session handle
        if( m_hInternetSession == nullptr )
        {
            m_hInternetSession = WinHttpOpen( L"WinHTTP Example/1.0",
                                              WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                              WINHTTP_NO_PROXY_NAME,
                                              WINHTTP_NO_PROXY_BYPASS,
                                              0 );
        }

        if( m_hInternetSession == nullptr )
        {
            return false;
        }

        return true;
    }

    bool Downloader::createConnection()
    {
        m_hHttpConnection = WinHttpConnect( m_hInternetSession, m_hostName.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0 );

        if( m_hHttpConnection == nullptr )
        {
            return false;
        }

        return true;
    }

    bool Downloader::createRequest()
    {
        // must be null-terminated
        if( m_acceptTypes.size() )
        {
            if( m_acceptTypes.back() != nullptr )
                m_acceptTypes.resize( m_acceptTypes.size() + 1 );
        }

        m_hRequest = WinHttpOpenRequest( m_hHttpConnection,
                                         m_httpVerb.size() ? m_httpVerb.c_str() : nullptr,
                                         m_object.size() ? m_object.c_str() : nullptr,
                                         m_httpVersion.size() ? m_httpVersion.c_str() : nullptr,
                                         m_referrer.size() ? m_referrer.c_str() : nullptr,
                                         m_acceptTypes.data(),
                                         m_requestFlags );

        if( m_hRequest == nullptr )
        {
            return false;
        }

        return true;
    }

    bool Downloader::queryStatusNumber( HINTERNET hInternet, DWORD flag, DWORD& outCode ) noexcept
    {
        outCode = 0;
        DWORD size = sizeof( outCode );
        return ( WinHttpQueryHeaders( hInternet, flag | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &outCode, &size, WINHTTP_NO_HEADER_INDEX ) != false );
    }

    bool Downloader::queryStatusCode( HINTERNET hInternet, DWORD& outCode ) noexcept
    {
        return queryStatusNumber( hInternet, WINHTTP_QUERY_STATUS_CODE, outCode );
    }

    bool Downloader::queryContentLength( HINTERNET hInternet, ULONGLONG& nCode )
    {
        std::wstring contentLength;
        contentLength.resize( 32 );
        DWORD size = contentLength.size();
        auto buffer = reinterpret_cast< void* >( const_cast< wchar_t* >( contentLength.data() ) );

        const bool success = ( WinHttpQueryHeaders( hInternet, WINHTTP_QUERY_CONTENT_LENGTH, nullptr, buffer, &size, WINHTTP_NO_HEADER_INDEX ) != FALSE );

        if( success )
            nCode = _wtoi64( contentLength.data() );

        return success;
    }

    void Downloader::releaseHandles() noexcept
    {
        //Free up the internet handles we may be using
        if( m_hRequest )
        {
            WinHttpCloseHandle( m_hRequest );
            m_hRequest = nullptr;
        }
        if( m_hHttpConnection != nullptr )
        {
            WinHttpCloseHandle( m_hHttpConnection );
            m_hHttpConnection = nullptr;
        }
        if( m_hInternetSession != nullptr )
        {
            WinHttpCloseHandle( m_hInternetSession );
            m_hInternetSession = nullptr;
        }
    }

    void Downloader::setDownloadDataCallback( const std::function<bool( const BYTE* data, DWORD size )>& callback ) noexcept
    {
        m_onDownLoadDataCallback = callback;
    }

    void Downloader::setDownloadProgressCallback( const std::function<bool( ULONGLONG fileSize, ULONGLONG totalBytesRead )>& callback ) noexcept
    {
        m_onDownLoadProgressCallback = callback;
    }

    void Downloader::setGotContentLengthCallback( const std::function<bool( ULONGLONG length )>& callback ) noexcept
    {
        m_onGotContentLengthCallback = callback;
    }

    void Downloader::setStatusChangedCallback( const std::function<void()>& callback ) noexcept
    {
        m_onStatusChanged = callback;
    }

    std::wstring Downloader::getStatus() const noexcept
    {
        switch( m_status )
        {
            case EStatus::CONNECTING:
                return { L"Connecting" };
            case EStatus::DOWNLOADING:
                return { L"Downloading" };
            case EStatus::FINISHED:
                return { L"Finished" };
            case EStatus::FAILED:
                return { L"Failed" };
            case EStatus::ABORTED:
                return { L"Aborted" };
            default:
                return { L"Unknown error" };
        }
    }

    bool Downloader::onDownloadData( const BYTE* data, DWORD size ) const noexcept
    {
        if( m_fileToDownloadInto.empty() || !m_file )
        {
            return false;
        }
        fwrite( data, static_cast< size_t >( size ), size_t( 1 ), m_file.get() );

        if( m_onDownLoadDataCallback )
            m_onDownLoadDataCallback( data, size );

        return true;
    }

    bool Downloader::onDownloadProgress( ULONGLONG fileSize, ULONGLONG totalBytesRead ) const noexcept
    {
        if( m_onDownLoadProgressCallback )
            m_onDownLoadProgressCallback( fileSize, totalBytesRead );
        return true;
    }


    bool Downloader::onGotContentLength( ULONGLONG length ) const noexcept
    {
        if( m_onGotContentLengthCallback )
            m_onGotContentLengthCallback( length );
        return true;
    }

    void Downloader::onStatusChanged() const noexcept
    {
        if( m_onStatusChanged )
            m_onStatusChanged();
    }

    void Downloader::setStatus( EStatus status ) noexcept
    {
        m_status = status;
        if( status == EStatus::FAILED || 
            status == EStatus::ABORTED )
        {
            m_file.reset();
            DeleteFile( m_fileToDownloadInto.c_str() );
        }
        onStatusChanged();
    }
}