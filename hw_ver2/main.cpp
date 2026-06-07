/*
 * Домашнее задание: выпрямление повёрнутого текста на сканированной странице.
 *
 * Алгоритм:
 *   1. Читаем TIFF-файл (собственная реализация tiff_reader).
 *   2. Вычисляем 2D FFT изображения.
 *   3. Строим логарифм амплитудного спектра (fftshift: DC в центре).
 *   4. Сегментация спектра — выделяем яркие пиксели (порог = mean + 4.5*std).
 *      Исключаем DC-зону и горизонтальную полосу (артефакты границ страницы).
 *   5. Вычисляем взвешенные центральные моменты 2-го порядка (лекция 10):
 *        mu20, mu02, mu11
 *   6. Главная ось инерции по формуле из лекции 10:
 *        phi = 0.5 * arctan(2*mu11 / (mu20 - mu02))
 *      Угол наклона текста: skew = phi - 90 градусов.
 *      Угол коррекции: correction = phi - 90 градусов.
 *   7. Поворачиваем исходное изображение на угол correction (билинейная интерполяция).
 *   8. Сохраняем результаты (PNG через PngProc).
 *
 * Выходные файлы:
 *   out_original.png   — исходное изображение (TIFF -> PNG)
 *   out_spectrum.png   — логарифм спектра с отмеченными пиками
 *   out_corrected.png  — выпрямленное изображение
 */

#define _USE_MATH_DEFINES
#include <cstdio>
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#include <cstring>

#include "tiff_reader.h"
#include "PngProc.h"

using Complex = std::complex<double>;
static const double PI = 3.14159265358979323846;

// ================================================================
//  Раздел 1. FFT (Кули-Тьюки, итеративный, бит-реверс)
// ================================================================

// Следующая степень двойки >= n
static int nextPow2(int n) {
    int p = 1;
    while (p < n) p <<= 1;
    return p;
}

// Одномерное FFT. Размер должен быть степенью 2.
// inverse=true — обратное преобразование (с делением на N)
static void fft1d(std::vector<Complex>& a, bool inverse) {
    int n = (int)a.size();

    // Бит-реверсная перестановка
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }

    // Основной цикл (бабочки)
    for (int len = 2; len <= n; len <<= 1) {
        double ang = 2.0 * PI / len * (inverse ? -1.0 : 1.0);
        Complex wlen(cos(ang), sin(ang));
        for (int i = 0; i < n; i += len) {
            Complex w(1.0, 0.0);
            for (int j = 0; j < len / 2; j++) {
                Complex u = a[i + j];
                Complex v = a[i + j + len / 2] * w;
                a[i + j]           = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    if (inverse)
        for (auto& x : a) x /= (double)n;
}

// Двумерное FFT: сначала по строкам, затем по столбцам
static void fft2d(std::vector<Complex>& data, int fftW, int fftH, bool inverse) {
    std::vector<Complex> buf;

    buf.resize(fftW);
    for (int y = 0; y < fftH; y++) {
        for (int x = 0; x < fftW; x++) buf[x] = data[(size_t)y * fftW + x];
        fft1d(buf, inverse);
        for (int x = 0; x < fftW; x++) data[(size_t)y * fftW + x] = buf[x];
    }

    buf.resize(fftH);
    for (int x = 0; x < fftW; x++) {
        for (int y = 0; y < fftH; y++) buf[y] = data[(size_t)y * fftW + x];
        fft1d(buf, inverse);
        for (int y = 0; y < fftH; y++) data[(size_t)y * fftW + x] = buf[y];
    }
}

// fftshift: нулевая частота (DC) в центр
static void fftshift(std::vector<Complex>& data, int fftW, int fftH) {
    int hy = fftH / 2, hx = fftW / 2;
    for (int y = 0; y < hy; y++) {
        for (int x = 0; x < hx; x++) {
            std::swap(data[(size_t) y      * fftW +  x     ],
                      data[(size_t)(y + hy) * fftW + (x + hx)]);
            std::swap(data[(size_t) y      * fftW + (x + hx)],
                      data[(size_t)(y + hy) * fftW +  x     ]);
        }
    }
}

// ================================================================
//  Раздел 2. Поворот изображения (билинейная интерполяция)
//  angleDeg > 0 — поворот против часовой стрелки (на экране)
// ================================================================

static std::vector<uint8_t> rotateImage(
    const std::vector<uint8_t>& src, int W, int H, double angleDeg)
{
    double rad  = angleDeg * PI / 180.0;
    double cosA = cos(rad);
    double sinA = sin(rad);
    double cx   = W / 2.0;
    double cy   = H / 2.0;

    std::vector<uint8_t> dst(W * H, 255);  // белый фон

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            // Обратный поворот: из выходного пикселя ищем исходный
            double dx = x - cx;
            double dy = y - cy;
            double sx = cosA * dx - sinA * dy + cx;
            double sy = sinA * dx + cosA * dy + cy;

            // Билинейная интерполяция
            int x0 = (int)sx, y0 = (int)sy;
            if (x0 < 0 || x0 + 1 >= W || y0 < 0 || y0 + 1 >= H) continue;

            double fx = sx - x0, fy = sy - y0;
            double v = (1 - fy) * ((1 - fx) * src[(size_t)y0 * W + x0]
                                 +      fx  * src[(size_t)y0 * W + x0 + 1])
                     +      fy  * ((1 - fx) * src[(size_t)(y0+1) * W + x0]
                                 +      fx  * src[(size_t)(y0+1) * W + x0 + 1]);
            dst[(size_t)y * W + x] = (uint8_t)(v + 0.5);
        }
    }
    return dst;
}

