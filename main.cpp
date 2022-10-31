#include "Resource.h"
#include <iostream>
#include <locale>
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

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern "C"{
  /* Private Window Messages */
  enum {
    PWM_TASKTRAY = (WM_APP + 1),
    PWM_INIT,
    PWM_SUPPRESS_SUSPEND,
    PWM_SUPPRESS_SCREENSAVER,
    PWM_USER_PRESET,
    PWM_SHUTDOWN,
    PWM_SHUTDOWN_IMPL,
    PWM_LAST
  };
  struct CreateWindowArgument{
    HICON hIcon;
    HICON hIconSm;
  };
}

class __declspec( uuid("7e1314e5-3d70-4445-89b1-25f2ff785722") ) ShellTasktrayIcon;

static BOOL AddNotificationIcon( HWND hWnd );
static BOOL DeleteNotificationIcon();
static void ShowContextMenu( HINSTANCE hInstance, HWND hWnd ,const POINT& pt );
static LRESULT wndproc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );

static BOOL AddNotificationIcon( HWND hWnd )
{
  NOTIFYICONDATA nid = { 0 };
  nid.cbSize = sizeof( nid );
  nid.uVersion = NOTIFYICON_VERSION_4;
  nid.hWnd = hWnd;
  nid.uFlags = NIF_GUID | NIF_MESSAGE | NIF_ICON ;
  {
    nid.guidItem = __uuidof( ShellTasktrayIcon );
    nid.uFlags |= NIF_GUID;
  }
  {
    nid.uCallbackMessage = PWM_TASKTRAY;
    nid.uFlags |= NIF_MESSAGE;
  }
  
  if( S_OK == LoadIconMetric( GetModuleHandle( NULL ),MAKEINTRESOURCE( IDI_THREADEXEC ) , LIM_SMALL, &nid.hIcon ) ){
    nid.uFlags |= NIF_ICON;
  }

  if( Shell_NotifyIcon(NIM_ADD, &nid) ){
    return Shell_NotifyIcon(NIM_SETVERSION, &nid );
  }else{
    assert(!"Shell_NotifyIcon( NIM_ADD, &nid) failed" );
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

struct DecorateContextMenu{
  const HWND hWnd;
  DecorateContextMenu( HWND hWnd ) : hWnd( hWnd ){
  };
  BOOL operator()(HMENU hContextMenu) const 
  {
    (void)( hContextMenu );
    return TRUE;
  }
};

template<typename Decorator_t , int RESOURCE_MENU_ID, int RESOURCE_SUBMENU_ID = 0> 
void ShowContextMenu(HWND hWnd , const POINT& pt ,const Decorator_t& dec)
{
  HINSTANCE const hInstance{ reinterpret_cast<HINSTANCE>(GetWindowLongPtr( hWnd , GWLP_HINSTANCE )) };
  HMENU const hMenu = LoadMenu( hInstance , MAKEINTRESOURCE( RESOURCE_MENU_ID ) );
  assert( hMenu );
  if( hMenu ){
    HMENU const hSubMenu = GetSubMenu( hMenu , RESOURCE_MENU_ID );
    assert( hSubMenu );
    if( hSubMenu ){

      // コンテキストメニューの装飾を行う
      VERIFY( dec( hSubMenu ) );

      // ウィンドウを前面に持ってくる。このウィンドウがフォーカスを失ったときに（コンテキストメニュー以外にフォーカスが当たった時に）コンテキストメニューを閉じるため。
      // そしてSetForegroundWindow は失敗することがあるが、特に気にしない。
      (void)(SetForegroundWindow( hWnd )); 

      // respect menu drop alignment
      UINT uFlags = TPM_RIGHTBUTTON;
      if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0){
        uFlags |= TPM_RIGHTALIGN;
      }else{
        uFlags |= TPM_LEFTALIGN;
      }
      VERIFY( TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hWnd, NULL) );
    }
  }
  return;
};

