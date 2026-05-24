#pragma once
#include <cstddef>
#include <cmath>

// “ри метода интерпол€ции
enum InterpolationMethod {
    NEAREST_NEIGHBOR,
    BILINEAR,
    BICUBIC
};

// ѕоворот изображени€ на угол angleDeg (в градусах)
// in  Ч входное изображение
// out Ч выходное изображение (должно быть выделено заранее, размер outW*outH)
void rotateImage(const unsigned char* in, size_t inW, size_t inH,
    unsigned char* out, size_t outW, size_t outH,
    double angleDeg, InterpolationMethod method);

// ¬ычислить размер выходного изображени€ после поворота
void getRotatedSize(size_t inW, size_t inH, double angleDeg,
    size_t& outW, size_t& outH);