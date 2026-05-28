#pragma once
#include <cstddef>
#include <vector>

// �������� ���������� (rank � ���������� ����� �� 0 �� size*size-1)
// rank=0 � �������, rank=size*size/2 � �������, rank=size*size-1 � ��������
void rankFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH, int rank);

// Медианный фильтр — сортировкой (оптимален для малых апертур, ≤5×5)
void medianFilterSort(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH);

// Медианный фильтр — алгоритм Хуанга: скользящая гистограмма
// O(256) на пиксель (не зависит от размера апертуры), оптимален для больших
void medianFilterHuang(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH);

// Медианный фильтр — автоматически выбирает реализацию:
//   ≤5×5: сортировка,  >5×5: алгоритм Хуанга
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