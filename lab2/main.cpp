#include <stdio.h>
#include "PngProc.h"
#include "lab2.h"

int main(int argc, char* argv[])
{
    // === Step 1: 1D FFT: mix of sinusoids ===
    printf("=== 1D FFT: mix of sinusoids ===\n");
    std::vector<double> freqs1d = { 5, 13, 30 };
    size_t sigLen = 256;
    auto signal = makeSineSignal(freqs1d, sigLen);

    // Save original signal waveform
    saveSignal(signal, "signal_1d.png");
    printf("1D signal saved: signal_1d.png\n");

    // FFT
    auto padded = padToPow2(signal);
    fft1d(padded);

    // Save 1D spectrum as bar chart (fftshift, all 3 frequency peaks visible)
    saveSpectrum1D(padded, "spectrum_1d.png");
    printf("1D spectrum saved: spectrum_1d.png  (freqs: 5, 13, 30 -> 6 peaks symmetric around DC)\n");

    // === Step 2: 2D FFT: sine image ===
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

    // === Step 3: FFT real image ===
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