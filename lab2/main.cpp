#include <stdio.h>
#include "PngProc.h"
#include "lab2.h"

int main(int argc, char* argv[])
{
    // === Тест 1: 1D сигнал из смеси синусоид ===
    printf("=== 1D FFT: mix of sinusoids ===\n");
    std::vector<double> freqs1d = { 5, 13, 30 };
    size_t sigLen = 256;
    auto signal = makeSineSignal(freqs1d, sigLen);
    auto padded = padToPow2(signal);
    fft1d(padded);

    size_t specLen = padded.size();
    std::vector<std::vector<Complex>> spec1d(specLen, std::vector<Complex>(1));
    for (size_t i = 0; i < specLen; ++i)
        spec1d[i][0] = padded[i];
    saveSpectrum(spec1d, 1, specLen, "spectrum_1d.png");
    printf("1D spectrum saved: spectrum_1d.png\n");

    // === Тест 2: 2D изображение из смеси синусоид ===
    printf("\n=== 2D FFT: sine image ===\n");
    size_t W = 256, H = 256;
    std::vector<unsigned char> sineImg(W * H);
    std::vector<std::pair<double, double>> freqs2d = { {3,0},{0,5},{7,4} };
    makeSineImage(sineImg.data(), W, H, freqs2d);
    NPngProc::writePngFile("sine_image.png", sineImg.data(), W, H, 8);
    printf("Sine image saved: sine_image.png\n");

    std::vector<std::vector<Complex>> spectrum;
    size_t outW, outH;
    fft2d(sineImg.data(), W, H, spectrum, outW, outH);
    saveSpectrum(spectrum, outW, outH, "spectrum_2d_sine.png");
    printf("2D sine spectrum saved: spectrum_2d_sine.png\n");

    // === Тест 3: FFT реального изображения ===
    if (argc >= 2) {
        printf("\n=== 2D FFT: real image (%s) ===\n", argv[1]);
        size_t nWidth = 0, nHeight = 0;
        size_t nSize = NPngProc::readPngFileGray(argv[1], nullptr, &nWidth, &nHeight);
        if (nSize != NPngProc::PNG_ERROR) {
            std::vector<unsigned char> img(nSize);
            NPngProc::readPngFileGray(argv[1], img.data(), &nWidth, &nHeight);
            fft2d(img.data(), nWidth, nHeight, spectrum, outW, outH);
            saveSpectrum(spectrum, outW, outH, "spectrum_2d_real.png");
            printf("2D real image spectrum saved: spectrum_2d_real.png\n");
        }
    }

    printf("\nDone!\n");
    return 0;
}