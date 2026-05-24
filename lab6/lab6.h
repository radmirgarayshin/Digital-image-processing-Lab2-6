#pragma once
#include <vector>
#include <cstdint>

struct RegionInfo {
    int label;
    int area;             // M00 — площадь
    double cx, cy;        // центр масс (M10/M00, M01/M00)
    double mu20, mu02, mu11; // центральные моменты 2-го порядка
    double circularity;   // 0..1, 1 = идеальный круг
};

// Алгоритм Оцу — возвращает оптимальный порог
int otsuThreshold(const uint8_t* img, int width, int height);

// Бинаризация: >= threshold -> 255, иначе 0
void binarize(const uint8_t* img, uint8_t* out, int width, int height, int threshold);

// Разметка 4-связных областей, возвращает число областей
int labelConnectedComponents(const uint8_t* binary, int* labels, int width, int height);

// Вычисление геометрических моментов для каждой области
std::vector<RegionInfo> computeRegionProperties(
    const int* labels, int width, int height, int numLabels);

// Подсчёт областей с area > minArea и circularity >= circThreshold
int countCircularRegions(const std::vector<RegionInfo>& regions,
    int minArea, double circThreshold);

// Сохранить карту меток как grayscale PNG
void saveLabeledImage(const int* labels, int width, int height,
    int numLabels, const char* filename);