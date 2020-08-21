#pragma once
#ifndef _SIMD_INSTRUCTION_SET_HPP_
#define _SIMD_INSTRUCTION_SET_HPP_
#include "cpu.hpp"

// Simplified version of boost.predef

// major number can be in [0, 99]
// minor number can be in [0, 99]
// patch number can be in [0, 999999]
#define SIMD_VERSION_NUMBER(major, minor, patch) \
    ((((major) % 100) * 10000000) + (((minor) % 100) * 100000) + ((patch) % 100000))

#define SIMD_VERSION_NUMBER_NOT_AVAILABLE \
    SIMD_VERSION_NUMBER(0, 0, 0)

#define SIMD_VERSION_NUMBER_AVAILABLE \
    SIMD_VERSION_NUMBER(0, 0, 1)

/*************************
 * CLEAR INSTRUCTION SET *
 *************************/

#undef SIMD_X86_INSTR_SET
#undef SIMD_X86_INSTR_SET_AVAILABLE

#undef SIMD_X86_AMD_INSTR_SET
#undef SIMD_X86_AMD_INSTR_SET_AVAILABLE

#undef SIMD_PPC_INSTR_SET
#undef SIMD_PPC_INSTR_SET_AVAILABLE

#undef SIMD_ARM_INSTR_SET
#undef SIMD_ARM_INSTR_SET_AVAILABLE

 /**********************
  * USER CONFIGURATION *
  **********************/

#ifdef SIMD_FORCE_X86_INSTR_SET
#define SIMD_X86_INSTR_SET SIMD_FORCE_X86_INSTR_SET
#define SIMD_X86_INSTR_SET_AVAILABLE SIMD_VERSION_NUMBER_AVAILABLE
#ifdef _MSC_VER
#pragma message("Warning: Forcing X86 instruction set")
#else
#warning "Forcing X86 instruction set"
#endif
#elif defined(SIMD_FORCE_X86_AMD_INSTR_SET)
#define SIMD_X86_AMD_INSTR_SET SIMD_FORCE_X86_AMD_INSTR_SET
#define SIMD_X86_AMD_INSTR_SET_AVAILABLE SIMD_VERSION_NUMBER_AVAILABLE
#ifdef _MSC_VER
#pragma message("Warning: Forcing X86 AMD instruction set")
#else
#warning "Forcing X86 AMD instruction set"
#endif
#elif defined(SIMD_FORCE_PPC_INSTR_SET)
#define SIMD_PPC_INSTR_SET SIMD_FORCE_PPC_INSTR_SET
#define SIMD_PPC_INSTR_SET_AVAILABLE SIMD_VERSION_NUMBER_AVAILABLE
#ifdef _MSC_VER
#pragma message("Warning: Forcing PPC instruction set")
#else
#warning "Forcing PPC instruction set"
#endif
#elif defined(arm)
#define SIMD_ARM_INSTR_SET SIMD_FORCE_ARM_INSTR_SET
#define SIMD_ARM_INSTR_SET_AVAILABLE SIMD_VERSION_NUMBER_AVAILABLE
#ifdef _MSC_VER
#pragma message("Warning: Forcing ARM instruction set")
#else
#warning "Forcing ARM instruction set"
#endif
#endif

#ifdef _MSC_VER
// For MSVC, manually setting!
#if _MSC_VER < 1500 // For VS2005 and lower version
#define __SSE__
#define __SSE2__
#endif
#if _MSC_VER >= 1500 // For VS2008 and VS2010
#define __SSE__
#define __SSE2__
#define __SSE3__
#define __SSSE3__
#endif
#if _MSC_VER >= 1700 // For VS2012 and VS2013
#define __SSE__
#define __SSE2__
#define __SSE3__
#define __SSSE3__
#define __SSE4_1__
#define __SSE4_2__
#endif
#if _MSC_VER >= 1900 // For VS2015(with update3) or later version
#define __SSE__
#define __SSE2__
#define __SSE3__
#define __SSSE3__
#define __SSE4_1__
#define __SSE4_2__
#ifndef __AVX__
#define __AVX__
#endif // !__AVX__
#define __FMA__
#ifndef __AVX2__
#define __AVX2__
#endif // !__AVX2__
#endif
//#if _MSC_VER >= 19?? (vs2019 16.16 LATER)
//define __AVX512__
#endif

/***********************
 * X86 INSTRUCTION SET *
 ***********************/

