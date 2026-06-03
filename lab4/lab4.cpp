#include "lab4.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

static double clamp(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ================================================================
//  Двумерная свёртка для произвольного ядра kW×kH
// ================================================================

void convolve2d(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH,
    const double* kernel, int kW, int kH)
{
    int kHalfW = kW / 2;
    int kHalfH = kH / 2;

    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            double sum = 0.0;
            for (int ky = 0; ky < kH; ++ky) {
                for (int kx = 0; kx < kW; ++kx) {
                    int ix = x + kx - kHalfW;
                    int iy = y + ky - kHalfH;
                    ix = (int)clamp(ix, 0, imgW - 1);
                    iy = (int)clamp(iy, 0, imgH - 1);
                    sum += in[iy * imgW + ix] * kernel[ky * kW + kx];
                }
            }
            out[y * imgW + x] = (unsigned char)clamp(sum, 0, 255);
        }
    }
}

// ================================================================
//  ФНЧ с усредняющим ядром (box filter) размером size×size
// ================================================================

void lowPassFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    int n = size * size;
    std::vector<double> kernel(n, 1.0 / n);
    convolve2d(in, out, imgW, imgH, kernel.data(), size, size);
}

// ================================================================
//  Усредняющий фильтр с порогом: усредняет только пиксели,
//  яркость которых отличается от центра не более чем на threshold
// ================================================================

void thresholdAvgFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size, double threshold)
{
    int half = size / 2;
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            double sum = 0.0;
            int count = 0;
            int center = in[y * imgW + x];
            for (int ky = -half; ky <= half; ++ky) {
                for (int kx = -half; kx <= half; ++kx) {
                    int ix = (int)clamp(x + kx, 0, imgW - 1);
                    int iy = (int)clamp(y + ky, 0, imgH - 1);
                    int val = in[iy * imgW + ix];
                    if (fabs(val - center) <= threshold) {
                        sum += val;
                        ++count;
                    }
                }
            }
            out[y * imgW + x] = (unsigned char)(sum / count);
        }
    }
}

// ================================================================
//  Добавление импульсного шума (salt & pepper)
// ================================================================

void addImpulseNoise(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double density)
{
    srand((unsigned)time(0));
    size_t total = imgW * imgH;
    for (size_t i = 0; i < total; ++i) out[i] = in[i];
    for (size_t i = 0; i < total; ++i) {
        double r = (double)rand() / RAND_MAX;
        if (r < density / 2)       out[i] = 0;
        else if (r < density)      out[i] = 255;
    }
}

// ================================================================
//  Лапласиан — правильный дискретный аналог Δf = d²f/dx² + d²f/dy²
//
//  Ядро крестообразное: ненулевые только центральная строка и столбец.
//  Коэффициенты берутся из формулы конечных разностей для d²/dx².
//  Для размера 3: [1,-2,1], для 5: [-1,16,-30,16,-1]/12, и т.д.
//  Работает для любого нечётного размера, не только 3/5/7.
//  Результат сдвигается на +128, чтобы отрицательные значения были видны.
// ================================================================

void laplacianFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    if (size < 3) size = 3;
    if (size % 2 == 0) size++;   // нечётный размер

    int half = size / 2;
    int n = size * size;
    std::vector<double> kernel(n, 0.0);

    // Считаем коэффициенты 1D стенсиля d²/dx² для произвольного half = size/2.
    // ratio накапливается как произведение (half-k+1)/(half+k) — так не нужны факториалы.
    // Знаки чередуются: нечётные k — плюс, чётные — минус.
    // Центральный коэффициент: c0 = -2*(1 + 1/4 + 1/9 + ... + 1/half²).
    std::vector<double> s(size, 0.0);
    {
        double ratio = 1.0;
        double c0 = 0.0;
        for (int k = 1; k <= half; ++k) {
            ratio *= (double)(half - k + 1) / (double)(half + k);
            double ck = 2.0 * ratio / (double)(k * k);
            if (k % 2 == 0) ck = -ck;  // чётные k — отрицательные
            s[half + k] = ck;
            s[half - k] = ck;  // стенсиль симметричный
            c0 -= 2.0 / (double)(k * k);
        }
        s[half] = c0;
    }

    // Складываем стенсиль по строке (d²/dx²) и по столбцу (d²/dy²).
    // Центральный пиксель попадает в оба — это правильно, для 3x3 даёт -4.
    for (int k = 0; k < size; ++k) {
        kernel[half * size + k] += s[k];  // центральная строка
        kernel[k * size + half] += s[k];  // центральный столбец
    }

    // Применяем ядро, сдвиг +128 чтобы отрицательные значения были серыми
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            double sum = 0.0;
            for (int ky = 0; ky < size; ++ky) {
                for (int kx = 0; kx < size; ++kx) {
                    if (kernel[ky * size + kx] == 0.0) continue;
                    int ix = (int)clamp(x + kx - half, 0, imgW - 1);
                    int iy = (int)clamp(y + ky - half, 0, imgH - 1);
                    sum += in[iy * imgW + ix] * kernel[ky * size + kx];
                }
            }
            out[y * imgW + x] = (unsigned char)clamp(sum + 128, 0, 255);
        }
    }
}

