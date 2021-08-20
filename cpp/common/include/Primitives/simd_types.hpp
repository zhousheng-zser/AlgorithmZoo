#pragma once
#ifndef _SIMD_TYPES_HPP_
#define _SIMD_TYPES_HPP_

#include "simd_instruction_set.hpp"
#include "logger.hpp"
#include "dllexport.hpp"

#ifdef __GNUC__
#define __forceinline inline __attribute__((always_inline))
#if (SIMD_X86_INSTR_SET >= SIMD_X86_SSE2_VERSION)
#define _mm_loadu_si64(p) _mm_loadl_epi64((__m128i const *)(p))
#define _mm_storeu_si64(p, a) (_mm_storel_epi64((__m128i *)(p), (a)))
#endif
#endif

namespace glasssix
{
#define NATIVE_CODE_WARNING LOG(WARNING) << "Unhandled Scenario: fall back to native code with extremely low performance."

#if (SIMD_X86_INSTR_SET >= SIMD_X86_SSE_VERSION) && (SIMD_X86_INSTR_SET <= SIMD_X86_SSE4_2_VERSION) //SSE
#define mm_load_ps _mm_loadu_ps
#define mm_store_ps _mm_storeu_ps
#define mm_set1_ps _mm_set1_ps
#define mm_setzero_ps _mm_setzero_ps
#define mm_add_ps _mm_add_ps
#define mm_sub_ps _mm_sub_ps
#define mm_mul_ps _mm_mul_ps
#define mm_type __m128
#define mm_typei __m128i
#define mm_round_ps _mm_round_ps
#define mm_load_si _mm_load_si128
#define mm_cvtepi32_ps _mm_cvtepi32_ps
#define mm_cvtps_epi32 _mm_cvtps_epi32
#define mm_cvtepu8_epi32 _mm_cvtepu8_epi32
#define mm_cvtepi8_epi32 _mm_cvtepi8_epi32
#define mm_cvtepi16_epi32 _mm_cvtepi16_epi32
#define mm_mullo_epi16 _mm_mullo_epi16
#define mm_mullo_epi32 _mm_mullo_epi32
#define mm_store_si _mm_store_si128
#define mm_setzero_si _mm_setzero_si128
#define mm_add_epi32 _mm_add_epi32
#define mm_align_size 4
#define mm_align_size2 8
#define mm_align_size3 12
#define mm_align_size4 16
#define mm_align_size5 20
#define mm_align_size6 24
#define mm_align_size7 28
#define mm_align_size8 32
#define simd_registers 16

    __forceinline __m128i _mm_madd_epi16_epi32(const __m128i a, const __m128i b, __m128i c)
    {
        const __m128i a_int16 = _mm_cvtepi8_epi16(a);
        const __m128i b_int16 = _mm_cvtepi8_epi16(b);
        const __m128i product_int16 = _mm_mullo_epi16(a_int16, b_int16);
        short product_int16_array[8];
        _mm_store_si128((__m128i *)product_int16_array, product_int16);
        const __m128i product_h = _mm_load_si128((__m128i *)product_int16_array);
        const __m128i product_l = _mm_load_si128((__m128i *)(product_int16_array + 4));
        const __m128i product_h_int32 = _mm_cvtepi16_epi32(product_h);
        const __m128i product_l_int32 = _mm_cvtepi16_epi32(product_l);
        return _mm_add_epi32(_mm_add_epi32(product_l_int32, product_h_int32), c);
    }

    __forceinline int _mm_sumall_epi32(const __m128i re)
    {
        int temp_sum[4];
        _mm_store_si128((__m128i *)temp_sum, re);
        return temp_sum[0] + temp_sum[1] + temp_sum[2] + temp_sum[3];
    }
    union union_type_s_mm128
    {
        double d[2];
        float s[4];
        __m128 v;
        __m64 t[2];
    };
#define q_type \
    union union_type_s_mm128
#define store_to_q(x, y) \
    _mm_store_ps(x, y)
#define mm128_final_ssum_quarter(q) (q.s[0])
#define mm128_final_ssum_half(q) (q.s[0] + q.s[1])
#define mm128_final_ssum_all(q) (q.s[0] + q.s[1] + q.s[2] + q.s[3])
    __forceinline float _mm_sumall_ps(__m128 r)
    {
        union_type_s_mm128 q = {0};
        _mm_store_ps(q.s, r);
        return (q.s[0] + q.s[1]) + (q.s[2] + q.s[3]);
    }
#define mm_sumall_ps _mm_sumall_ps
#if USE_FMADD128
#define mm_fmadd_ps _mm_fmadd_ps
#else
#define mm_fmadd_ps(A, B, C) _mm_add_ps(_mm_mul_ps(A, B), C)
#endif

