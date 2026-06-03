#include "lab5.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// ================================================================
//  Ранговый фильтр (rank=0: минимум, rank=N/2: медиана, rank=N-1: максимум)
// ================================================================

void rankFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH, int rank)
{
    int halfW = apertureW / 2;
    int halfH = apertureH / 2;
    std::vector<unsigned char> window(apertureW * apertureH);

    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            int idx = 0;
            for (int ky = -halfH; ky <= halfH; ++ky)
                for (int kx = -halfW; kx <= halfW; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    window[idx++] = in[iy * imgW + ix];
                }
            // nth_element: O(n) выбор k-го элемента — быстрее O(n log n) full sort
            std::nth_element(window.begin(), window.begin() + rank, window.end());
            out[y * imgW + x] = window[rank];
        }
    }
}

// ================================================================
//  Медианный фильтр — сортировкой через nth_element (O(n) выбор)
//  nth_element быстрее std::sort (O(n log n)) — только ищет k-й элемент
// ================================================================

void medianFilterSort(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH)
{
    int halfW = apertureW / 2;
    int halfH = apertureH / 2;
    int total = apertureW * apertureH;
    int medIdx = total / 2;
    std::vector<unsigned char> window(total);

    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            int idx = 0;
            for (int ky = -halfH; ky <= halfH; ++ky)
                for (int kx = -halfW; kx <= halfW; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    window[idx++] = in[iy * imgW + ix];
                }
            // nth_element: O(n) в среднем — быстрее O(n log n) full sort
            std::nth_element(window.begin(), window.begin() + medIdx, window.end());
            out[y * imgW + x] = window[medIdx];
        }
    }
}

// ================================================================
//  Быстрый медианный фильтр 3×3 — сеть сравнений (19 операций)
//  Классический opt_med9 (N. Devillard). Без ветвлений на значениях,
//  только условные перестановки — быстрее nth_element для 9 элементов.
// ================================================================

static inline void cs9(unsigned char& a, unsigned char& b) {
    if (a > b) { unsigned char t = a; a = b; b = t; }
}

static inline unsigned char median9network(unsigned char v[9]) {
    cs9(v[1],v[2]); cs9(v[4],v[5]); cs9(v[7],v[8]);
    cs9(v[0],v[1]); cs9(v[3],v[4]); cs9(v[6],v[7]);
    cs9(v[1],v[2]); cs9(v[4],v[5]); cs9(v[7],v[8]);
    cs9(v[0],v[3]); cs9(v[5],v[8]); cs9(v[4],v[7]);
    cs9(v[3],v[6]); cs9(v[1],v[4]); cs9(v[2],v[5]);
    cs9(v[4],v[7]); cs9(v[4],v[2]); cs9(v[6],v[4]);
    cs9(v[4],v[2]);
    return v[4];  // гарантированно медиана
}

void medianFilter3x3Fast(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH)
{
    unsigned char win[9];
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            int idx = 0;
            for (int ky = -1; ky <= 1; ++ky)
                for (int kx = -1; kx <= 1; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    win[idx++] = in[iy * imgW + ix];
                }
            out[y * imgW + x] = median9network(win);
        }
    }
}

// ================================================================
//  Медианный фильтр — алгоритм Хуанга (скользящая гистограмма)
//  Сложность: O(imgW*imgH*(kH + 256))  — не растёт с размером апертуры
//
//  Принцип: для каждой строки сначала строим гистограмму окна (kW x kH)
//  для x=0 (O(kW*kH)), затем скользим вправо: удаляем левый столбец
//  и добавляем правый (O(kH) на шаг). Медиана ищется за O(256).
//  Граничные пиксели обрабатываются с зеркальным clamp.
// ================================================================

void medianFilterHuang(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int kW, int kH)
{
    int halfW  = kW / 2;
    int halfH  = kH / 2;
    int medRank = (kW * kH) / 2;  // индекс медианы (0-based, нижняя)

    std::vector<int> hist(256, 0);

    for (int y = 0; y < (int)imgH; ++y) {
        // Строим начальную гистограмму для x=0
        std::fill(hist.begin(), hist.end(), 0);
        for (int ky = -halfH; ky <= halfH; ++ky) {
            int iy = std::max(0, std::min((int)imgH - 1, y + ky));
            for (int kx = -halfW; kx <= halfW; ++kx) {
                int ix = std::max(0, std::min((int)imgW - 1, kx));
                hist[in[iy * imgW + ix]]++;
            }
        }

        for (int x = 0; x < (int)imgW; ++x) {
            // Найти медиану за O(256)
            int count = 0;
            for (int v = 0; v <= 255; ++v) {
                count += hist[v];
                if (count > medRank) {
                    out[y * imgW + x] = (unsigned char)v;
                    break;
                }
            }

            // Сдвиг вправо: удалить столбец x-halfW, добавить столбец x+halfW+1
            if (x + 1 < (int)imgW) {
                int removeX = x - halfW;
                int addX    = x + halfW + 1;
                for (int ky = -halfH; ky <= halfH; ++ky) {
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    int rxc = std::max(0, std::min((int)imgW - 1, removeX));
                    hist[in[iy * imgW + rxc]]--;
                    int axc = std::max(0, std::min((int)imgW - 1, addX));
                    hist[in[iy * imgW + axc]]++;
                }
            }
        }
    }
}

