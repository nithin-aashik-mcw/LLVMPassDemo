// g++ mat-mul.cpp -O3 -march=native -fopenmp -o mat-mul
#include <unistd.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

#include "kernel.h"

float **matrix_a;
float **matrix_b;
float **matrix_c;

void prepareMatrix(float **&matrix, const size_t &N, const bool &rand = true,
                   const float &val = 0.0f) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // one aligned block, rows padded to an odd number of cache lines
    size_t lines = (N + 15) / 16;
    if (lines % 2 == 0)
        lines++;
    size_t stride = lines * 16;

    void *data;
    if (posix_memalign(&data, 64, N * stride * sizeof(float) + 64) != 0) {
        std::cerr << "out of memory" << std::endl;
        exit(1);
    }
    matrix = new float *[N];
    for (size_t i = 0; i < N; i++) {
        matrix[i] = (float *)data + i * stride;
        for (size_t j = 0; j < N; j++) {
            matrix[i][j] = rand ? dist(gen) : val;
        }
    }
}

constexpr size_t ROW_BLOCK = 10 * MR; // rows of C per omp task

// leave 2 cpus for windows, starting all 14 is slow in WSL
int getThreadCount() {
    int threads = omp_get_max_threads();
    if (threads > 8)
        threads -= 2;
    return threads;
}

// a tile of A + a tile of B in half the L2 (hyperthreads share it)
size_t getTileSize() {
    long l2 = sysconf(_SC_LEVEL2_CACHE_SIZE);
    if (l2 <= 0)
        l2 = 256 * 1024;

    size_t tile = (size_t)std::sqrt(l2 / 2.0 / (2 * sizeof(float)));
    tile = tile / NR * NR;
    return std::max(tile, NR);
}

// C += A*B, C must start at zero.
// each task is ROW_BLOCK rows x T columns of C, tiles are walked in snake order
void multiplyMatrix(float **a, float **b, float **c, const size_t &N, const size_t &T) {
    size_t nTiles = (N + T - 1) / T;
    size_t nBlocks = (N + ROW_BLOCK - 1) / ROW_BLOCK;

#pragma omp parallel for collapse(2) schedule(dynamic) num_threads(getThreadCount())
    for (size_t tj = 0; tj < nTiles; tj++) {
        for (size_t r = 0; r < nBlocks; r++) {
            size_t rb = (tj % 2) ? nBlocks - 1 - r : r;
            size_t i0 = rb * ROW_BLOCK, i1 = std::min(i0 + ROW_BLOCK, N);
            size_t j0 = tj * T, j1 = std::min(j0 + T, N);
            size_t nStrips = (j1 - j0 + NR - 1) / NR;
            for (size_t s = 0; s < nTiles; s++) {
                size_t tk = (rb % 2) ? nTiles - 1 - s : s;
                size_t k0 = tk * T, k1 = std::min(k0 + T, N);
                for (size_t t = 0; t < nStrips; t++) {
                    size_t j = j0 + ((s % 2) ? nStrips - 1 - t : t) * NR;
                    for (size_t i = i0; i < i1; i += MR) {
                        if (i + MR <= i1 && j + NR <= j1)
                            microKernel(a, b, c, i, j, k0, k1);
                        else
                            edgeKernel(a, b, c, i, std::min(i + MR, i1), j, std::min(j + NR, j1),
                                       k0, k1);
                    }
                }
            }
        }
    }
}

void printUsage(const char *progName, std::ostream &out = std::cerr) {
    out << "Usage: " << progName << " <N>\n"
        << "       " << progName << " -h | --help\n"
        << "  N             size of the square matrices to multiply (N x N)\n"
        << "  -h, --help    show this help message and exit\n";
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0], std::cout);
            return 0;
        }
    }
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }
    size_t N;
    try {
        std::string arg = argv[1];
        // stoull takes "-5" and wraps it around, so reject it ourselves
        if (arg.find('-') != std::string::npos) {
            throw std::invalid_argument("negative value");
        }
        size_t pos;
        N = std::stoull(arg, &pos);
        if (pos != arg.size()) {
            throw std::invalid_argument("trailing characters");
        }
    } catch (const std::exception &) {
        std::cerr << "Error: N must be a non-negative integer, got '" << argv[1] << "'\n";
        printUsage(argv[0]);
        return 1;
    }
    std::cout << "N = " << N << std::endl;
    std::cout << "Preparing Matrix..." << std::endl;
    prepareMatrix(matrix_a, N);
    prepareMatrix(matrix_b, N);
    prepareMatrix(matrix_c, N, false);
    size_t T = getTileSize();
    std::cout << "Tile size = " << T << std::endl;
    std::cout << "Kernel = " << MR << "x" << NR << std::endl;
#ifdef _OPENMP
    std::cout << "Threads = " << getThreadCount() << std::endl;
#endif
    std::cout << "Multiplying Matrix..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    multiplyMatrix(matrix_a, matrix_b, matrix_c, N, T);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << " ms" << std::endl;
    return 0;
}