// ================================================================
//  Раздел 3. Сохранение PNG через PngProc
// ================================================================

static void savePng(const char* fname, const std::vector<uint8_t>& buf, int w, int h) {
    NPngProc::writePngFile(fname,
        const_cast<uint8_t*>(buf.data()),
        (size_t)w, (size_t)h, 8);
}

// Сохранение спектра: масштаб 50%, яркие пиксели отмечены белым
static void saveSpectrum(const char* fname,
                         const std::vector<double>& logMag,
                         int fftW, int fftH,
                         const std::vector<std::pair<int,int>>& markedPixels)
{
    double minV = *std::min_element(logMag.begin(), logMag.end());
    double maxV = *std::max_element(logMag.begin(), logMag.end());
    double rng  = (maxV > minV) ? (maxV - minV) : 1.0;

    // Уменьшаем в 2 раза для компактного файла
    int outW = fftW / 2, outH = fftH / 2;
    std::vector<uint8_t> out(outW * outH);
    for (int y = 0; y < outH; y++)
        for (int x = 0; x < outW; x++)
            out[(size_t)y * outW + x] =
                (uint8_t)((logMag[(size_t)(y*2)*fftW + (x*2)] - minV) / rng * 200.0);

    // Отмечаем выбранные пиксели спектра белым (в масштабе 50%)
    for (auto& px : markedPixels) {
        int py = px.second / 2, ppx = px.first / 2;
        for (int dy = -3; dy <= 3; dy++)
            for (int dx = -3; dx <= 3; dx++) {
                int yy = py + dy, xx = ppx + dx;
                if (yy >= 0 && yy < outH && xx >= 0 && xx < outW)
                    out[(size_t)yy * outW + xx] = 255;
            }
    }
    savePng(fname, out, outW, outH);
}

// ================================================================
//  Главная функция
// ================================================================

