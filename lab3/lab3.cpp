#include "lab3.h"
#include <cmath>
#include <algorithm>

const double PI = 3.14159265358979323846;

// ================================================================
//  Вспомогательные функции интерполяции
// ================================================================

static double clamp(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Получить пиксель с зеркальным отражением на границе
static unsigned char getPixel(const unsigned char* img, int x, int y,
    int w, int h) {
    x = (int)clamp(x, 0, w - 1);
    y = (int)clamp(y, 0, h - 1);
    return img[y * w + x];
}

// Ближайший сосед
static unsigned char nearestNeighbor(const unsigned char* img,
    double x, double y, int w, int h) {
    return getPixel(img, (int)round(x), (int)round(y), w, h);
}

// Билинейная интерполяция
static unsigned char bilinear(const unsigned char* img,
    double x, double y, int w, int h) {
    int x0 = (int)floor(x), y0 = (int)floor(y);
    int x1 = x0 + 1, y1 = y0 + 1;
    double dx = x - x0, dy = y - y0;

    double v00 = getPixel(img, x0, y0, w, h);
    double v10 = getPixel(img, x1, y0, w, h);
    double v01 = getPixel(img, x0, y1, w, h);
    double v11 = getPixel(img, x1, y1, w, h);

    double val = v00 * (1 - dx) * (1 - dy) + v10 * dx * (1 - dy)
        + v01 * (1 - dx) * dy + v11 * dx * dy;
    return (unsigned char)clamp(val, 0, 255);
}

// Весовой коэффициент бикубической интерполяции (Keys, a=-0.5)
static double cubicWeight(double t) {
    double a = -0.5;
    t = fabs(t);
    if (t < 1.0) return (a + 2) * t * t * t - (a + 3) * t * t + 1;
    if (t < 2.0) return a * t * t * t - 5 * a * t * t + 8 * a * t - 4 * a;
    return 0.0;
}

// Бикубическая интерполяция (окно 4×4)
static unsigned char bicubic(const unsigned char* img,
    double x, double y, int w, int h) {
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    double val = 0.0;
    for (int dy = -1; dy <= 2; ++dy)
        for (int dx = -1; dx <= 2; ++dx) {
            double p = getPixel(img, x0 + dx, y0 + dy, w, h);
            val += p * cubicWeight(x - (x0 + dx)) * cubicWeight(y - (y0 + dy));
        }
    return (unsigned char)clamp(val, 0, 255);
}

// ================================================================
//  Размер повёрнутого изображения (вписывающий прямоугольник)
// ================================================================

void getRotatedSize(size_t inW, size_t inH, double angleDeg,
    size_t& outW, size_t& outH) {
    double rad = angleDeg * PI / 180.0;
    double cosA = fabs(cos(rad));
    double sinA = fabs(sin(rad));
    outW = (size_t)ceil(inW * cosA + inH * sinA);
    outH = (size_t)ceil(inW * sinA + inH * cosA);
}

// ================================================================
//  Поворот (ИСПРАВЛЕННАЯ ВЕРСИЯ): switch вынесен за пределы цикла
//  — условие проверяется один раз, а не для каждого пикселя
// ================================================================

void rotateImage(const unsigned char* in, size_t inW, size_t inH,
    unsigned char* out, size_t outW, size_t outH,
    double angleDeg, InterpolationMethod method)
{
    double rad  = -angleDeg * PI / 180.0;
    double cosA = cos(rad);
    double sinA = sin(rad);

    double cx_in  = inW / 2.0,  cy_in  = inH / 2.0;
    double cx_out = outW / 2.0, cy_out = outH / 2.0;

    // switch OUTSIDE the loop: один цикл на метод
    switch (method) {

    case NEAREST_NEIGHBOR:
        for (size_t y = 0; y < outH; ++y) {
            for (size_t x = 0; x < outW; ++x) {
                double dx   = x - cx_out, dy = y - cy_out;
                double srcX = cosA * dx - sinA * dy + cx_in;
                double srcY = sinA * dx + cosA * dy + cy_in;
                out[y * outW + x] = (srcX >= 0 && srcX < inW && srcY >= 0 && srcY < inH)
                    ? nearestNeighbor(in, srcX, srcY, (int)inW, (int)inH) : 0;
            }
        }
        break;

    case BILINEAR:
        for (size_t y = 0; y < outH; ++y) {
            for (size_t x = 0; x < outW; ++x) {
                double dx   = x - cx_out, dy = y - cy_out;
                double srcX = cosA * dx - sinA * dy + cx_in;
                double srcY = sinA * dx + cosA * dy + cy_in;
                out[y * outW + x] = (srcX >= 0 && srcX < inW && srcY >= 0 && srcY < inH)
                    ? bilinear(in, srcX, srcY, (int)inW, (int)inH) : 0;
            }
        }
        break;

    case BICUBIC:
        for (size_t y = 0; y < outH; ++y) {
            for (size_t x = 0; x < outW; ++x) {
                double dx   = x - cx_out, dy = y - cy_out;
                double srcX = cosA * dx - sinA * dy + cx_in;
                double srcY = sinA * dx + cosA * dy + cy_in;
                out[y * outW + x] = (srcX >= 0 && srcX < inW && srcY >= 0 && srcY < inH)
                    ? bicubic(in, srcX, srcY, (int)inW, (int)inH) : 0;
            }
        }
        break;
    }
}

// ================================================================
//  Поворот (СТАРАЯ ВЕРСИЯ для сравнения): switch ВНУТРИ цикла
// ================================================================

void rotateImageOld(const unsigned char* in, size_t inW, size_t inH,
    unsigned char* out, size_t outW, size_t outH,
    double angleDeg, InterpolationMethod method)
{
    double rad  = -angleDeg * PI / 180.0;
    double cosA = cos(rad);
    double sinA = sin(rad);

    double cx_in  = inW / 2.0,  cy_in  = inH / 2.0;
    double cx_out = outW / 2.0, cy_out = outH / 2.0;

    for (size_t y = 0; y < outH; ++y) {
        for (size_t x = 0; x < outW; ++x) {
            double dx   = x - cx_out, dy = y - cy_out;
            double srcX = cosA * dx - sinA * dy + cx_in;
            double srcY = sinA * dx + cosA * dy + cy_in;

            unsigned char val = 0;
            if (srcX >= 0 && srcX < inW && srcY >= 0 && srcY < inH) {
                switch (method) {             // switch ВНУТРИ — проверка на каждый пиксель
                case NEAREST_NEIGHBOR:
                    val = nearestNeighbor(in, srcX, srcY, (int)inW, (int)inH); break;
                case BILINEAR:
                    val = bilinear(in, srcX, srcY, (int)inW, (int)inH); break;
                case BICUBIC:
                    val = bicubic(in, srcX, srcY, (int)inW, (int)inH); break;
                }
            }
            out[y * outW + x] = val;
        }
    }
}