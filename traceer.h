#pragma once
#if !defined( TRACEER_H_HEADER_GUARD_8af87dbf_e723_4330_b4d7_a44cdc21d779 )
#define TRACEER_H_HEADER_GUARD_8af87dbf_e723_4330_b4d7_a44cdc21d779 1

#if defined( __cplusplus )

#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <type_traits>

#if !defined( VERIFY )
# if defined( NDEBUG )
#  define VERIFY( exp ) (exp)
# else
#  define VERIFY( exp ) assert( (exp) )
# endif /* defined( NDEBUG ) */
#endif /* !defined( VERIFY ) */

#define TRACEER_0(exp) # exp
#define TRACEER_1(exp) TRACEER_0(exp)
#define TRACEER_LINE " (" __FILE__ "@L." TRACEER_1( __LINE__ ) ")\n"

#if !defined( TRACEER )
# if defined( NDEBUG )
#  define TRACEER( fmt, ... ) (void)(TRACEER_LINE , fmt,__VA_ARGS__)
# else  /* defined( NDEBUG ) */
#  define TRACEER( fmt, ... ) traceer::traceer( TRACEER_LINE , fmt , __VA_ARGS__ )
# endif /* defined( NDEBUG ) */
#endif /* !defined( TRACEER ) */

namespace traceer{
  template<size_t n,typename ... Args>
  static inline int traceer( const char (&line)[n], const char* fmt , Args&& ... args )
  {
    char buf[512] = {0};
    do{
      _snprintf_s(buf, _TRUNCATE, fmt, std::forward<Args...>(args)... );
      strcat_s( buf , line );
      OutputDebugStringA( buf );
    }while( false );
    return 0;
  }
  template<size_t n>
  static inline int traceer( const char (&line)[n], const char* fmt )
  {
    char buf[512] = {0};
    do{
      strcpy_s( buf , fmt );
      strcat_s( buf , line );
      OutputDebugStringA( buf );
    }while( false );
    return 0;
  }
  
  template<size_t n,typename ... Args>
  static inline int traceer( const char (&line)[n], const wchar_t* fmt , Args&& ... args )
  {
    wchar_t buf[512] = {0};
    do{
      _swprintf_s( buf ,_TRUNCATE, fmt , std::forward<Args...>( args )... );
      wcscat_s( buf , line );
      OutputDebugStringW( buf );
                                  
    }while( false );
    return 0;
  }

  template<size_t n, typename ... Args>
  static inline int traceer( const char (&line)[n], const wchar_t* fmt )
  {
    wchar_t buf[512] = {0};
    do{
      _snwprintf_s( buf , _TRUNCATE , L"%s%S", fmt , line);
      OutputDebugStringW( buf );
    }while( false );
    return 0;
  }
  
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( TRACEER_H_HEADER_GUARD_8af87dbf_e723_4330_b4d7_a44cdc21d779 ) */