int main(int argc, char* argv[]) {
    const char* fname = (argc > 1) ? argv[1] : "1.tif";

    // ----------------------------------------------------------
    // Шаг 1. Загружаем TIFF
    // ----------------------------------------------------------
    printf("=== Step 1: Loading TIFF: %s ===\n", fname);
    TiffImage tiff = loadTiff(fname);
    if (!tiff.valid) {
        printf("Error: %s\n", tiff.error.c_str());
        return 1;
    }
    int W = tiff.width, H = tiff.height;
    printf("  Size: %d x %d, %d bpp\n\n", W, H, tiff.bpp);

    savePng("out_original.png", tiff.pixels, W, H);
    printf("  Saved: out_original.png\n\n");

    // ----------------------------------------------------------
    // Шаг 2. Двумерное FFT
    // ----------------------------------------------------------
    int fftW = nextPow2(W);
    int fftH = nextPow2(H);
    printf("=== Step 2: 2D FFT ===\n");
    printf("  Image %dx%d -> FFT buffer %dx%d\n", W, H, fftW, fftH);
    printf("  Allocating %.1f MB...\n",
           (double)fftW * fftH * sizeof(Complex) / 1024.0 / 1024.0);

    std::vector<Complex> fftBuf((size_t)fftW * fftH, {0.0, 0.0});
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            fftBuf[(size_t)y * fftW + x] = { (double)tiff.pixels[(size_t)y * W + x], 0.0 };

    fft2d(fftBuf, fftW, fftH, false);
    fftshift(fftBuf, fftW, fftH);
    printf("  FFT done.\n\n");

    // ----------------------------------------------------------
    // Шаг 3. Логарифм амплитудного спектра
    // ----------------------------------------------------------
    printf("=== Step 3: Log-magnitude spectrum ===\n");
    size_t N = (size_t)fftW * fftH;
    std::vector<double> logMag(N);
    double sumL = 0, sumL2 = 0;
    for (size_t i = 0; i < N; i++) {
        logMag[i] = log(1.0 + std::abs(fftBuf[i]));
        sumL  += logMag[i];
        sumL2 += logMag[i] * logMag[i];
    }
    double meanL = sumL / (double)N;
    double stdL  = sqrt(sumL2 / (double)N - meanL * meanL);
    double threshold = meanL + 4.5 * stdL;
    printf("  mean=%.4f  std=%.4f  threshold=%.4f\n\n", meanL, stdL, threshold);

    // ----------------------------------------------------------
    // Шаг 4. Сегментация спектра — собираем яркие пиксели
    //        Исключаем DC-зону и горизонтальную полосу.
    //        Горизонтальные пики — артефакты границ страницы,
    //        они бы исказили оценку угла наклона текста.
    // ----------------------------------------------------------
    printf("=== Steps 4-5: Segmentation and moment computation ===\n");

    int cx0 = fftW / 2, cy0 = fftH / 2;
    int excludeR = (int)(0.03 * std::min(fftW, fftH));  // радиус DC-зоны
    int minCyAbs = (int)(0.01 * fftH);  // исключаем горизонтальную полосу

    // Собираем координаты ярких пикселей и их веса (logMag - порог)
    std::vector<std::pair<int,int>> spectralPx;
    std::vector<double> weights;

    for (int y = 1; y < fftH - 1; y++) {
        for (int x = 1; x < fftW - 1; x++) {
            if (logMag[(size_t)y * fftW + x] < threshold) continue;
            int dxC = x - cx0, dyC = y - cy0;
            // Исключаем DC-зону
            if (dxC * dxC + dyC * dyC < excludeR * excludeR) continue;
            // Исключаем горизонтальную полосу (|dy| < minCyAbs)
            if (std::abs(dyC) < minCyAbs) continue;
            spectralPx.push_back({x, y});
            weights.push_back(logMag[(size_t)y * fftW + x] - threshold);
        }
    }
    printf("  Bright spectrum pixels (after filtering): %zu\n", spectralPx.size());

    // ----------------------------------------------------------
    // Шаг 5. Взвешенные центральные моменты 2-го порядка (лекция 10)
    //        Вес пикселя = logMag - порог (чем ярче, тем важнее)
    // ----------------------------------------------------------
    double totalW = 0, sumX = 0, sumY = 0;
    for (size_t i = 0; i < spectralPx.size(); i++) {
        totalW += weights[i];
        sumX   += weights[i] * spectralPx[i].first;
        sumY   += weights[i] * spectralPx[i].second;
    }

    double phi_deg = 0.0;
    double correction_deg = 0.0;

    if (totalW > 0) {
        double centX = sumX / totalW;
        double centY = sumY / totalW;
        printf("  Peak centroid: cx=%.1f  cy=%.1f\n", centX - cx0, centY - cy0);

        // Центральные моменты 2-го порядка
        double mu20 = 0, mu02 = 0, mu11 = 0;
        for (size_t i = 0; i < spectralPx.size(); i++) {
            double dx = spectralPx[i].first  - centX;
            double dy = spectralPx[i].second - centY;
            mu20 += weights[i] * dx * dx;
            mu02 += weights[i] * dy * dy;
            mu11 += weights[i] * dx * dy;
        }
        mu20 /= totalW;
        mu02 /= totalW;
        mu11 /= totalW;
        printf("  mu20=%.2f  mu02=%.2f  mu11=%.2f\n", mu20, mu02, mu11);

        // ----------------------------------------------------------
        // Шаг 6. Главная ось инерции (лекция 10, слайд 21)
        //   phi = 0.5 * arctan(2*mu11 / (mu20 - mu02))
        //
        //   Пики в спектре вытянуты перпендикулярно строкам текста.
        //   Если текст горизонтальный — пики вертикальные (phi = 90).
        //   Если текст наклонён на alpha — phi = 90 + alpha.
        //   Угол коррекции = phi - 90 (насколько повернуть изображение).
        // ----------------------------------------------------------
        double phi_rad = 0.5 * atan2(2.0 * mu11, mu20 - mu02);
        phi_deg = phi_rad * 180.0 / PI;
        correction_deg = phi_deg - 90.0;

        // Угол коррекции должен быть в диапазоне (-45, 45) — небольшой наклон
        if (correction_deg >  45.0) correction_deg -= 90.0;
        if (correction_deg < -45.0) correction_deg += 90.0;

        printf("\n=== Step 6: Principal axis of inertia ===\n");
        printf("  Principal axis angle (phi): %.2f deg\n", phi_deg);
        printf("  Text skew:                  %.2f deg\n", phi_deg - 90.0);
        printf("  Correction angle:           %.2f deg\n\n", correction_deg);
    } else {
        printf("  Warning: no bright peaks found, skipping rotation.\n\n");
    }

    // Сохраняем спектр с отмеченными пиками
    saveSpectrum("out_spectrum.png", logMag, fftW, fftH, spectralPx);
    printf("  Saved: out_spectrum.png\n\n");

    // ----------------------------------------------------------
    // Шаг 7. Поворот исходного изображения (билинейная интерполяция)
    // ----------------------------------------------------------
    printf("=== Step 7: Rotating image by %.2f deg ===\n", correction_deg);
    auto corrected = rotateImage(tiff.pixels, W, H, correction_deg);
    savePng("out_corrected.png", corrected, W, H);
    printf("  Saved: out_corrected.png\n\n");

    printf("=== Done! ===\n");
    printf("  out_original.png  - original image\n");
    printf("  out_spectrum.png  - log spectrum (bright peaks marked white)\n");
    printf("  out_corrected.png - deskewed image\n");

    return 0;
}
