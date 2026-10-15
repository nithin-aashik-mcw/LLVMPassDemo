// AVX2/FMA kernels, build with -march=native
#pragma once

#include <cstddef>
#include <immintrin.h>

// avx2 has 16 ymm registers, avx-512 has 32
#ifdef __AVX512VL__
constexpr size_t VEC_REGS = 32;
#else
constexpr size_t VEC_REGS = 16;
#endif
constexpr size_t VEC = 8;          // floats per register
constexpr size_t LINE = 16;        // floats per cache line
constexpr size_t MIN_SUMS = 4 * 2; // fma latency 4 x 2 units

struct Shape {
    size_t mr, nr;
};

// biggest block of C that fits in registers (sums + B row + A value + 1 spare)
constexpr Shape pickShape() {
    Shape best = {1, LINE};
    double bestRatio = 0;
    for (size_t nr = LINE; nr <= 4 * LINE; nr += LINE) {
        size_t v = nr / VEC;
        if (VEC_REGS < v + 2)
            break;
        size_t mr = (VEC_REGS - 2 - v) / v;
        size_t sums = mr * v;
        if (mr == 0 || sums < MIN_SUMS)
            continue;
        double ratio = double(sums) / double(mr + v); // fma's per load
        if (ratio > bestRatio) {
            bestRatio = ratio;
            best = {mr, nr};
        }
    }
    return best;
}

constexpr size_t MR = pickShape().mr; // 6 on AVX2
constexpr size_t NR = pickShape().nr; // 16 on AVX2
constexpr size_t NV = NR / VEC;       // registers per row

// one MR x NR block of C, sums stay in registers
static inline void microKernel(float **a, float **b, float **c, size_t i, size_t j, size_t kk,
                               size_t kEnd) {
    __m256 sum[MR][NV];
    for (size_t r = 0; r < MR; r++)
        for (size_t v = 0; v < NV; v++)
            sum[r][v] = _mm256_loadu_ps(c[i + r] + j + v * VEC);

    for (size_t k = kk; k < kEnd; k++) {
        __m256 bv[NV];
        for (size_t v = 0; v < NV; v++)
            bv[v] = _mm256_loadu_ps(b[k] + j + v * VEC);
        for (size_t r = 0; r < MR; r++) {
            __m256 av = _mm256_broadcast_ss(&a[i + r][k]);
            for (size_t v = 0; v < NV; v++)
                sum[r][v] = _mm256_fmadd_ps(av, bv[v], sum[r][v]);
        }
    }

    for (size_t r = 0; r < MR; r++)
        for (size_t v = 0; v < NV; v++)
            _mm256_storeu_ps(c[i + r] + j + v * VEC, sum[r][v]);
}

// leftover edges
static inline void edgeKernel(float **a, float **b, float **c, size_t i, size_t iEnd, size_t j,
                              size_t jEnd, size_t kk, size_t kEnd) {
    for (size_t r = i; r < iEnd; r++) {
        float *cr = c[r];
        for (size_t k = kk; k < kEnd; k++) {
            const float *bk = b[k];
            float aik = a[r][k];
            for (size_t q = j; q < jEnd; q++)
                cr[q] += aik * bk[q];
        }
    }
}