    static float mul_add_3x3_simd(__m128 r0_data, __m128 r1_data, __m128 r2_data, __m128 k0_data, __m128 k1_data, __m128 k2_data, float bias)
    {
        float sum_sum = bias;
        __m128 sum = _mm_setzero_ps();

#if USE_FMADD128
        sum = _mm_fmadd_ps(r0_data, k0_data, sum);
        sum = _mm_fmadd_ps(r1_data, k1_data, sum);
        sum = _mm_fmadd_ps(r2_data, k2_data, sum);
        //sum_sum += sum.m128_f32[0] + sum.m128_f32[1] + sum.m128_f32[2];
#else
        sum = _mm_add_ps(_mm_mul_ps(r0_data, k0_data), sum);
        sum = _mm_add_ps(_mm_mul_ps(r1_data, k1_data), sum);
        sum = _mm_add_ps(_mm_mul_ps(r2_data, k2_data), sum);
        //sum_sum += sum.m128_f32[0] + sum.m128_f32[1] + sum.m128_f32[2];
#endif

        float temp[4];
        _mm_storeu_ps(temp, sum);
        for (int i = 0; i < 3; i++)
        {
            sum_sum += temp[i];
        }

        return sum_sum;
    }
#elif (SIMD_X86_INSTR_SET >= SIMD_X86_AVX_VERSION) && (SIMD_X86_INSTR_SET <= SIMD_X86_AVX2_VERSION) //AVX
#define mm_load_ps _mm256_loadu_ps
#define mm_store_ps _mm256_storeu_ps
#define mm_set1_ps _mm256_set1_ps
#define mm_setzero_ps _mm256_setzero_ps
#define mm_add_ps _mm256_add_ps
#define mm_sub_ps _mm256_sub_ps
#define mm_mul_ps _mm256_mul_ps
#define mm_type __m256
#define mm_typei __m256i
#define mm_round_ps _mm256_round_ps
#define mm_load_si _mm256_load_si256
#define mm_cvtepi32_ps _mm256_cvtepi32_ps
#define mm_cvtps_epi32 _mm256_cvtps_epi32
#define mm_cvtepu8_epi32 _mm256_cvtepu8_epi32
#define mm_cvtepi8_epi32 _mm256_cvtepi8_epi32
#define mm_cvtepi16_epi32 _mm256_cvtepi16_epi32
#define mm_mullo_epi16 _mm256_mullo_epi16
#define mm_mullo_epi32 _mm256_mullo_epi32
#define mm_store_si _mm256_store_si256
#define mm_setzero_si _mm256_setzero_si256
#define mm_add_epi32 _mm256_add_epi32
#define mm_align_size 8
#define mm_align_size2 16
#define mm_align_size3 24
#define mm_align_size4 32
#define mm_align_size5 40
#define mm_align_size6 48
#define mm_align_size7 56
#define mm_align_size8 64
#define simd_registers 16

    __forceinline __m256i _mm256_madd_epi16_epi32(const __m128i a, const __m128i b, __m256i c)
    {
        const __m256i a_int16 = _mm256_cvtepi8_epi16(a);
        const __m256i b_int16 = _mm256_cvtepi8_epi16(b);
        const __m256i product_int16 = _mm256_mullo_epi16(a_int16, b_int16);
        /*const __m128i product_h = _mm256_extractf128_si256(product_int16, 1);
	const __m128i product_l = _mm256_extractf128_si256(product_int16, 0);*/
        short product_int16_array[16];
        _mm256_store_si256((__m256i *)product_int16_array, product_int16);
        const __m128i product_h = _mm_load_si128((__m128i *)product_int16_array);
        const __m128i product_l = _mm_load_si128((__m128i *)(product_int16_array + 8));
        const __m256i product_h_int32 = _mm256_cvtepi16_epi32(product_h);
        const __m256i product_l_int32 = _mm256_cvtepi16_epi32(product_l);
        return _mm256_add_epi32(_mm256_add_epi32(product_l_int32, product_h_int32), c);
    }

