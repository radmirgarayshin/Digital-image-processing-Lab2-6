#pragma once
#include <cstddef>
#include <vector>
#include <complex>

using Complex = std::complex<double>;

// 1D FFT (размер должен быть степенью 2, иначе дополн€ем нул€ми)
void fft1d(std::vector<Complex>& a, bool inverse = false);

// ƒополнить вектор нул€ми до ближайшей степени 2
std::vector<Complex> padToPow2(const std::vector<Complex>& in);

// 2D FFT дл€ изображени€ (width и height будут округлены до степени 2)
// –езультат Ч двумерный массив комплексных чисел размером outW x outH
void fft2d(const unsigned char* img, size_t width, size_t height,
    std::vector<std::vector<Complex>>& spectrum,
    size_t& outW, size_t& outH);

// —охранить логарифм амплитудного спектра в PNG
// Ќизкие частоты Ч в центре (fftshift)
void saveSpectrum(const std::vector<std::vector<Complex>>& spectrum,
    size_t outW, size_t outH,
    const char* filename);

// √енераци€ 1D сигнала из смеси синусоид
// freqs Ч массив частот, n Ч количество отсчЄтов
std::vector<Complex> makeSineSignal(const std::vector<double>& freqs, size_t n);

// √енераци€ 2D изображени€ из смеси синусоид
void makeSineImage(unsigned char* img, size_t width, size_t height,
    const std::vector<std::pair<double, double>>& freqs);