// Автовыбор реализации по размеру апертуры:
//   3×3        → сеть сравнений (medianFilter3x3Fast, 19 операций)
//   5×5..7×7   → nth_element / сортировка (medianFilterSort)
//   9×9 и выше → алгоритм Хуанга (скользящая гистограмма)
void medianFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH)
{
    if (apertureW == 3 && apertureH == 3)
        medianFilter3x3Fast(in, out, imgW, imgH);
    else if (apertureW * apertureH <= 49)   // ≤ 7×7
        medianFilterSort(in, out, imgW, imgH, apertureW, apertureH);
    else
        medianFilterHuang(in, out, imgW, imgH, apertureW, apertureH);
}

// ================================================================
//  Фильтр усечённого среднего (trimCount крайних значений отбрасывается)
// ================================================================

void trimmedMeanFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int apertureW, int apertureH,
    int trimCount)
{
    int halfW = apertureW / 2;
    int halfH = apertureH / 2;
    int total = apertureW * apertureH;
    std::vector<unsigned char> window(total);

    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            int idx = 0;
            for (int ky = -halfH; ky <= halfH; ++ky)
                for (int kx = -halfW; kx <= halfW; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    window[idx++] = in[iy * imgW + ix];
                }
            // Два вызова nth_element вместо полной сортировки:
            // 1-й: гарантирует, что window[0..trimCount-1] — trimCount наименьших
            std::nth_element(window.begin(),
                             window.begin() + trimCount,
                             window.end());
            // 2-й: среди оставшихся [trimCount..total-1] перемещает trimCount
            //      наибольших в конец [total-trimCount..total-1]
            std::nth_element(window.begin() + trimCount,
                             window.begin() + (total - trimCount),
                             window.end());
            // Суммируем только средние элементы [trimCount .. total-trimCount-1]
            double sum = 0;
            int count = total - 2 * trimCount;
            for (int i = trimCount; i < total - trimCount; ++i)
                sum += window[i];
            out[y * imgW + x] = (unsigned char)(count > 0 ? sum / count : 0);
        }
    }
}

// ================================================================
//  Усредняющий фильтр (для сравнения PSNR)
// ================================================================

void meanFilter(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    int half = size / 2;
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            double sum = 0;
            int count = 0;
            for (int ky = -half; ky <= half; ++ky)
                for (int kx = -half; kx <= half; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    sum += in[iy * imgW + ix];
                    ++count;
                }
            out[y * imgW + x] = (unsigned char)(sum / count);
        }
    }
}

// ================================================================
//  PSNR
// ================================================================

double computePSNR(const unsigned char* orig, const unsigned char* filtered,
    size_t total)
{
    double mse = 0;
    for (size_t i = 0; i < total; ++i) {
        double d = (double)orig[i] - (double)filtered[i];
        mse += d * d;
    }
    mse /= total;
    if (mse < 1e-10) return 999.0;
    return 10.0 * log10(255.0 * 255.0 / mse);
}

// ================================================================
//  Добавление шума (Бокс-Мюллер для гауссова, равномерный для импульсного)
// ================================================================

void addGaussianNoise(const unsigned char* in, unsigned char* out,
    size_t total, double sigma)
{
    srand(42);
    for (size_t i = 0; i < total; ++i) {
        double u1 = (double)(rand() + 1) / (RAND_MAX + 1.0);
        double u2 = (double)rand() / RAND_MAX;
        double noise = sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159 * u2) * sigma;
        int val = (int)(in[i] + noise);
        out[i] = (unsigned char)(val < 0 ? 0 : val > 255 ? 255 : val);
    }
}

void addImpulseNoise(const unsigned char* in, unsigned char* out,
    size_t total, double density)
{
    srand(42);
    for (size_t i = 0; i < total; ++i) out[i] = in[i];
    for (size_t i = 0; i < total; ++i) {
        double r = (double)rand() / RAND_MAX;
        if (r < density / 2)  out[i] = 0;
        else if (r < density) out[i] = 255;
    }
}

// ================================================================
//  Морфологические операции (structuring element — квадрат size×size)
// ================================================================

void morphErode(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    int half = size / 2;
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            unsigned char minVal = 255;
            for (int ky = -half; ky <= half; ++ky)
                for (int kx = -half; kx <= half; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    minVal = std::min(minVal, in[iy * imgW + ix]);
                }
            out[y * imgW + x] = minVal;
        }
    }
}

void morphDilate(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    int half = size / 2;
    for (int y = 0; y < (int)imgH; ++y) {
        for (int x = 0; x < (int)imgW; ++x) {
            unsigned char maxVal = 0;
            for (int ky = -half; ky <= half; ++ky)
                for (int kx = -half; kx <= half; ++kx) {
                    int ix = std::max(0, std::min((int)imgW - 1, x + kx));
                    int iy = std::max(0, std::min((int)imgH - 1, y + ky));
                    maxVal = std::max(maxVal, in[iy * imgW + ix]);
                }
            out[y * imgW + x] = maxVal;
        }
    }
}

void morphOpen(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    std::vector<unsigned char> tmp(imgW * imgH);
    morphErode(in, tmp.data(), imgW, imgH, size);
    morphDilate(tmp.data(), out, imgW, imgH, size);
}

void morphClose(const unsigned char* in, unsigned char* out,
    size_t imgW, size_t imgH, int size)
{
    std::vector<unsigned char> tmp(imgW * imgH);
    morphDilate(in, tmp.data(), imgW, imgH, size);
    morphErode(tmp.data(), out, imgW, imgH, size);
}