static void ShowContextMenu( HINSTANCE hInstance, HWND hWnd ,const POINT& pt )
{
  // この hInstance は、Windowクラスの hInstance と同じ 
  assert( hInstance == reinterpret_cast<HINSTANCE>( GetWindowLongPtr( hWnd , GWLP_HINSTANCE ) ));
  HMENU hMenu = LoadMenu( hInstance , MAKEINTRESOURCE( IDC_CONTEXTMENU ) );
  assert( hMenu );
  if( hMenu ){
    HMENU hSubMenu = GetSubMenu( hMenu , 0 );
    assert( hSubMenu );
    if( hSubMenu ){

      // respect menu drop alignment
      UINT uFlags = TPM_RIGHTBUTTON;
      if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0){
        uFlags |= TPM_RIGHTALIGN;
      }else{
        uFlags |= TPM_LEFTALIGN;
      }

      MENUITEMINFO menuItemInfo = {
        .cbSize = sizeof( MENUITEMINFO ) ,
        .fMask  = MIIM_STATE };
      
      switch( (uintptr_t)GetProp( hWnd, TEXT("State") ) ){
      case PWM_SUPPRESS_SUSPEND:
        menuItemInfo.fState = 0;
        if( GetMenuItemInfo( hSubMenu , IDM_SUPRESS_SLEEP , FALSE , &menuItemInfo ) ){
          menuItemInfo.fState |= MFS_CHECKED;
          VERIFY( SetMenuItemInfo( hSubMenu , IDM_SUPRESS_SLEEP , FALSE , &menuItemInfo ));
        }
        break;
      case PWM_SUPPRESS_SCREENSAVER:
        menuItemInfo.fState = 0;
        if( GetMenuItemInfo( hSubMenu , IDM_SUPRESS_SCREENSAVER , FALSE , &menuItemInfo )){
          menuItemInfo.fState |= MFS_CHECKED;
          VERIFY( SetMenuItemInfo( hSubMenu , IDM_SUPRESS_SCREENSAVER , FALSE , &menuItemInfo ));
        }
        break;
      case PWM_USER_PRESET:
        menuItemInfo.fState = 0;
        if( GetMenuItemInfo( hSubMenu , IDM_USER_PRESET , FALSE , &menuItemInfo ) ){
          menuItemInfo.fState |= MFS_CHECKED ;
          VERIFY( SetMenuItemInfo( hSubMenu , IDM_USER_PRESET , FALSE , &menuItemInfo ));
        }
        break;
      default:
        break;
      }

      /*
        ここで SetForegroundWindow(hWnd) しておくのは、コンテキストメニュー以外の場所をクリックした時に、
        コンテキストメニューが閉じる動作をするため。 hWnd自体が、表示されているかどうかはまた別の問題
       */
      (void)(SetForegroundWindow(hWnd));
      VERIFY( TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hWnd, NULL) );
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
        SendMessage( hWnd, PWM_SHUTDOWN , 0 , 0 ); // ウィンドウを閉じる
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
      SendMessage( hWnd , PWM_SUPPRESS_SUSPEND , 0 , 0 );
      if(! AddNotificationIcon( hWnd ) ){
        PostMessage( hWnd , PWM_SHUTDOWN_IMPL , 0 , 0);
      }
    }
    return 1;
  case PWM_SUPPRESS_SUSPEND:
    do{
      TRACEER( TEXT("PWM_SUPPRESS_SUSPEND"));
      if( ! SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED) ){
        MessageBox( hWnd, TEXT("失敗"), TEXT("ERROR") , MB_OK | MB_ICONWARNING );
        break;
      }
      VERIFY(SetProp( hWnd , TEXT("State") , (HANDLE)(PWM_SUPPRESS_SUSPEND)));
    }while( false );
    return 1;
  case PWM_SUPPRESS_SCREENSAVER:
    do{
      TRACEER( TEXT("PWM_SUPPRESS_SCREENSAVER") );
      if( ! SetThreadExecutionState( ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED )){
        MessageBox( hWnd, TEXT("失敗"), TEXT("ERROR") , MB_OK | MB_ICONWARNING );
        break;
      }
      VERIFY(SetProp( hWnd , TEXT("State") , (HANDLE)(PWM_SUPPRESS_SCREENSAVER)));
    }while( false );
    return 1;
  case PWM_USER_PRESET:
    do{
      TRACEER( TEXT("PWM_USER_PRESET") );
      if( ! SetThreadExecutionState( ES_CONTINUOUS ) ){
        MessageBox( hWnd , TEXT("失敗"), TEXT("ERROR") , MB_OK | MB_ICONWARNING );
        break;
      }
      VERIFY(SetProp( hWnd , TEXT("State") , (HANDLE)(PWM_USER_PRESET)));
    }while( false );
    return 1;
  case PWM_SHUTDOWN:
    {
      TRACEER( TEXT("PWM_SHUTDOWN") );
      VERIFY( DeleteNotificationIcon() ); // アイコンを消して
    }
    // tear down 
  case PWM_SHUTDOWN_IMPL:
    {
      TRACEER( TEXT("PWM_SHUTDOWN_IMPL") );
      if( ! SetThreadExecutionState( ES_CONTINUOUS ) ){
        MessageBox( hWnd, TEXT("失敗" ), TEXT("ERROR") , MB_OK | MB_ICONWARNING );
      }
      DestroyWindow( hWnd );
    }
    return 0;
  case PWM_TASKTRAY:
    {
      TRACEER( TEXT("PWM_TASKTRAY %d") , LOWORD(lParam) );
      switch( LOWORD( lParam ) ){
      case WM_CONTEXTMENU:
        {
          POINT const pt = { LOWORD( wParam ) , HIWORD( wParam ) };
          ShowContextMenu( reinterpret_cast<HINSTANCE>(GetWindowLongPtr( hWnd , GWLP_HINSTANCE )) , hWnd , pt );
        }
        break;
      default:
        break;
      }
    }
    return 0;
    
  case WM_CREATE:
    do{
      TRACEER( TEXT("WM_CREATE"));
      if(! lParam ){
        goto HAS_ERROR;
      }

      {
        const CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>( lParam );
        assert( createStruct );
        if( !createStruct->lpCreateParams ){
          goto HAS_ERROR;
        }
      
        {
          const CreateWindowArgument* arg = reinterpret_cast<CreateWindowArgument*>(createStruct->lpCreateParams);
          if(! arg ){
            goto HAS_ERROR;
          }
        }
      }
      VERIFY( PostMessage( hWnd, PWM_INIT , 0 ,0 ) );
      
      break;
    HAS_ERROR:
      return -1;
    }while( false );
    break;
  case WM_DESTROY:
    PostQuitMessage( 0 );
    break;
  case WM_DPICHANGED:
    // DPI 変更がされました。
    TRACEER( TEXT("WM_DPI_CHANGED( wParam HIWORD=%hu LOWORD=%hu )" ),
             HIWORD( wParam ) , LOWORD( wParam ) );
    return ::DefWindowProc( hWnd , msg , wParam , lParam );
  default:
    break;
  }
  return ::DefWindowProc( hWnd, msg , wParam  , lParam );
}

