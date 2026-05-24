#pragma once
#include <cstddef>
#include <vector>

// –ангова€ фильтраци€ (rank Ч пор€дковый номер от 0 до size*size-1)
// rank=0 Ч минимум, rank=size*size/2 Ч медиана, rank=size*size-1 Ч максимум
void rankFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH, int rank);

// ћедианный фильтр
void medianFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH);

// ‘ильтр усечЄнного среднего (отбрасывает trimCount наименьших и наибольших)
void trimmedMeanFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH,
    int trimCount);

// ”средн€ющий фильтр (дл€ сравнени€ PSNR)
void meanFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);

// PSNR
double computePSNR(const unsigned char* orig, const unsigned char* filtered,
    size_t total);

// ƒобавление гауссова шума
void addGaussianNoise(const unsigned char* in, unsigned char* out,
    size_t total, double sigma);

// ƒобавление импульсного шума
void addImpulseNoise(const unsigned char* in, unsigned char* out,
    size_t total, double density);

// ћорфологические операции (structuring element Ч квадрат size x size)
void morphErode(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);

void morphDilate(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);

void morphOpen(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);

void morphClose(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);