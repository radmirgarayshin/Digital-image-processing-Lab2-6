#pragma once
#include <cstddef>

void computeHistogram(const unsigned char* img, size_t width, size_t height,
    int hist[256]);

struct ImageStats {
    double mean;
    double variance;
    double stddev;
    double entropy;
    double energy;
    double skewness;
    double kurtosis;
    double q1, q2, q3;
};

ImageStats computeStats(const int hist[256], size_t totalPixels);

void computeGLCM(const unsigned char* img, size_t width, size_t height,
    int dr, int dc,
    double glcm[256][256]);

double computeGLCMEnergy(const double glcm[256][256]);

// Сохранить GLCM как PNG 256x256 (лог-нормализация для наглядности)
// Яркий пиксель (i,j) = часто встречается пара яркостей (i,j) в изображении
void saveGLCM(const double glcm[256][256], const char* filename);

void addGaussianNoise(const unsigned char* in, unsigned char* out,
    size_t width, size_t height, double variance);

double computePSNR(const unsigned char* orig, const unsigned char* noisy,
    size_t width, size_t height);