// ================================================================
//  LoG (Лапласиан Гаусса): сначала размытие гауссом, затем лапласиан
//  Размер ядра выбирается автоматически: size = 6*sigma (нечётное)
// ================================================================

void logFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double sigma)
{
    int size = (int)(6 * sigma) | 1; // нечётный размер ~6σ
    if (size < 3) size = 3;
    int half = size / 2;

    std::vector<double> kernel(size * size);
    double sigma2 = sigma * sigma;
    double sum = 0.0;

    for (int y = -half; y <= half; ++y) {
        for (int x = -half; x <= half; ++x) {
            double r2 = x * x + y * y;
            double v = (r2 - 2 * sigma2) / (sigma2 * sigma2)
                * exp(-r2 / (2 * sigma2));
            kernel[(y + half) * size + (x + half)] = v;
            sum += v;
        }
    }
    // Нормализация (убираем постоянную составляющую)
    for (auto& k : kernel) k -= sum / (size * size);

    // Применяем, результат смещён на +128 для отображения
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            double s = 0.0;
            for (int ky = 0; ky < size; ++ky) {
                for (int kx = 0; kx < size; ++kx) {
                    int ix = (int)clamp(x + kx - half, 0, imgW - 1);
                    int iy = (int)clamp(y + ky - half, 0, imgH - 1);
                    s += in[iy * imgW + ix] * kernel[ky * size + kx];
                }
            }
            out[y * imgW + x] = (unsigned char)clamp(s + 128, 0, 255);
        }
    }
}

// ================================================================
//  Детектирование границ через нулевой переход LoG
//  Граница = смена знака между соседними пикселями LoG-образа
// ================================================================

void zeroCrossing(const unsigned char* log_img, unsigned char* out,
    size_t imgW, size_t imgH)
{
    for (size_t i = 0; i < imgW * imgH; ++i) out[i] = 0;

    for (int y = 1; y < (int)imgH - 1; ++y) {
        for (int x = 1; x < (int)imgW - 1; ++x) {
            int c = (int)log_img[y * imgW + x] - 128;
            // Проверяем смену знака по горизонтали и вертикали
            int r = (int)log_img[y * imgW + x + 1] - 128;
            int d = (int)log_img[(y + 1) * imgW + x] - 128;
            if ((c > 0 && r < 0) || (c < 0 && r > 0) ||
                (c > 0 && d < 0) || (c < 0 && d > 0))
                out[y * imgW + x] = 255;
        }
    }
}

// ================================================================
//  Повышение резкости (unsharp masking) — конфигурируемый размер ядра
//  Ядро size×size: все -amount, центр = 1 + (size*size-1)*amount
//  Сумма ядра = 1 (яркость сохраняется)
// ================================================================

void sharpenFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, double amount, int size)
{
    if (size < 1) size = 1;
    if (size % 2 == 0) size++;

    int n = size * size;
    std::vector<double> kernel(n, -amount);
    kernel[n / 2] = 1.0 + (double)(n - 1) * amount;

    convolve2d(in, out, imgW, imgH, kernel.data(), size, size);
}