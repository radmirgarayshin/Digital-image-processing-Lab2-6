#pragma once
#include <vector>
#include <cstdint>

struct RegionInfo {
    int label;
    int area;                   // M00 (площадь)
    double cx, cy;              // центр масс (M10/M00, M01/M00)
    double mu20, mu02, mu11;    // центральные моменты 2-го порядка (нормированные на площадь)

    // circEig = lam_min / lam_max — отношение собственных значений матрицы моментов.
    // Проблема: для любой центрально-симметричной фигуры (квадрат, ромб) тоже равно 1.
    double circEig;

    // circMom = A / (2*pi*(mu20+mu02)) — признак округлости через моменты.
    // Круг: 1.0, квадрат: ~0.955, прямоугольник/эллипс 3:1: ~0.57-0.60.
    // В отличие от circEig, равен 1 только для круга.
    double circMom;

    // circIso = 4*pi*S/P² — изопериметрический коэффициент (для справки)
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

// Подсчёт округлых областей по момент-признаку C_m = A/(2*pi*(mu20+mu02))
// momMin: нижний порог (рекомендуется 0.90 — круги: ~1.0, квадраты: ~0.955, эллипсы/прямоуг.: <0.65)
int countCircularRegions(const std::vector<RegionInfo>& regions,
    int minArea, double momMin, double momMax);

// Сохранить все размеченные области (разные серые уровни)
void saveLabeledImage(const int* labels, int width, int height,
    int numLabels, const char* filename);

// Сохранить только округлые области (остальное — чёрное)
void saveCircularOnly(const int* labels, const std::vector<RegionInfo>& regions,
    int width, int height,
    int minArea, double circMin, double circMax, const char* filename);

// Генерация тестового изображения с кругами, квадратами и ромбами
void generateTestImage(uint8_t* img, int width, int height);