#define SIMD_X86_SSE_VERSION SIMD_VERSION_NUMBER(1, 0, 0)
#define SIMD_X86_SSE2_VERSION SIMD_VERSION_NUMBER(2, 0, 0)
#define SIMD_X86_SSE3_VERSION SIMD_VERSION_NUMBER(3, 0, 0)
#define SIMD_X86_SSSE3_VERSION SIMD_VERSION_NUMBER(3, 1, 0)
#define SIMD_X86_SSE4_1_VERSION SIMD_VERSION_NUMBER(4, 1, 0)
#define SIMD_X86_SSE4_2_VERSION SIMD_VERSION_NUMBER(4, 2, 0)
#define SIMD_X86_AVX_VERSION SIMD_VERSION_NUMBER(5, 0, 0)
#define SIMD_X86_FMA3_VERSION SIMD_VERSION_NUMBER(5, 2, 0)
#define SIMD_X86_AVX2_VERSION SIMD_VERSION_NUMBER(5, 3, 0)
#define SIMD_X86_AVX512_VERSION SIMD_VERSION_NUMBER(6, 0, 0)
#define SIMD_X86_MIC_VERSION SIMD_VERSION_NUMBER(9, 0, 0)

#if !defined(SIMD_X86_INSTR_SET) && defined(__MIC__)
#define SIMD_X86_INSTR_SET SIMD_X86_MIC_VERSION
#endif

 // AVX512 instructions are supported starting with gcc 6
 // see https://www.gnu.org/software/gcc/gcc-6/changes.html
#if !defined(SIMD_X86_INSTR_SET) && (defined(__AVX512__) || defined(__KNCNI__) || defined(__AVX512F__)\
    && (!defined(__GNUC__) || __GNUC__ >= 6))
#define SIMD_X86_INSTR_SET SIMD_X86_AVX512_VERSION
#endif

#if defined(__AVX512BW__)
#define SIMD_AVX512BW_AVAILABLE 1
#endif

#if !defined(SIMD_X86_INSTR_SET) && defined(__AVX2__)
#define SIMD_X86_INSTR_SET SIMD_X86_AVX2_VERSION
#endif

#if !defined(SIMD_X86_INSTR_SET) && defined(__FMA__)
#define SIMD_X86_INSTR_SET SIMD_X86_FMA3_VERSION
#endif

#if !defined(SIMD_X86_INSTR_SET) && defined(__AVX__)
#define SIMD_X86_INSTR_SET SIMD_X86_AVX_VERSION
#endif

#if !defined(SIMD_X86_INSTR_SET) && defined(__SSE4_2__)
#define SIMD_X86_INSTR_SET SIMD_X86_SSE4_2_VERSION
#endif

#if !defined(SIMD_X86_INSTR_SET) && defined(__SSE4_1__)
#define SIMD_X86_INSTR_SET SIMD_X86_SSE4_1_VERSION
#endif

#if !defined(SIMD_X86_INSTR_SET) && defined(__SSSE3__)
#define SIMD_X86_INSTR_SET SIMD_X86_SSSE3_VERSION
#endif

#if !defined(SIMD_X86_INSTR_SET) && defined(__SSE3__)
#define SIMD_X86_INSTR_SET SIMD_X86_SSE3_VERSION
#endif

#if !defined(SIMD_X86_INSTR_SET) && (defined(__SSE2__) || defined(__x86_64__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
#define SIMD_X86_INSTR_SET SIMD_X86_SSE2_VERSION
#endif

#if !defined(SIMD_X86_INSTR_SET) && (defined(__SSE__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1))
#define SIMD_X86_INSTR_SET SIMD_X86_SSE_VERSION
#endif

#if !(defined SIMD_X86_INSTR_SET)
#define SIMD_X86_INSTR_SET SIMD_VERSION_NUMBER_NOT_AVAILABLE
#else
#define SIMD_X86_INSTR_SET_AVAILABLE SIMD_VERSION_NUMBER_AVAILABLE
#endif

/***************************
 * X86_AMD INSTRUCTION SET *
 ***************************/

#define SIMD_X86_AMD_SSE4A_VERSION SIMD_VERSION_NUMBER(4, 0, 0)
#define SIMD_X86_AMD_FMA4_VERSION SIMD_VERSION_NUMBER(5, 1, 0)
#define SIMD_X86_AMD_XOP_VERSION SIMD_VERSION_NUMBER(5, 1, 1)

#if !defined(SIMD_X86_AMD_INSTR_SET) && defined(__XOP__)
#define SIMD_X86_AMD_INSTR_SET SIMD_X86_AMD_XOP_VERSION
#endif

#if !defined(SIMD_X86_AMD_INSTR_SET) && defined(__FMA4__)
#define SIMD_X86_AMD_INSTR_SET SIMD_X86_AMD_FMA4_VERSION
#endif

