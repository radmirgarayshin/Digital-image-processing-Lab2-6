#include "lab4.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

static double clamp(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ================================================================
//  Двумерная свёртка
// ================================================================

void convolve2d(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH,
    const double* kernel, int kW, int kH)
{
    int kHalfW = kW / 2;
    int kHalfH = kH / 2;

    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            double sum = 0.0;
            for (int ky = 0; ky < kH; ++ky) {
                for (int kx = 0; kx < kW; ++kx) {
                    int ix = x + kx - kHalfW;
                    int iy = y + ky - kHalfH;
                    ix = (int)clamp(ix, 0, imgW - 1);
                    iy = (int)clamp(iy, 0, imgH - 1);
                    sum += in[iy * imgW + ix] * kernel[ky * kW + kx];
                }
            }
            out[y * imgW + x] = (unsigned char)clamp(sum, 0, 255);
        }
    }
}

// ================================================================
//  ФНЧ — усредняющий фильтр
// ================================================================

void lowPassFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    int n = size * size;
    std::vector<double> kernel(n, 1.0 / n);
    convolve2d(in, out, imgW, imgH, kernel.data(), size, size);
}

// ================================================================
//  Усредняющий фильтр с порогом
// ================================================================

void thresholdAvgFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size, double threshold)
{
    int half = size / 2;
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            double sum = 0.0;
            int count = 0;
            int center = in[y * imgW + x];
            for (int ky = -half; ky <= half; ++ky) {
                for (int kx = -half; kx <= half; ++kx) {
                    int ix = (int)clamp(x + kx, 0, imgW - 1);
                    int iy = (int)clamp(y + ky, 0, imgH - 1);
                    int val = in[iy * imgW + ix];
                    if (fabs(val - center) <= threshold) {
                        sum += val;
                        ++count;
                    }
                }
            }
            out[y * imgW + x] = (unsigned char)(sum / count);
        }
    }
}

// ================================================================
//  Импульсный шум (salt & pepper)
// ================================================================

void addImpulseNoise(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double density)
{
    srand((unsigned)time(0));
    size_t total = imgW * imgH;
    for (size_t i = 0; i < total; ++i) out[i] = in[i];
    for (size_t i = 0; i < total; ++i) {
        double r = (double)rand() / RAND_MAX;
        if (r < density / 2)       out[i] = 0;
        else if (r < density)      out[i] = 255;
    }
}

// ================================================================
//  Лапласиан
// ================================================================

void laplacianFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH)
{
    double kernel[] = {
         0, -1,  0,
        -1,  4, -1,
         0, -1,  0
    };
    // Сохраняем знаковый результат со сдвигом 128
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            double sum = 0.0;
            int kH = 3, kW = 3;
            for (int ky = 0; ky < kH; ++ky) {
                for (int kx = 0; kx < kW; ++kx) {
                    int ix = (int)clamp(x + kx - 1, 0, imgW - 1);
                    int iy = (int)clamp(y + ky - 1, 0, imgH - 1);
                    sum += in[iy * imgW + ix] * kernel[ky * kW + kx];
                }
            }
            out[y * imgW + x] = (unsigned char)clamp(sum + 128, 0, 255);
        }
    }
}

// ================================================================
//  LoG (Лапласиан Гауссиана)
// ================================================================

void logFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double sigma)
{
    int size = (int)(6 * sigma) | 1; // нечётный размер
    if (size < 3) size = 3;
    int half = size / 2;

    std::vector<double> kernel(size * size);
    double sigma2 = sigma * sigma;
    double sum = 0.0;

    for (int y = -half; y <= half; ++y) {
        for (int x = -half; x <= half; ++x) {
            double r2 = x * x + y * y;
            double v = (r2 - 2 * sigma2) / (sigma2 * sigma2)
                * exp(-r2 / (2 * sigma2));
            kernel[(y + half) * size + (x + half)] = v;
            sum += v;
        }
    }
    // Нормировка
    for (auto& k : kernel) k -= sum / (size * size);

    // Применяем со сдвигом 128
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            double s = 0.0;
            for (int ky = 0; ky < size; ++ky) {
                for (int kx = 0; kx < size; ++kx) {
                    int ix = (int)clamp(x + kx - half, 0, imgW - 1);
                    int iy = (int)clamp(y + ky - half, 0, imgH - 1);
                    s += in[iy * imgW + ix] * kernel[ky * size + kx];
                }
            }
            out[y * imgW + x] = (unsigned char)clamp(s + 128, 0, 255);
        }
    }
}

// ================================================================
//  Детектирование границ через нулевой переход
// ================================================================

void zeroCrossing(const unsigned char* log_img, unsigned char* out,
    size_t imgW, size_t imgH)
{
    for (size_t i = 0; i < imgW * imgH; ++i) out[i] = 0;

    for (int y = 1; y < (int)imgH - 1; ++y) {
        for (int x = 1; x < (int)imgW - 1; ++x) {
            int c = (int)log_img[y * imgW + x] - 128;
            // Проверяем соседей по горизонтали и вертикали
            int r = (int)log_img[y * imgW + x + 1] - 128;
            int d = (int)log_img[(y + 1) * imgW + x] - 128;
            if ((c > 0 && r < 0) || (c < 0 && r > 0) ||
                (c > 0 && d < 0) || (c < 0 && d > 0))
                out[y * imgW + x] = 255;
        }
    }
}

// ================================================================
//  Повышение резкости (unsharp masking)
// ================================================================

void sharpenFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double amount)
{
    // Ядро: исходное + amount * лапласиан
    double kernel[] = {
             0,      -amount,       0,
        -amount, 1 + 4 * amount, -amount,
             0,      -amount,       0
    };
    convolve2d(in, out, imgW, imgH, kernel, 3, 3);
}