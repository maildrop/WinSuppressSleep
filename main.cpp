﻿#include "Resource.h"
#include <iostream>
#include <functional>
#include <thread>
#include <cassert>

#include <tchar.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>

#pragma comment(lib, "User32.lib" )
#pragma comment(lib, "Gdi32.lib" )
#pragma comment(lib, "Ole32.lib" )
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' \
name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' \
processorArchitecture='*' \
publicKeyToken='6595b64144ccf1df' \
language='*'\"")

#if !defined( VERIFY )
# if defined( NDEBUG )
#  define VERIFY( exp ) (exp)
# else
#  define VERIFY( exp ) assert( (exp) )
# endif /* defined( NDEBUG ) */
#endif /* !defined( VERIFY ) */

extern "C"{
  /* Private Window Messages */
  enum {
    PWM_TASKTRAY = (WM_APP + 1),
    PWM_INIT,
    PWM_SUPRESS_SUSPEND,
    PWM_SUPRESS_SCREENSAVER,
    PWM_SHUTDOWN,
    PWM_LAST
  };
  struct CreateWindowArgument{
    HICON hIcon;
    HICON hIconSm;
  };
}

class __declspec( uuid("9c8dd510-7260-4928-86b4-19a14e5d9b94") ) ShellTasktrayIcon;

static BOOL AddNotificationIcon( HWND hWnd );
static BOOL DeleteNotificationIcon();
static LRESULT wndproc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );

static BOOL AddNotificationIcon( HWND hWnd )
{
  NOTIFYICONDATA nId = { 0 };
  nId.cbSize = sizeof( nId );
  nId.hWnd = hWnd;
  nId.uID = 0; // use guidItem
  nId.uFlags = NIF_GUID | NIF_ICON /* | NIF_MESSAGE */ ;
  assert( S_OK == LoadIconMetric( GetModuleHandle( NULL ),MAKEINTRESOURCE( IDI_THREADEXEC ) , LIM_SMALL, &nId.hIcon ) );
  nId.uCallbackMessage = PWM_TASKTRAY;
  nId.guidItem = __uuidof( ShellTasktrayIcon );
  Shell_NotifyIcon(NIM_ADD, &nId);

  nId.uVersion = NOTIFYICON_VERSION_4;
  return Shell_NotifyIcon(NIM_SETVERSION, &nId );
}

static BOOL DeleteNotificationIcon()
{
  NOTIFYICONDATA nid = {};
  nid.cbSize = sizeof( nid );
  nid.uFlags = NIF_GUID;
  nid.guidItem = __uuidof( ShellTasktrayIcon );
  return Shell_NotifyIcon(NIM_DELETE, &nid );
}

static LRESULT wndproc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
{
  switch( msg ){
  case PWM_INIT:
    {
      OutputDebugString(TEXT("PWM_INIT\n"));
      PostMessage( hWnd , PWM_SUPRESS_SUSPEND , 0 , 0 );
      
    }
    return 1;
  case PWM_SUPRESS_SUSPEND:
    {
      OutputDebugString(TEXT("PWM_SUPRESS_SUSPEND\n"));
    }
    return 1;
  case PWM_SUPRESS_SCREENSAVER:
    {
      OutputDebugString(TEXT("PWM_SUPRESS_SCREENSAVER\n"));
    }
    return 1;
  case PWM_SHUTDOWN:
    {
      OutputDebugString(TEXT("PWM_SHUTDOWN\n"));
    }
    return 1;
  case PWM_TASKTRAY:
    {
      OutputDebugString( TEXT("PWM_TASKTRAY") );
      
      switch( LOWORD( lParam ) ){
      case WM_CONTEXTMENU:
        {
          POINT const pt = { LOWORD( wParam ) , HIWORD( wParam ) };
          (void)(pt);
        }
        break;
      default:
        break;
      }
    }
    break;
  case WM_CREATE:
    {
      OutputDebugString(TEXT("WM_CREATE\n"));
      if( lParam ){
        const CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>( lParam );
        assert( createStruct );
        if( createStruct->lpCreateParams ){
          const CreateWindowArgument* arg = reinterpret_cast<CreateWindowArgument*>(createStruct->lpCreateParams);
          if( arg ){
            OutputDebugString( TEXT("OK") );
            VERIFY( AddNotificationIcon( hWnd ) );
          }
        }
      }
      VERIFY( PostMessage( hWnd, PWM_INIT , 0 ,0 ) );
    }
    break;
  case WM_CLOSE:
    VERIFY( DestroyWindow( hWnd ) );
    return 0;
  case WM_DESTROY:
    VERIFY( DeleteNotificationIcon() );
    PostQuitMessage( 0 );
    break;
  default:
    break;
  }
  return ::DefWindowProc( hWnd, msg , wParam  , lParam );
}



