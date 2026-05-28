#include "lab1.h"
#include "PngProc.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>

void computeHistogram(const unsigned char* img, size_t width, size_t height,
    int hist[256])
{
    for (int i = 0; i < 256; ++i) hist[i] = 0;
    size_t total = width * height;
    for (size_t i = 0; i < total; ++i)
        ++hist[img[i]];
}

ImageStats computeStats(const int hist[256], size_t totalPixels)
{
    ImageStats s = {};

    double mean = 0.0;
    for (int i = 0; i < 256; ++i)
        mean += (double)i * hist[i];
    mean /= totalPixels;
    s.mean = mean;

    double var = 0.0, skew = 0.0, kurt = 0.0;
    for (int i = 0; i < 256; ++i) {
        double d = i - mean;
        double p = (double)hist[i] / totalPixels;
        var += p * d * d;
        skew += p * d * d * d;
        kurt += p * d * d * d * d;
    }
    s.variance = var;
    s.stddev = sqrt(var);
    s.skewness = (var > 1e-12) ? skew / (var * s.stddev) : 0.0;
    s.kurtosis = (var > 1e-12) ? kurt / (var * var) - 3.0 : 0.0;

    double entropy = 0.0, energy = 0.0;
    for (int i = 0; i < 256; ++i) {
        if (hist[i] == 0) continue;
        double p = (double)hist[i] / totalPixels;
        entropy -= p * log2(p);
        energy += p * p;
    }
    s.entropy = entropy;
    s.energy = energy;

    long long cumul = 0;
    bool q1set = false, q2set = false, q3set = false;
    for (int i = 0; i < 256; ++i) {
        cumul += hist[i];
        if (!q1set && cumul >= (long long)(totalPixels * 0.25)) { s.q1 = i; q1set = true; }
        if (!q2set && cumul >= (long long)(totalPixels * 0.50)) { s.q2 = i; q2set = true; }
        if (!q3set && cumul >= (long long)(totalPixels * 0.75)) { s.q3 = i; q3set = true; }
    }

    return s;
}

void computeGLCM(const unsigned char* img, size_t width, size_t height,
    int dr, int dc, double glcm[256][256])
{
    for (int i = 0; i < 256; ++i)
        for (int j = 0; j < 256; ++j)
            glcm[i][j] = 0.0;

    long long count = 0;
    for (int y = 0; y < (int)height; ++y) {
        int ny = y + dr;
        if (ny < 0 || ny >= (int)height) continue;
        for (int x = 0; x < (int)width; ++x) {
            int nx = x + dc;
            if (nx < 0 || nx >= (int)width) continue;
            int a = img[y * width + x];
            int b = img[ny * width + nx];
            glcm[a][b] += 1.0;
            glcm[b][a] += 1.0;
            count += 2;
        }
    }
    if (count > 0)
        for (int i = 0; i < 256; ++i)
            for (int j = 0; j < 256; ++j)
                glcm[i][j] /= count;
}

double computeGLCMEnergy(const double glcm[256][256])
{
    double energy = 0.0;
    for (int i = 0; i < 256; ++i)
        for (int j = 0; j < 256; ++j)
            energy += glcm[i][j] * glcm[i][j];
    return energy;
}

static double randGauss()
{
    double u1, u2;
    do { u1 = (double)rand() / RAND_MAX; } while (u1 < 1e-10);
    u2 = (double)rand() / RAND_MAX;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159265358979 * u2);
}

void addGaussianNoise(const unsigned char* in, unsigned char* out,
    size_t width, size_t height, double variance)
{
    srand((unsigned)time(0));
    double sigma = sqrt(variance);
    size_t total = width * height;
    for (size_t i = 0; i < total; ++i) {
        double val = in[i] + sigma * randGauss();
        if (val < 0.0)   val = 0.0;
        if (val > 255.0) val = 255.0;
        out[i] = (unsigned char)(val + 0.5);
    }
}

double computePSNR(const unsigned char* orig, const unsigned char* noisy,
    size_t width, size_t height)
{
    double mse = 0.0;
    size_t total = width * height;
    for (size_t i = 0; i < total; ++i) {
        double d = (double)orig[i] - (double)noisy[i];
        mse += d * d;
    }
    mse /= total;
    if (mse < 1e-10) return 999.0;
    return 10.0 * log10(255.0 * 255.0 / mse);
}

// ================================================================
//  Сохранение GLCM как PNG 256x256 с логарифмической нормализацией
//
//  Пиксель (i,j) тем ярче, чем чаще встречается пара яркостей (i,j)
//  в исходном изображении. Для наглядности применяется log(1 + p).
//
//  Интерпретация:
//   - Фото: узкая полоса вдоль диагонали (соседние пиксели близки)
//   - Текстура (шахматка): два ярких пятна симметрично вне диагонали
//     (пары 0-255 и 255-0 доминируют)
//   - Горизонтальный градиент: диагональная полоса со смещением
//   - Однотонное: единственная точка строго на диагонали
// ================================================================

void saveGLCM(const double glcm[256][256], const char* filename)
{
    // Логарифмическая нормализация для лучшей видимости малых значений
    std::vector<double> logV(256 * 256);
    double maxLog = 0.0;
    for (int i = 0; i < 256; ++i)
        for (int j = 0; j < 256; ++j) {
            logV[i * 256 + j] = log(1.0 + glcm[i][j]);
            if (logV[i * 256 + j] > maxLog)
                maxLog = logV[i * 256 + j];
        }

    std::vector<unsigned char> out(256 * 256);
    for (int k = 0; k < 256 * 256; ++k) {
        double v = (maxLog > 1e-15) ? logV[k] / maxLog * 255.0 : 0.0;
        out[k] = (unsigned char)(v + 0.5);
    }

    NPngProc::writePngFile(filename, out.data(), 256, 256, 8);
}