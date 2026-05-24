#pragma once
#include <cstddef>
#include <vector>

// ƒвумерна€ свЄртка дл€ произвольного €дра
// kernel Ч €дро размером kW x kH (плоский массив)
void convolve2d(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH,
    const double* kernel, int kW, int kH);

// ‘Ќ„ Ч усредн€ющий фильтр (box filter) размером size x size
void lowPassFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);

// ”средн€ющий фильтр с порогом
void thresholdAvgFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size, double threshold);

// ƒобавление импульсного шума (salt & pepper)
void addImpulseNoise(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double density);

// ‘¬„ Ч Ћапласиан
void laplacianFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH);

// ‘¬„ Ч LoG (Ћапласиан √ауссиана)
void logFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double sigma);

// ƒетектирование границ через нулевой переход LoG
void zeroCrossing(const unsigned char* log_img, unsigned char* out,
    size_t imgW, size_t imgH);

// ‘ильтр повышени€ резкости (unsharp masking)
void sharpenFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double amount);