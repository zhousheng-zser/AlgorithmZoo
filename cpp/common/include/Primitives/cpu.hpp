#pragma once
#ifndef _CPU_HPP_
#define _CPU_HPP_

#include <string>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef __ANDROID__ // Android
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#endif

#if __APPLE__ // iOS
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/machine.h>
#define __IOS__ 1
#endif
#endif

#ifdef _MSC_VER
#if defined(_M_IX86) || defined(_M_X64)
#define x86
#endif
#elif defined(__GNUC__)
#if defined(__i386__) || defined(__x86_64__)
#define x86
#else
#define arm
#endif
#endif

#ifdef x86
//This file is written according to <Intel?64 and IA-32 Architectures Software Developer's Manual(Volum 2A: Instruction Set Reference)>
#if defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
#    if defined(__PIC__) && defined(__i386__)
	// Case for x86 with PIC
#      define __cpuidex(abcd,func,id) \
         __asm__ __volatile__ ("xchgl %%ebx, %k1;cpuid; xchgl %%ebx,%k1": "=a" (abcd[0]), "=&r" (abcd[1]), "=c" (abcd[2]), "=d" (abcd[3]) : "a" (func), "c" (id));
#    elif defined(__PIC__) && defined(__x86_64__)
	// Case for x64 with PIC. In theory this is only a problem with recent gcc and with medium or large code model, not with the default small code model.
	// However, we cannot detect which code model is used, and the xchg overhead is negligible anyway.
#      define __cpuidex(abcd,func,id) \
        __asm__ __volatile__ ("xchg{q}\t{%%}rbx, %q1; cpuid; xchg{q}\t{%%}rbx, %q1": "=a" (abcd[0]), "=&r" (abcd[1]), "=c" (abcd[2]), "=d" (abcd[3]) : "0" (func), "2" (id));
#    else
	// Case for x86_64 or x86 w/o PIC
#      define __cpuidex(abcd,func,id) \
         __asm__ __volatile__ ("cpuid": "=a" (abcd[0]), "=b" (abcd[1]), "=c" (abcd[2]), "=d" (abcd[3]) : "0" (func), "2" (id) );
#    endif
#  elif defined(_MSC_VER)
#    if (_MSC_VER  < 1500) && ( defined(_M_IX86) || defined(_M_X64) )
#      define __cpuidex(abcd,func,id) __cpuidex((int*)abcd,func,id)
#    endif
#  endif
#endif //!x86

namespace glasssix
{
#ifdef x86
	/// Return CPU vendor infomation:\n
	/// GenuineIntel: Intel\n
	/// AMDisbetter! or AuthenticAMD: AMD\n
	/// CyrixInstead: Cyrix\n
	/// CentaurHauls: Centaur\n
	/// GenuineTMx86 or TransmetaCPU: Transmeta\n
	/// Geode by NSC: National Semiconductor\n
	/// RiseRiseRise: Rise\n
	/// SiS SiS SiS: SiS\n
	/// UMC UMC UMC: UMC\n
	/// NexGenDriven: NexGen\n
	std::string cpu_vendor();

	/// Judge the CPU is INTEL series
	bool isIntel();

	/// Judge the CPU is AMD series
	bool isAMD();

	/// For specific cpu infomation
	std::string cpu_brand();

	//****** SIMD and other instructions support situation ********//

	/// Judge if the CPU support MMX instruction
	bool support_MMX();

	/// Judge if the CPU support SSE instruction
	bool support_SSE();

	/// Judge if the CPU support SSE2 instruction
	bool support_SSE2();

	/// Judge if the CPU support SSE3 instruction
	bool support_SSE3();

	/// Judge if the CPU support SSSE3 instruction
	bool support_SSSE3();

	/// Judge if the CPU support FMA instruction
	bool support_FMA();

	/// Judge if the CPU support SSE4.1 instruction
	bool support_SSE41();

	/// Judge if the CPU support SSE4.2 instruction
	bool support_SSE42();

	/// Judge if the CPU support AVX instruction
	bool support_AVX();

	/// Judge if the CPU support F16C instruction
	bool support_F16C();

	/// Judge if the CPU support AVX2 instruction
	bool support_AVX2();

	/// Judge if the CPU support AVX512F instruction
	bool support_AVX512F();

	/// Judge if the CPU support AVX512PF instruction
	/// Intel?Xeon Phi only
	bool support_AVX512PF();

	/// Judge if the CPU support AVX512ER instruction
	/// Intel?Xeon Phi only
	bool support_AVX512ER();

	/// Judge if the CPU support AVX512CD instruction
	bool support_AVX512CD();

	/// Query the cache size of the CPU:\n
	/// For L1	cache, it returns size for each logical cores(Hyper Threading Technology) in Bytes;\n
	/// For L2	cache, it returns size for each physical cores in Bytes;\n
	/// For L3	cache, it returns size for the whole CPU in Bytes;\n
	void queryCacheSizes(int& l1, int& l2, int& l3);

	/// Query the number of the logical processor of the CPU
	unsigned int queryLogicalProcessors();
#endif //!x86

#ifdef arm
	// test optional cpu features
	// neon = armv7 neon or aarch64 asimd
	int cpu_support_arm_neon();
	// vfpv4 = armv7 fp16 + fma
	int cpu_support_arm_vfpv4();
	// asimdhp = aarch64 asimd half precision
	int cpu_support_arm_asimdhp();

	// cpu info
	int get_cpu_count();

	/// bind all threads on little clusters if powersave enabled
	/// affacts HMP arch cpu like ARM big.LITTLE
	/// only implemented on android at the moment
	/// switching powersave is expensive and not thread-safe\n
	/// 0 = all cores enabled(default)\n
	/// 1 = only little clusters enabled\n
	/// 2 = only big clusters enabled\n
	/// return 0 if success for setter function\n
	int get_cpu_powersave();
	int set_cpu_powersave(int powersave);

#endif //!arm


	//************* misc function wrapper for openmp routines *****************//

	/// get openmp threads number
	int get_omp_num_threads();

	/// set openmp threads number
	void set_omp_num_threads(int num_threads);

	int get_omp_dynamic();

	void set_omp_dynamic(int dynamic);
}

#endif // !_CPU_HPP_