#if !defined(SIMD_X86_AMD_INSTR_SET) && defined(__SSE4A__)
#define SIMD_X86_AMD_INSTR_SET SIMD_X86_AMD_SSE4A_VERSION
#endif

#if !defined(SIMD_X86_AMD_INSTR_SET)
#define SIMD_X86_AMD_INSTR_SET SIMD_VERSION_NUMBER_NOT_AVAILABLE
#else
 // X86_AMD implies X86
#if SIMD_X86_INSTR_SET > SIMD_X86_AMD_INSTR_SET
#undef SIMD_X86_AMD_INSTR_SET
#define SIMD_X86_AMD_INSTR_SET SIMD_X86_INSTR_SET
#endif
#define SIMD_X86_AMD_INSTR_SET_AVAILABLE SIMD_VERSION_NUMBER_AVAILABLE
#endif

/***********************
 * PPC INSTRUCTION SET *
 ***********************/

 // We haven't implemented any support for PPC, so we should
 // not enable detection for this instructoin set
 /*#define SIMD_PPC_VMX_VERSION SIMD_VERSION_NUMBER(1, 0, 0)
 #define SIMD_PPC_VSX_VERSION SIMD_VERSION_NUMBER(1, 1, 0)
 #define SIMD_PPC_QPX_VERSION SIMD_VERSION_NUMBER(2, 0, 0)
 #if !defined(SIMD_PPC_INSTR_SET) && defined(__VECTOR4DOUBLE__)
	 #define SIMD_PPC_INSTR_SET SIMD_PPC_QPX_VERSION
 #endif
 #if !defined(SIMD_PPC_INSTR_SET) && defined(__VSX__)
	 #define SIMD_PPC_INSTR_SET SIMD_PPC_VSX_VERSION
 #endif
 #if !defined(SIMD_PPC_INSTR_SET) && (defined(__ALTIVEC__) || defined(__VEC__))
	 #define SIMD_PPC_INSTR_SET SIMD_PPC_VMX_VERSION
 #endif
 #if !defined(SIMD_PPC_INSTR_SET)
	 #define SIMD_PPC_INSTR_SET SIMD_VERSION_NUMBER_NOT_AVAILABLE
 #else
	 #define SIMD_PPC_INSTR_SET_AVAILABLE SIMD_VERSION_NUMBER_AVAILABLE
 #endif*/

 /***********************
  * ARM INSTRUCTION SET *
  ***********************/

#define SIMD_ARM7_NEON_VERSION SIMD_VERSION_NUMBER(7, 0, 0)
#define SIMD_ARM8_32_NEON_VERSION SIMD_VERSION_NUMBER(8, 0, 0)
#define SIMD_ARM8_64_NEON_VERSION SIMD_VERSION_NUMBER(8, 1, 0)

  // TODO __ARM_FEATURE_FMA
#if defined(SIMD_ARM_INSTR_SET) && (defined(__ARM_NEON))
#if __ARM_ARCH >= 8
#if defined(__aarch64__)
#define SIMD_ARM_INSTR_SET SIMD_ARM8_64_NEON_VERSION
#else
#define SIMD_ARM_INSTR_SET SIMD_ARM8_32_NEON_VERSION
#endif
#elif __ARM_ARCH >= 7
#define SIMD_ARM_INSTR_SET SIMD_ARM7_NEON_VERSION
#else
	static_assert("NEON instruction set not supported.", false);
#endif
#endif

#if !defined(SIMD_ARM_INSTR_SET)
#define SIMD_ARM_INSTR_SET SIMD_VERSION_NUMBER_NOT_AVAILABLE
#else
#define SIMD_ARM_INSTR_SET_AVAILABLE SIMD_VERSION_NUMBER_AVAILABLE
#endif

/***************************
 * GENERIC INSTRUCTION SET *
 ***************************/

#undef SIMD_INSTR_SET
#undef SIMD_INSTR_SET_AVAILABLE

#if defined(SIMD_X86_AMD_AVAILABLE)
#if SIMD_X86_INSTR_SET > SIMD_X86_AMD_INSTR_SET
#define SIMD_INSTR_SET SIMD_X86_INSTR_SET
#else
#define SIMD_INSTR_SET SIMD_X86_AMD_INSTR_SET
#endif
#endif

#if !defined(SIMD_INSTR_SET) && defined(SIMD_X86_INSTR_SET_AVAILABLE)
#define SIMD_INSTR_SET SIMD_X86_INSTR_SET
#endif

#if !defined(SIMD_INSTR_SET) && defined(SIMD_PPC_INSTR_SET_AVAILABLE)
#define SIMD_INSTR_SET SIMD_PPC_INSTR_SET
#endif