int wWinMain( HINSTANCE hInstance  , HINSTANCE , PWSTR lpCmdLine , int nCmdShow )
{
  struct CoUnInitializeRAII{
    CoUnInitializeRAII(){}
    ~CoUnInitializeRAII()
    {
      ::CoUninitialize();
    }
  };

  std::locale::global( std::locale(""));

  /* CoInitializeEx and CoUninitialize */
  {
    HRESULT const hr = ::CoInitializeEx( NULL , COINIT_MULTITHREADED );
    assert( S_OK == hr );
    if( !SUCCEEDED( hr )){
      return 3;
    }
  }
  struct CoUnInitializeRAII raii{};
  

  /* entry point */
  std::thread entry(std::bind([]( HINSTANCE hInstance , PWSTR lpCmdLine, int nCmdShow ){
    static_cast<void>( lpCmdLine );
    static_cast<void>( nCmdShow );
    
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
    if( !hWnd ){
      return 1;
    }
    
#if 0
    {
      // High DPI に対応するために
      TRACEER( TEXT("GetDpiForWindow() = %u"),GetDpiForWindow( hWnd ) );
      TRACEER( TEXT("SM_CXICON = %u, SM_CYICON = %u"),
               GetSystemMetricsForDpi( SM_CXICON , GetDpiForWindow( hWnd ) ),
               GetSystemMetricsForDpi( SM_CYICON , GetDpiForWindow( hWnd ) ));
      // ShowWindow( hWnd , nCmdShow );
      // UpdateWindow( hWnd );
    }
#endif /* 0 */
    {
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
