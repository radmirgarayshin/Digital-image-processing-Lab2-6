#include "lab5.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// ================================================================
//  Ранговая фильтрация
// ================================================================

void rankFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH, int rank)
{
    int halfW = apertureW / 2;
    int halfH = apertureH / 2;
    std::vector<unsigned char> window(apertureW * apertureH);

    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            int idx = 0;
            for (int ky = -halfH; ky <= halfH; ++ky)
                for (int kx = -halfW; kx <= halfW; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    window[idx++] = in[iy * imgW + ix];
                }
            std::sort(window.begin(), window.end());
            out[y * imgW + x] = window[rank];
        }
    }
}

void medianFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH)
{
    rankFilter(in, out, imgW, imgH, apertureW, apertureH,
        (apertureW * apertureH) / 2);
}

// ================================================================
//  Фильтр усечённого среднего
// ================================================================

void trimmedMeanFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH,
    int trimCount)
{
    int halfW = apertureW / 2;
    int halfH = apertureH / 2;
    int total = apertureW * apertureH;
    std::vector<unsigned char> window(total);

    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            int idx = 0;
            for (int ky = -halfH; ky <= halfH; ++ky)
                for (int kx = -halfW; kx <= halfW; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    window[idx++] = in[iy * imgW + ix];
                }
            std::sort(window.begin(), window.end());
            double sum = 0;
            int count = 0;
            for (int i = trimCount; i < total - trimCount; ++i) {
                sum += window[i];
                ++count;
            }
            out[y * imgW + x] = (unsigned char)(count > 0 ? sum / count : 0);
        }
    }
}

// ================================================================
//  Усредняющий фильтр
// ================================================================

void meanFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    int half = size / 2;
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            double sum = 0;
            int count = 0;
            for (int ky = -half; ky <= half; ++ky)
                for (int kx = -half; kx <= half; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    sum += in[iy * imgW + ix];
                    ++count;
                }
            out[y * imgW + x] = (unsigned char)(sum / count);
        }
    }
}

// ================================================================
//  PSNR
// ================================================================

double computePSNR(const unsigned char* orig, const unsigned char* filtered,
    size_t total)
{
    double mse = 0;
    for (size_t i = 0; i < total; ++i) {
        double d = (double)orig[i] - (double)filtered[i];
        mse += d * d;
    }
    mse /= total;
    if (mse < 1e-10) return 999.0;
    return 10.0 * log10(255.0 * 255.0 / mse);
}

// ================================================================
//  Шум
// ================================================================

void addGaussianNoise(const unsigned char* in, unsigned char* out,
    size_t total, double sigma)
{
    srand(42);
    for (size_t i = 0; i < total; ++i) {
        double u1 = (double)(rand() + 1) / (RAND_MAX + 1.0);
        double u2 = (double)rand() / RAND_MAX;
        double noise = sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159 * u2) * sigma;
        int val = (int)(in[i] + noise);
        out[i] = (unsigned char)(val < 0 ? 0 : val > 255 ? 255 : val);
    }
}

void addImpulseNoise(const unsigned char* in, unsigned char* out,
    size_t total, double density)
{
    srand(42);
    for (size_t i = 0; i < total; ++i) out[i] = in[i];
    for (size_t i = 0; i < total; ++i) {
        double r = (double)rand() / RAND_MAX;
        if (r < density / 2)  out[i] = 0;
        else if (r < density) out[i] = 255;
    }
}

// ================================================================
//  Морфологические операции
// ================================================================

void morphErode(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    int half = size / 2;
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            unsigned char minVal = 255;
            for (int ky = -half; ky <= half; ++ky)
                for (int kx = -half; kx <= half; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    minVal = std::min(minVal, in[iy * imgW + ix]);
                }
            out[y * imgW + x] = minVal;
        }
    }
}

void morphDilate(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    int half = size / 2;
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            unsigned char maxVal = 0;
            for (int ky = -half; ky <= half; ++ky)
                for (int kx = -half; kx <= half; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    maxVal = std::max(maxVal, in[iy * imgW + ix]);
                }
            out[y * imgW + x] = maxVal;
        }
    }
}

void morphOpen(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    std::vector<unsigned char> tmp(imgW * imgH);
    morphErode(in, tmp.data(), imgW, imgH, size);
    morphDilate(tmp.data(), out, imgW, imgH, size);
}

void morphClose(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    std::vector<unsigned char> tmp(imgW * imgH);
    morphDilate(in, tmp.data(), imgW, imgH, size);
    morphErode(tmp.data(), out, imgW, imgH, size);
}