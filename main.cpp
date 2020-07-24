#include "Resource.h"
#include <iostream>
#include <functional>
#include <thread>
#include <cassert>

#include <tchar.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>

#include "traceer.h"

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

extern "C"{
  /* Private Window Messages */
  enum {
    PWM_TASKTRAY = (WM_APP + 1),
    PWM_INIT,
    PWM_SUPPRESS_SUSPEND,
    PWM_SUPPRESS_SCREENSAVER,
    PWM_USER_PRESET,
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
  // TODO: 複数回アプリを立ち上げると、Shellのアイコン管理が上手くいかなくなるのでどうにかする
  NOTIFYICONDATA nid = { 0 };
  nid.cbSize = sizeof( nid );
  nid.hWnd = hWnd;
  nid.uID = 0;
  nid.uFlags = NIF_GUID | NIF_MESSAGE  ;
  if( S_OK == LoadIconMetric( GetModuleHandle( NULL ),MAKEINTRESOURCE( IDI_THREADEXEC ) , LIM_SMALL, &nid.hIcon ) ){
    nid.uFlags |= NIF_ICON;
  }
  nid.uCallbackMessage = PWM_TASKTRAY;
  nid.guidItem = __uuidof( ShellTasktrayIcon );
  if( Shell_NotifyIcon(NIM_ADD, &nid) ){
    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIcon(NIM_SETVERSION, &nid );
  }else{
    return FALSE;
  }
}

static BOOL DeleteNotificationIcon()
{
  NOTIFYICONDATA nid = { 0 };
  nid.cbSize = sizeof( nid );
  nid.uID = 0;
  nid.uFlags = NIF_GUID;
  nid.guidItem = __uuidof( ShellTasktrayIcon );
  return Shell_NotifyIcon(NIM_DELETE, &nid );
}


static void ShowContextMenu( HINSTANCE hInstance, HWND hWnd ,const POINT& pt )
{
  HMENU hMenu = LoadMenu( hInstance , MAKEINTRESOURCE( IDC_CONTEXTMENU ) );
  assert( hMenu );
  if( hMenu ){
    HMENU hSubMenu = GetSubMenu( hMenu , 0 );
    assert( hSubMenu );
    if( hSubMenu ){
      SetForegroundWindow(hWnd);
      /*
        ここで SetForegroundWindow(hWnd) しておくのは、コンテキストメニュー以外の場所をクリックした時に、
        コンテキストメニューが閉じる動作をするため。 hWnd自体が、表示されているかどうかはまた別の問題
       */
      // respect menu drop alignment
      UINT uFlags = TPM_RIGHTBUTTON;
      if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0){
        uFlags |= TPM_RIGHTALIGN;
      }else{
        uFlags |= TPM_LEFTALIGN;
      }
      TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hWnd, NULL);      
    }
    DestroyMenu( hMenu );
  }
  return;
}


static LRESULT wndproc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
{
  switch( msg ){
  case WM_PAINT:
    {
      PAINTSTRUCT ps = {0};
      HDC hdc = ::BeginPaint( hWnd , &ps );
      static_cast<void>( hdc );
      ::EndPaint( hWnd , &ps );
    }
    return 0;
  case WM_COMMAND:
    {
      const int wmId = LOWORD( wParam );
      switch( wmId ){
      case IDM_EXIT:
        DestroyWindow( hWnd );
        return 1;
      case IDM_SUPRESS_SLEEP:
        PostMessage( hWnd , PWM_SUPPRESS_SUSPEND , 0, 0 );
        return 1;
      case IDM_SUPRESS_SCREENSAVER:
        PostMessage( hWnd , PWM_SUPPRESS_SCREENSAVER , 0 , 0);
        return 1;
      case IDM_USER_PRESET:
        PostMessage( hWnd , PWM_USER_PRESET , 0 ,0 );
      default:
        break;
      }
    }
    break;
  case PWM_INIT:
    {
      TRACEER( TEXT("PWM_INIT") );
      PostMessage( hWnd , PWM_SUPPRESS_SUSPEND , 0 , 0 );
    }
    return 1;
  case PWM_SUPPRESS_SUSPEND:
    {
      TRACEER( TEXT("PWM_SUPPRESS_SUSPEND"));
      if( ! SetThreadExecutionState( ES_AWAYMODE_REQUIRED | ES_CONTINUOUS) ){
        MessageBox( hWnd, TEXT("失敗"), TEXT("ERROR") , MB_OK | MB_ICONWARNING );
      }
    }
    return 1;
  case PWM_SUPPRESS_SCREENSAVER:
    {
      TRACEER( TEXT("PWM_SUPPRESS_SCREENSAVER") );
      if( ! SetThreadExecutionState( ES_DISPLAY_REQUIRED | ES_CONTINUOUS )){
        MessageBox( hWnd, TEXT("失敗"), TEXT("ERROR") , MB_OK | MB_ICONWARNING );
      }
    }
    return 1;
  case PWM_USER_PRESET:
    {
      TRACEER( TEXT("PWM_USER_PRESET") );
      if( ! SetThreadExecutionState( ES_CONTINUOUS ) ){
        MessageBox( hWnd , TEXT("失敗"), TEXT("ERROR") , MB_OK | MB_ICONWARNING );
      }
    }
    return 1;
  case PWM_SHUTDOWN:
    {
      TRACEER( TEXT("PWM_SHUTDOWN") );
      if( ! SetThreadExecutionState( ES_CONTINUOUS ) ){
        MessageBox( hWnd, TEXT("失敗" ), TEXT("ERROR") , MB_OK | MB_ICONWARNING );
      }
    }
    return 0;
  case PWM_TASKTRAY:
    {
      TRACEER( TEXT("PWM_TASKTRAY %d") , LOWORD(lParam) );
      switch( LOWORD( lParam ) ){
      case WM_CONTEXTMENU:
        {
          POINT const pt = { LOWORD( wParam ) , HIWORD( wParam ) };
          ShowContextMenu( GetModuleHandle( NULL ) , hWnd , pt );
        }
        break;
      default:
        break;
      }
    }
    return 0;
    
  case WM_CREATE:
    {
      TRACEER( TEXT("WM_CREATE"));
      if( lParam ){
        const CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>( lParam );
        assert( createStruct );
        if( createStruct->lpCreateParams ){
          const CreateWindowArgument* arg = reinterpret_cast<CreateWindowArgument*>(createStruct->lpCreateParams);
          if( arg ){
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
    SendMessage( hWnd, PWM_SHUTDOWN , 0 , 0 );
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

      //ShowWindow( hWnd , nCmdShow );
      //UpdateWindow( hWnd );
      
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
