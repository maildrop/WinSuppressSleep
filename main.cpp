#include <iostream>
#include <cassert>
#include <Windows.h>
#include "Resource.h"

#pragma comment(lib, "User32.lib" )
#pragma comment(lib, "Gdi32.lib" )

#if !defined( VERIFY )
# if defined( NDEBUG )
#  define VERIFY( exp ) (exp)
# else
#  define VERIFY( exp ) assert( (exp) )
# endif /* defined( NDEBUG ) */
#endif /* !defined( VERIFY ) */

/* Private Window Messages */
enum {
  PWM_TASKTRAY = (WM_APP + 1),
  PWM_INIT,
  PWM_SUPRESS_SUSPEND,
  PWM_SUPRESS_SCREENSAVER,
  PWM_SHUTDOWN,
  PWM_LAST
};


class __declspec( uuid("9c8dd510-7260-4928-86b4-19a14e5d9b94") ) NormalIcon;


static BOOL AddNotificationIcon( HWND hWnd );
static LRESULT wndproc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );

static BOOL AddNotificationIcon( HWND hWnd )
{
  NOTIFYICONDATA nId = { 0 };
  nId.cbSize = sizeof( nId );
  nId.hWnd = hWnd;
  nId.uID = 0; // use guidItem
  nId.uFlags = NIF_MESSAGE | NIF_GUID;

  nId.uCallbackMessage = PWM_TASKTRAY;
  nId.guidItem = __uuidof( NormalIcon );
  nId.uVersion = NOTIFYICON_VERSION_4;
  return FALSE;
}

static LRESULT wndproc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
{
  switch( msg ){
  case WM_CLOSE:
    VERIFY( DestroyWindow( hWnd ) );
    return 0;
  case WM_DESTROY:
    PostQuitMessage( 0 );
    break;
  case WM_CREATE:
    {
      OutputDebugString(TEXT("WM_CREATE\n"));
      VERIFY( PostMessage( hWnd, PWM_INIT , 0 ,0 ) );
    }
    break;
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
    break;
  default:
    break;
  }
  return ::DefWindowProc( hWnd, msg , wParam  , lParam );
}


int wWinMain( HINSTANCE hInstance  , HINSTANCE , PWSTR /* lpCmdLine */, int nCmdShow )
{
  
                                 
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
  
  HWND hWnd = CreateWindow( TEXT("threadexecwnd") ,
                            TEXT("電源管理") ,
                            WS_OVERLAPPEDWINDOW ,
                            CW_USEDEFAULT,CW_USEDEFAULT,
                            200,200, NULL ,NULL , hInstance , NULL );
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
}