    __forceinline int _mm256_sumall_epi32(const __m256i re)
    {
        int temp_sum[8];
        _mm256_store_si256((__m256i *)temp_sum, re);
        return temp_sum[0] + temp_sum[1] + temp_sum[2] + temp_sum[3] + temp_sum[4] + temp_sum[5] + temp_sum[6] + temp_sum[7];
    }

    __forceinline float _mm256_sum_ps(const __m256 re)
    {
        float temp_sum[8];
        _mm256_storeu_ps(temp_sum, re);
        return temp_sum[0] + temp_sum[1] + temp_sum[2] + temp_sum[3] + temp_sum[4] + temp_sum[5] + temp_sum[6] + temp_sum[7];
    }

    __forceinline __m256i _mm256_add_epi16_epi32(const __m128i a, const __m128i b, __m256i c)
    {
        const __m128i a_int16 = _mm_cvtepi8_epi16(a);
        const __m128i b_int16 = _mm_cvtepi8_epi16(b);
        const __m128i product_int16 = _mm_mullo_epi16(a_int16, b_int16);
        short product_int16_array[8];
        _mm_store_si128((__m128i *)product_int16_array, product_int16);
        const __m128i product_h = _mm_load_si128((__m128i *)product_int16_array);
        const __m256i product_h_int32 = _mm256_cvtepi16_epi32(product_h);
        return _mm256_add_epi32(product_h_int32, c);
    }

    __forceinline __m256i _mm256__epi32(const __m128i a, const __m128i b, __m256i c)
    {
        const __m256i producta_int32 = _mm256_cvtepi16_epi32(a);
        const __m256i productb_int32 = _mm256_cvtepi16_epi32(b);
        __m256i product_h = _mm256_mullo_epi32(producta_int32, productb_int32);
        return _mm256_add_epi32(product_h, c);
    }

    union union_type_s_mm128
    {
        double d[2];
        float s[4];
        __m128 v;
        __m64 t[2];
    };

    union union_type_s_mm256
    {
        double d[4];
        float s[8];
        __m256 v;
        __m128 p[2];
        __m64 t[4];
    };
//#define q_type \
//	union union_type_s_mm256
//
//#define store_to_q(x,y)\
//	_mm256_store_ps(x,y)
#define mm128_final_ssum_quarter(q) (q.s[0])
#define mm128_final_ssum_half(q) (q.s[0] + q.s[1])
#define mm128_final_ssum_all(q) (q.s[0] + q.s[1] + q.s[2] + q.s[3])
#define mm256_final_ssum_quarter(q) (q.s[0] + q.s[1])
#define mm256_final_ssum_half(q) (q.s[0] + q.s[1] + q.s[2] + q.s[3])
#define mm256_final_ssum_all(q) (q.s[0] + q.s[1] + q.s[2] + q.s[3] + q.s[4] + q.s[5] + q.s[6] + q.s[7])
    __forceinline float _mm_sumall_ps(__m128 r)
    {
        float s[4] = {0};
        _mm_store_ps(s, r);
        return (s[0] + s[1]) + (s[2] + s[3]);
    }
    __forceinline float _mm256_sumall_ps(__m256 r)
    {
        __m128 h = _mm256_extractf128_ps(r, 1);
        __m128 l = _mm256_extractf128_ps(r, 0);
        h = _mm_add_ps(h, l);
        return _mm_sumall_ps(h);
    }
#define mm_sumall_ps _mm256_sumall_ps
#if SIMD_X86_INSTR_SET >= SIMD_X86_FMA3_VERSION
#define mm_fmadd_ps _mm256_fmadd_ps
#else
#define mm_fmadd_ps(A, B, C) _mm256_add_ps(_mm256_mul_ps(A, B), C)
#endif // !FMA3

#elif SIMD_X86_INSTR_SET > SIMD_X86_AVX2_VERSION // AVX512
#define mm_load_ps _mm512_loadu_ps
#define mm_store_ps _mm512_storeu_ps
#define mm_set1_ps _mm512_set1_ps
#define mm_setzero_ps _mm512_setzero_ps
#define mm_add_ps _mm512_add_ps
#define mm_mul_ps _mm512_mul_ps
#define mm_type __m512
#define mm_align_size 16
#define mm_align_size2 32
#define mm_align_size3 48
#define mm_align_size4 64
#define mm_align_size5 80
#define mm_align_size6 96
#define mm_align_size7 112
#define mm_align_size8 128
#define simd_registers 32
    union union_type_s_mm512
    {
        double d[8];
        float s[16];
        __m512 r;
        __m256 v[2];
        __m128 p[4];
        __m64 t[8];
    };
#define q_type \
    union union_type_s_mm512

#define store_to_q(x, y) \
    _mm512_store_ps(x, y)

#define mm_final_ssum_quarter(q) (q.s[0] + q.s[1] + q.s[2] + q.s[3])
#define mm_final_ssum_half(q) (q.s[0] + q.s[1] + q.s[2] + q.s[3] + q.s[4] + q.s[5] + q.s[6] + q.s[7])
#define mm_final_ssum_all(q) (q.s[0] + q.s[1] + q.s[2] + q.s[3] + q.s[4] + q.s[5] + q.s[6] + q.s[7] + \
                              q.s[8] + q.s[9] + q.s[10] + q.s[11] + q.s[12] + q.s[13] + q.s[14] + q.s[15])
    __forceinline float _mm512_sumall_ps(__m512 r)
    {
        __m256 h = _mm512_extractf32x8_ps(r, 1);
        __m256 l = _mm512_extractf32x8_ps(r, 0);
        h = _mm256_add_ps(h, l);
        return _mm256_sumall_ps(h);
    }
