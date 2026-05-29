#pragma once
#include <cstddef>
#include <vector>

// �������� ���������� (rank � ���������� ����� �� 0 �� size*size-1)
// rank=0 � �������, rank=size*size/2 � �������, rank=size*size-1 � ��������
void rankFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH, int rank);

// Медианный фильтр — сортировкой через nth_element (O(n) выбор)
void medianFilterSort(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH);

// Быстрый медианный фильтр 3×3 — сеть сравнений (19 операций, opt_med9)
void medianFilter3x3Fast(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH);

// Медианный фильтр — алгоритм Хуанга: скользящая гистограмма
// O(kH + 256) на пиксель (не растёт с площадью апертуры), оптимален для больших
void medianFilterHuang(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH);

// Медианный фильтр — автоматически выбирает реализацию:
//   3×3: сеть сравнений; 5×5-7×7: nth_element; 9×9+: алгоритм Хуанга
void medianFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH);

// ������ ���������� �������� (����������� trimCount ���������� � ����������)
void trimmedMeanFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH,
    int trimCount);

// ����������� ������ (��� ��������� PSNR)
void meanFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);

// PSNR
double computePSNR(const unsigned char* orig, const unsigned char* filtered,
    size_t total);

// ���������� �������� ����
void addGaussianNoise(const unsigned char* in, unsigned char* out,
    size_t total, double sigma);

// ���������� ����������� ����
void addImpulseNoise(const unsigned char* in, unsigned char* out,
    size_t total, double density);

// ��������������� �������� (structuring element � ������� size x size)
void morphErode(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);

void morphDilate(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);

void morphOpen(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);

void morphClose(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);