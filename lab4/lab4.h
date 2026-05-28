#pragma once
#include <cstddef>
#include <vector>

// ��������� ������ ��� ������������� ����
// kernel � ���� �������� kW x kH (������� ������)
void convolve2d(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH,
    const double* kernel, int kW, int kH);

// ��� � ����������� ������ (box filter) �������� size x size
void lowPassFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size);

// ����������� ������ � �������
void thresholdAvgFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size, double threshold);

// ���������� ����������� ���� (salt & pepper)
void addImpulseNoise(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double density);

// ФВЧ Лапласиан — конфигурируемый размер ядра (size x size, нечётное)
// Ядро: все элементы = -1, центр = size*size - 1 (сумма = 0)
// Результат смещён на +128 для отображения
void laplacianFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size = 3);

// ФВЧ LoG (Лапласиан Гаусса)
void logFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double sigma);

// Детектирование границ через нулевой переход LoG
void zeroCrossing(const unsigned char* log_img, unsigned char* out,
    size_t imgW, size_t imgH);

// Повышение резкости (unsharp masking) — конфигурируемый размер ядра
// Ядро: все элементы = -amount, центр = 1 + (size*size - 1)*amount
void sharpenFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double amount, int size = 3);