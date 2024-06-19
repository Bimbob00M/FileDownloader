#include "base_window.h"

namespace FileDownloader
{
    BaseWindow::~BaseWindow() noexcept
    {
        SendMessage( m_hWnd, WM_CLOSE, 0, 0 );
    }

    LRESULT BaseWindow::windowProcedure( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        BaseWindow* window{ nullptr };

        if( uMsg == WM_NCCREATE )
        {
            CREATESTRUCT* createInfo = reinterpret_cast<CREATESTRUCT*>( lParam );
            window = reinterpret_cast<BaseWindow*>( createInfo->lpCreateParams );
            SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( window ) );

            window->m_hWnd = hwnd;
        }
        else
        {
            window = reinterpret_cast<BaseWindow*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
        }

        return window ? window->handleMessage( uMsg, wParam, lParam )
                      : DefWindowProc( hwnd, uMsg, wParam, lParam );
    }

    BOOL BaseWindow::create( PCWSTR lpWindowName,
                             DWORD dwStyle,
                             DWORD dwExStyle,
                             int x,
                             int y,
                             int nWidth,
                             int nHeight,
                             HWND hWndParent,
                             HMENU hMenu )
    {
        if( m_hWnd || !registerClass() )
            return false;

        m_hWnd = CreateWindowEx( dwExStyle,
                                 getClassName(),
                                 lpWindowName,
                                 dwStyle,
                                 x, y,
                                 nWidth, nHeight,
                                 hWndParent,
                                 hMenu,
                                 GetModuleHandle( nullptr ),
                                 this );

        return m_hWnd ? true : false;
    }

    BOOL BaseWindow::show( int nCmdShow )
    {
        return ShowWindow( m_hWnd, nCmdShow );
    }

    BOOL BaseWindow::update()
    {
        return UpdateWindow( m_hWnd );
    }

    BOOL BaseWindow::invalidateRect( const RECT* rect, bool erase )
    {
        return InvalidateRect( m_hWnd, rect, erase );
    };

    HWND BaseWindow::getHandle() const
    {
        return m_hWnd;
    }

    void BaseWindow::initClass( WNDCLASSEX& outWndClass ) const
    {
        outWndClass.hInstance = GetModuleHandle( nullptr );
        outWndClass.lpfnWndProc = windowProcedure;
        outWndClass.style = CS_HREDRAW | CS_VREDRAW;
        outWndClass.hbrBackground = reinterpret_cast<HBRUSH>( COLOR_WINDOW + 1 );
        outWndClass.lpszClassName = getClassName();
    }

    ATOM BaseWindow::registerClass() const
    {
        WNDCLASSEX wcex{};
        wcex.cbSize = sizeof( WNDCLASSEX );
        initClass( wcex );

        return RegisterClassEx( &wcex );
    }
}