#define mm_sumall_ps _mm512_sumall_ps
#define mm_fmadd_ps _mm512_fmadd_ps
#endif

#if (SIMD_ARM_INSTR_SET >= SIMD_ARM8_64_NEON_VERSION)
#define mm_align_size 4
#define simd_registers 32
#elif (SIMD_ARM_INSTR_SET >= SIMD_ARM7_NEON_VERSION)
#define mm_align_size 4
#define simd_registers 16
#endif

#ifndef mm_align_size
#define mm_align_size 1
#endif // !mm_align_size

    //************************types convertors**************************//

    // convert float32 to half
    EXPORT_EXCALIBUR_PRIMITIVES unsigned short float32_to_float16(float value);

    // convert half to float32
    EXPORT_EXCALIBUR_PRIMITIVES float float16_to_float32(unsigned short value);

    // convert float to brain half
    EXPORT_EXCALIBUR_PRIMITIVES unsigned short float32_to_bfloat16(float value);

    // convert brain half to float
    EXPORT_EXCALIBUR_PRIMITIVES float bfloat16_to_float32(unsigned short value);

    // convert float32 to int8
    EXPORT_EXCALIBUR_PRIMITIVES signed char float32_to_int8(float value);

    // convert float32 to int8 with SIMD
#if __ARM_NEON
    EXPORT_EXCALIBUR_PRIMITIVES int8x8_t float32_to_int8(float32x4_t _vlow, float32x4_t _vhigh);
#endif

    EXPORT_EXCALIBUR_PRIMITIVES void int8_to_float(const signed char *int8_data, const float *scales, float *floats, int num, int group);

#if (SIMD_X86_INSTR_SET >= SIMD_X86_SSE_VERSION)
    // convert float32 to float16 with SIMD
    EXPORT_EXCALIBUR_PRIMITIVES void float2half(const float *floats, unsigned short *halfs, int length);

    // convert float16 to float32 with SIMD
    EXPORT_EXCALIBUR_PRIMITIVES void half2float(const unsigned short *halfs, float *floats, int length);
#endif

    EXPORT_EXCALIBUR_PRIMITIVES float mul_add_3x3_native(const float *r0, const float *r1, const float *r2, const float *k0, const float *k1, const float *k2, float bias);

#if (SIMD_X86_INSTR_SET >= SIMD_X86_SSE_VERSION) && (SIMD_X86_INSTR_SET <= SIMD_X86_AVX2_VERSION)
    EXPORT_EXCALIBUR_PRIMITIVES float mul_add_3x3_simd(__m128 r0_data, __m128 r1_data, __m128 r2_data, __m128 k0_data, __m128 k1_data, __m128 k2_data, float bias);
#endif
}
#endif // !_SIMD_TYPES_HPP_
