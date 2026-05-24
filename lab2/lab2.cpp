#include "lab2.h"
#include "PngProc.h"
#include <cmath>
#include <algorithm>

const double PI = 3.14159265358979323846;

// ================================================================
//  1D FFT (Cooley-Tukey, рекурсивный)
// ================================================================

void fft1d(std::vector<Complex>& a, bool inverse)
{
    size_t n = a.size();
    if (n == 1) return;

    std::vector<Complex> even(n / 2), odd(n / 2);
    for (size_t i = 0; i < n / 2; ++i) {
        even[i] = a[2 * i];
        odd[i] = a[2 * i + 1];
    }

    fft1d(even, inverse);
    fft1d(odd, inverse);

    double ang = 2 * PI / n * (inverse ? 1 : -1);
    Complex w(1), wn(cos(ang), sin(ang));

    for (size_t i = 0; i < n / 2; ++i) {
        a[i] = even[i] + w * odd[i];
        a[i + n / 2] = even[i] - w * odd[i];
        if (inverse) {
            a[i] /= 2;
            a[i + n / 2] /= 2;
        }
        w *= wn;
    }
}

// ================================================================
//  Дополнение до степени 2
// ================================================================

std::vector<Complex> padToPow2(const std::vector<Complex>& in)
{
    size_t n = 1;
    while (n < in.size()) n <<= 1;
    std::vector<Complex> out(n, Complex(0, 0));
    for (size_t i = 0; i < in.size(); ++i)
        out[i] = in[i];
    return out;
}

// ================================================================
//  2D FFT
// ================================================================

void fft2d(const unsigned char* img, size_t width, size_t height,
    std::vector<std::vector<Complex>>& spectrum,
    size_t& outW, size_t& outH)
{
    // Находим размеры до степени 2
    outW = 1; while (outW < width)  outW <<= 1;
    outH = 1; while (outH < height) outH <<= 1;

    // Инициализируем матрицу
    spectrum.assign(outH, std::vector<Complex>(outW, Complex(0, 0)));
    for (size_t y = 0; y < height; ++y)
        for (size_t x = 0; x < width; ++x)
            spectrum[y][x] = Complex(img[y * width + x], 0);

    // FFT по строкам
    for (size_t y = 0; y < outH; ++y)
        fft1d(spectrum[y]);

    // FFT по столбцам
    for (size_t x = 0; x < outW; ++x) {
        std::vector<Complex> col(outH);
        for (size_t y = 0; y < outH; ++y)
            col[y] = spectrum[y][x];
        fft1d(col);
        for (size_t y = 0; y < outH; ++y)
            spectrum[y][x] = col[y];
    }
}

// ================================================================
//  Сохранение логарифма амплитудного спектра (fftshift)
// ================================================================

void saveSpectrum(const std::vector<std::vector<Complex>>& spectrum,
    size_t outW, size_t outH,
    const char* filename)
{
    // Вычисляем логарифм амплитуды
    std::vector<double> logAmp(outW * outH);
    for (size_t y = 0; y < outH; ++y)
        for (size_t x = 0; x < outW; ++x)
            logAmp[y * outW + x] = log(1.0 + std::abs(spectrum[y][x]));

    // Нормализуем в 0..255
    double maxVal = *std::max_element(logAmp.begin(), logAmp.end());
    double minVal = *std::min_element(logAmp.begin(), logAmp.end());

    std::vector<unsigned char> out(outW * outH);

    // fftshift — переносим низкие частоты в центр
    size_t halfW = outW / 2;
    size_t halfH = outH / 2;
    for (size_t y = 0; y < outH; ++y) {
        for (size_t x = 0; x < outW; ++x) {
            size_t sx = (x + halfW) % outW;
            size_t sy = (y + halfH) % outH;
            double val = (logAmp[y * outW + x] - minVal) / (maxVal - minVal) * 255.0;
            out[sy * outW + sx] = (unsigned char)(val + 0.5);
        }
    }

    NPngProc::writePngFile(filename, out.data(), outW, outH, 8);
}

// ================================================================
//  Генерация тестового 1D сигнала из смеси синусоид
// ================================================================

std::vector<Complex> makeSineSignal(const std::vector<double>& freqs, size_t n)
{
    std::vector<Complex> sig(n);
    for (size_t i = 0; i < n; ++i) {
        double val = 0;
        for (double f : freqs)
            val += sin(2 * PI * f * i / n);
        sig[i] = Complex(val, 0);
    }
    return sig;
}

// ================================================================
//  Генерация 2D изображения из смеси синусоид
// ================================================================

void makeSineImage(unsigned char* img, size_t width, size_t height,
    const std::vector<std::pair<double, double>>& freqs)
{
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            double val = 0;
            for (auto& f : freqs)
                val += sin(2 * PI * (f.first * x / width + f.second * y / height));
            // Нормализуем в 0..255
            val = val / freqs.size();
            val = (val + 1.0) / 2.0 * 255.0;
            img[y * width + x] = (unsigned char)(std::max(0.0, std::min(255.0, val)));
        }
    }
}