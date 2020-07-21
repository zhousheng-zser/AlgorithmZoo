#pragma once
#ifndef _COMPILER_HPP_
#define _COMPILER_HPP_

// Compilers and C++0x/C++11/C++14/C++17 Evaluation
#if __cplusplus >= 201103L
#	define G6_CXX0X 1
#  define G6_CXX11 1
#else
#	define G6_CXX0X 1
#endif  // __cplusplus >= 201103L
#if __cplusplus >= 201402L
#	define G6_CXX0X 1
#  define G6_CXX11 1
#  define G6_CXX14 1
#endif  // __cplusplus >= 201402L
#if __cplusplus >= 201703L
#	define G6_CXX0X 1
#  define G6_CXX11 1
#  define G6_CXX14 1
#  define G6_CXX17 1
#endif  // __cplusplus >= 201703L

#if (_MSC_VER >= 1600)
#    define G6_CXX0X 1
#endif
#if(_MSC_VER >= 1700)
#    define G6_CXX11 1
#endif
#if(_MSC_VER >= 1800)
#    define G6_CXX14 1
#endif
#if(_MSC_VER >= 1900)
#    define G6_CXX17 1
#endif


#endif // !_COMPILER_HPP_
