#pragma once

#include <Windows.h>

namespace FileDownloader
{
    class BaseWindow
    {
    public:
        virtual ~BaseWindow() noexcept;

        BaseWindow( const BaseWindow& ) = delete;
        BaseWindow& operator=( const BaseWindow& ) = delete;

        static LRESULT CALLBACK windowProcedure( HWND hwnd,
                                                 UINT uMsg,
                                                 WPARAM wParam,
                                                 LPARAM lParam );

        BOOL create( PCWSTR lpWindowName,
                     DWORD dwStyle,
                     DWORD dwExStyle = 0,
                     int x = CW_USEDEFAULT,
                     int y = CW_USEDEFAULT,
                     int nWidth = CW_USEDEFAULT,
                     int nHeight = CW_USEDEFAULT,
                     HWND hWndParent = nullptr,
                     HMENU hMenu = nullptr );

        BOOL show( int nCmdShow );
        BOOL update();
        BOOL invalidateRect( const RECT* rect, bool erase = false );

        HWND getHandle() const;
        virtual PCWSTR getClassName() const = 0;

    protected:
        HWND m_hWnd{ nullptr };

        BaseWindow() noexcept = default;

        virtual void initClass( WNDCLASSEX& outWndClass ) const;
        virtual LRESULT handleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam ) = 0;

    private:
        ATOM registerClass() const;
    };
}   