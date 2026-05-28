#pragma once
#include <vector>
#include <cstdint>

struct RegionInfo {
    int label;
    int area;                   // M00 (площадь)
    double cx, cy;              // центр масс (M10/M00, M01/M00)
    double mu20, mu02, mu11;    // центральные моменты 2-го порядка

    // Eigenvalue circularity: lam_min / lam_max
    // Недостаток: для квадрата и ромба тоже = 1 (симметричные формы)
    double circEig;

    // Isoperimetric circularity: 4*pi*Area / Perimeter^2
    // Круг: ~1.0   Квадрат: ~pi/4~0.785   Ромб: ~pi/4~0.785
    int    perimeter;
    double circIso;
};

// Порог Оцу
int otsuThreshold(const uint8_t* img, int width, int height);

// Бинаризация: >= threshold -> 255, иначе 0
void binarize(const uint8_t* img, uint8_t* out, int width, int height, int threshold);

// Разметка 4-связных компонент; возвращает число меток
int labelConnectedComponents(const uint8_t* binary, int* labels, int width, int height);

// Вычисление геометрических признаков для каждой области
std::vector<RegionInfo> computeRegionProperties(
    const int* labels, int width, int height, int numLabels);

// Подсчёт округлых областей (по isoperimetric circularity)
// Используется диапазон [circMin, circMax]: круги ~1.2-1.3, ромбы ~1.6 (выше), квадраты ~0.8 (ниже)
int countCircularRegions(const std::vector<RegionInfo>& regions,
    int minArea, double circMin, double circMax);

// Сохранить все размеченные области (разные серые уровни)
void saveLabeledImage(const int* labels, int width, int height,
    int numLabels, const char* filename);

// Сохранить только округлые области (остальное — чёрное)
void saveCircularOnly(const int* labels, const std::vector<RegionInfo>& regions,
    int width, int height,
    int minArea, double circMin, double circMax, const char* filename);

// Генерация тестового изображения с кругами, квадратами и ромбами
void generateTestImage(uint8_t* img, int width, int height);