#if !defined(SIMD_INSTR_SET) && defined(SIMD_ARM_INSTR_SET_AVAILABLE)
#define SIMD_INSTR_SET SIMD_ARM_INSTR_SET
#endif

#if !defined(SIMD_INSTR_SET)
#define SIMD_INSTR_SET SIMD_VERSION_NUMBER_NOT_AVAILABLE
#elif SIMD_INSTR_SET != SIMD_VERSION_NUMBER_NOT_AVAILABLE
#define SIMD_INSTR_SET_AVAILABLE SIMD_VERSION_NUMBER_AVAILABLE
#endif


//*****************DEPTRCATED PREPARE!!!**********//
#if SIMD_X86_INSTR_SET == SIMD_X86_AVX512_VERSION
#define SIMD_TYPE 4
#endif // SIMD_X86_INSTR_SET == SIMD_X86_AVX512_VERSION

#if SIMD_X86_INSTR_SET == SIMD_X86_AVX2_VERSION
#define SIMD_TYPE 3
#endif // SIMD_X86_INSTR_SET == SIMD_X86_AVX2_VERSION

#if SIMD_X86_INSTR_SET == SIMD_X86_AVX_VERSION
#define SIMD_TYPE 2
#endif // SIMD_X86_INSTR_SET == SIMD_X86_AVX_VERSION

#if (SIMD_X86_INSTR_SET >= SIMD_X86_SSE_VERSION) && (SIMD_X86_INSTR_SET <= SIMD_X86_SSE4_2_VERSION)
#define SIMD_TYPE 1
#endif // (SIMD_X86_INSTR_SET >= SIMD_X86_SSE_VERSION) && (SIMD_X86_INSTR_SET <= SIMD_X86_SSE4_2_VERSION)

#if SIMD_X86_INSTR_SET < SIMD_X86_SSE_VERSION
#define SIMD_TYPE 0
#endif // SIMD_X86_INSTR_SET < SIMD_X86_SSE_VERSION

#define SIMDTYPE_NONE 0
#define SIMDTYPE_SSE 1
#define SIMDTYPE_AVX 2
#define SIMDTYPE_AVX2 3
#define SIMDTYPE_AVX512 4
//*****************INCLUDE FILES*********************//


// X86 intruction sets
#if SIMD_X86_INSTR_SET >= SIMD_X86_FMA3_VERSION // FMA3 and later
#ifdef __GNUC__
// x86intrin.h includes header files for whatever instruction
// sets are specified on the compiler command line, such as:
// xopintrin.h, fma4intrin.h
#include <x86intrin.h>         
#else
// MS version of immintrin.h covers AVX, AVX2 and FMA3
#include <immintrin.h> 
#endif // __GNUC__
#elif SIMD_X86_INSTR_SET == SIMD_X86_AVX_VERSION
#include <immintrin.h>             // AVX
#elif SIMD_X86_INSTR_SET == SIMD_X86_SSE4_2_VERSION
#include <nmmintrin.h>             // SSE4.2
#elif SIMD_X86_INSTR_SET == SIMD_X86_SSE4_1_VERSION
#include <smmintrin.h>             // SSE4.1
#elif SIMD_X86_INSTR_SET == SIMD_X86_SSSE3_VERSION
#include <tmmintrin.h>             // SSSE3
#elif SIMD_X86_INSTR_SET == SIMD_X86_SSE3_VERSION
#include <pmmintrin.h>             // SSE3
#elif SIMD_X86_INSTR_SET == SIMD_X86_SSE2_VERSION
#include <emmintrin.h>             // SSE2
#elif SIMD_X8_INSTR_SET == SIMD_X86_SSE_VERSION
#include <xmmintrin.h>             // SSE
#endif // SIMD_X86_INSTR_SET

// AMD instruction sets
#if SIMD_X86_AMD_INSTR_SET >= SIMD_X86_AMD_FMA4_VERSION
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#if SIMD_X86_AMD_INSTR_SET >= SIMD_X86_AMD_XOP_VERSION
#include <xopintrin.h>
#else
#include <fma4intrin.h>
#endif
#endif //  _MSC_VER
#elif SIMD_X86_AMD_INSTR_SET == SIMD_X86_AMD_SSE4A_VERSION
#include <ammintrin.h>
#endif // SIMD_X86_AMD_INSTR_SET

#if SIMD_ARM_INSTR_SET >= SIMD_ARM7_NEON_VERSION
#include <arm_neon.h>
#endif

// TODO: add ALTIVEC instruction set


#endif // !_SIMD_INSTRUCTION_SET_HPP_
