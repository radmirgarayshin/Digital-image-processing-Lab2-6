#pragma once
#include <cstdint>
#include <vector>
#include <string>

// Структура для хранения загруженного TIFF-изображения
struct TiffImage {
    int width;                    // ширина в пикселях
    int height;                   // высота в пикселях
    int bpp;                      // бит на пиксель
    std::vector<uint8_t> pixels;  // пиксели, построчно (grayscale)
    bool        valid;            // true если загрузка успешна
    std::string error;            // описание ошибки
};

// Загрузить несжатый полутоновый TIFF
// Поддерживается: Intel (II) и Motorola (MM), несколько strips
TiffImage loadTiff(const char* filename);