int wWinMain( HINSTANCE hInstance  , HINSTANCE , PWSTR lpCmdLine , int nCmdShow )
{
  {
    HRESULT const hr = ::CoInitializeEx( NULL , COINIT_MULTITHREADED );
    assert( S_OK == hr );
    if( !SUCCEEDED( hr )){
      return 3;
    }
  }

  struct CoUnInitializeRAII{
    CoUnInitializeRAII(){}
    ~CoUnInitializeRAII()
    {
      ::CoUninitialize();
    }
  };
  
  CoUnInitializeRAII raii{};
  
  std::thread entry(std::bind([]( HINSTANCE hInstance , PWSTR lpCmdLine, int nCmdShow ){
    static_cast<void>( lpCmdLine );
    
    {
      HRESULT const hr = ::CoInitializeEx( NULL,  COINIT_MULTITHREADED );
      assert( S_OK == hr );
      if(! SUCCEEDED( hr )){
        return 3;
      }
    }

    CoUnInitializeRAII raii{};
    HICON appIcon = // LoadIcon( hInstance , MAKEINTRESOURCE( IDI_THREADEXEC ));
      static_cast<HICON>( LoadImage( hInstance ,
                                     MAKEINTRESOURCE( IDI_THREADEXEC ),
                                     IMAGE_ICON ,
                                     GetSystemMetrics( SM_CXICON ),
                                     GetSystemMetrics( SM_CYICON ),
                                     LR_DEFAULTCOLOR ) );
    assert( appIcon);
    
    HICON smIcon = // LoadIcon( hInstance , MAKEINTRESOURCE( IDI_SMALL ));
      static_cast<HICON>( LoadImage( hInstance ,
                                     MAKEINTRESOURCE( IDI_SMALL ),
                                     IMAGE_ICON ,
                                     GetSystemMetrics( SM_CXSMICON ),
                                     GetSystemMetrics( SM_CYSMICON ),
                                     LR_DEFAULTCOLOR ) );
    assert( smIcon );
    
    const WNDCLASSEXW wndClass = {
      .cbSize = sizeof( WNDCLASSEXW ),
      .style = CS_HREDRAW | CS_VREDRAW,
      .lpfnWndProc = wndproc,
      .cbClsExtra = 0,
      .cbWndExtra = 0,
      .hInstance = hInstance,
      .hIcon = appIcon,
      .hCursor = LoadCursor( NULL , IDC_ARROW ),
      .hbrBackground = static_cast<HBRUSH>( GetStockObject( WHITE_BRUSH ) ),
      .lpszMenuName = NULL,
      .lpszClassName = TEXT("threadexecwnd"),
      .hIconSm = smIcon
    };
    const ATOM atom = RegisterClassEx( &wndClass );
    assert( atom );
    if(! atom ){
      return EXIT_FAILURE;
    }
    const struct ATOMRAII {
      ATOM atom ;
      HINSTANCE hInstance;
      ATOMRAII( ATOM atom , HINSTANCE hInstance) : atom(atom),hInstance(hInstance) {}
      ~ATOMRAII()
      {
        VERIFY( UnregisterClass( (LPCTSTR)(atom) , hInstance ) );
      }
    } atomraii( atom,hInstance );

    CreateWindowArgument arg = {0};
    HWND hWnd = CreateWindow( TEXT("threadexecwnd") ,
                              TEXT("電源管理") ,
                              WS_OVERLAPPEDWINDOW ,
                              CW_USEDEFAULT,CW_USEDEFAULT,
                              300,200, NULL ,NULL , hInstance , static_cast<PVOID>(&arg) );
    if( hWnd ){
      ShowWindow( hWnd , nCmdShow );
      UpdateWindow( hWnd );
      BOOL bRet;
      MSG msg = { 0 };
      while(static_cast<bool>((bRet = GetMessage(&msg , NULL , 0 ,0  ) ) )){
        if( -1 == bRet ){
          break;
        }else{
          TranslateMessage( &msg );
          DispatchMessage( &msg );
        }
      }
    }
    return EXIT_SUCCESS;
  }, hInstance,lpCmdLine,nCmdShow ));
  entry.join();
  return EXIT_SUCCESS;
}
