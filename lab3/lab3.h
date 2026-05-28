#pragma once
#include <cstddef>
#include <cmath>

// ��� ������ ������������
enum InterpolationMethod {
    NEAREST_NEIGHBOR,
    BILINEAR,
    BICUBIC
};

// ������� ����������� �� ���� angleDeg (� ��������)
// in  � ������� �����������
// out � �������� ����������� (������ ���� �������� �������, ������ outW*outH)
void rotateImage(const unsigned char* in, size_t inW, size_t inH,
    unsigned char* out, size_t outW, size_t outH,
    double angleDeg, InterpolationMethod method);

// Вычисление размеров повёрнутого изображения
void getRotatedSize(size_t inW, size_t inH, double angleDeg,
    size_t& outW, size_t& outH);

// Старая версия (switch внутри цикла) — только для бенчмарка
void rotateImageOld(const unsigned char* in, size_t inW, size_t inH,
    unsigned char* out, size_t outW, size_t outH,
    double angleDeg, InterpolationMethod method);