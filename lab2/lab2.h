#pragma once
#include <cstddef>
#include <vector>
#include <complex>

using Complex = std::complex<double>;

// 1D FFT (������ ������ ���� �������� 2, ����� ��������� ������)
void fft1d(std::vector<Complex>& a, bool inverse = false);

// ��������� ������ ������ �� ��������� ������� 2
std::vector<Complex> padToPow2(const std::vector<Complex>& in);

// 2D FFT ��� ����������� (width � height ����� ��������� �� ������� 2)
// ��������� � ��������� ������ ����������� ����� �������� outW x outH
void fft2d(const unsigned char* img, size_t width, size_t height,
    std::vector<std::vector<Complex>>& spectrum,
    size_t& outW, size_t& outH);

// ��������� �������� ������������ ������� � PNG
// ������ ������� � � ������ (fftshift)
void saveSpectrum(const std::vector<std::vector<Complex>>& spectrum,
    size_t outW, size_t outH,
    const char* filename);

// ��������� 1D ������� �� ����� ��������
// freqs � ������ ������, n � ���������� ��������
std::vector<Complex> makeSineSignal(const std::vector<double>& freqs, size_t n);

// ��������� 2D ����������� �� ����� ��������
void makeSineImage(unsigned char* img, size_t width, size_t height,
    const std::vector<std::pair<double, double>>& freqs);

// Сохранение 1D сигнала как осциллограмма (PNG, белая линия на чёрном фоне)
void saveSignal(const std::vector<Complex>& signal, const char* filename);

// Сохранение 1D амплитудного спектра как столбчатая диаграмма (fftshift, PNG)
void saveSpectrum1D(const std::vector<Complex>& spectrum, const char* filename);