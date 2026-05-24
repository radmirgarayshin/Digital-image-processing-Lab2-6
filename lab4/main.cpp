#include <stdio.h>
#include "PngProc.h"
#include "lab4.h"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: lab4 <input.png>\n");
        return -1;
    }

    const char* inputFile = argv[1];
    size_t nWidth = 0, nHeight = 0;
    size_t nSize = NPngProc::readPngFileGray(inputFile, nullptr, &nWidth, &nHeight);
    if (nSize == NPngProc::PNG_ERROR) {
        printf("Error reading: %s\n", inputFile);
        return -1;
    }

    unsigned char* pIn = new unsigned char[nSize];
    unsigned char* pOut = new unsigned char[nSize];
    unsigned char* pTmp = new unsigned char[nSize];
    NPngProc::readPngFileGray(inputFile, pIn, &nWidth, &nHeight);
    printf("Image: %s (%zux%zu)\n\n", inputFile, nWidth, nHeight);

    // --- ФНЧ на чистом изображении ---
    lowPassFilter(pIn, pOut, nWidth, nHeight, 5);
    NPngProc::writePngFile("out_lpf_clean.png", pOut, nWidth, nHeight, 8);
    printf("LPF (clean): out_lpf_clean.png\n");

    // --- Гауссов шум + ФНЧ ---
    // Добавляем гауссов шум вручную
    srand(42);
    for (size_t i = 0; i < nWidth * nHeight; ++i) {
        double u1 = (double)(rand() + 1) / (RAND_MAX + 1.0);
        double u2 = (double)rand() / RAND_MAX;
        double noise = sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159 * u2) * 20.0;
        int val = (int)(pIn[i] + noise);
        pTmp[i] = (unsigned char)(val < 0 ? 0 : val > 255 ? 255 : val);
    }
    NPngProc::writePngFile("out_gauss_noise.png", pTmp, nWidth, nHeight, 8);
    lowPassFilter(pTmp, pOut, nWidth, nHeight, 5);
    NPngProc::writePngFile("out_lpf_gauss.png", pOut, nWidth, nHeight, 8);
    printf("LPF (gaussian noise): out_lpf_gauss.png\n");

    // --- Импульсный шум + ФНЧ ---
    addImpulseNoise(pIn, pTmp, nWidth, nHeight, 0.05);
    NPngProc::writePngFile("out_impulse_noise.png", pTmp, nWidth, nHeight, 8);
    lowPassFilter(pTmp, pOut, nWidth, nHeight, 5);
    NPngProc::writePngFile("out_lpf_impulse.png", pOut, nWidth, nHeight, 8);
    printf("LPF (impulse noise):  out_lpf_impulse.png\n");

    // --- Усредняющий фильтр с порогом ---
    addImpulseNoise(pIn, pTmp, nWidth, nHeight, 0.05);
    thresholdAvgFilter(pTmp, pOut, nWidth, nHeight, 5, 40.0);
    NPngProc::writePngFile("out_threshold_avg.png", pOut, nWidth, nHeight, 8);
    printf("Threshold avg filter: out_threshold_avg.png\n");

    // --- Лапласиан ---
    laplacianFilter(pIn, pOut, nWidth, nHeight);
    NPngProc::writePngFile("out_laplacian.png", pOut, nWidth, nHeight, 8);
    printf("Laplacian:            out_laplacian.png\n");

    // --- LoG ---
    logFilter(pIn, pOut, nWidth, nHeight, 2.0);
    NPngProc::writePngFile("out_log.png", pOut, nWidth, nHeight, 8);
    printf("LoG:                  out_log.png\n");

    // --- Детектирование границ через нулевой переход ---
    logFilter(pIn, pTmp, nWidth, nHeight, 2.0);
    zeroCrossing(pTmp, pOut, nWidth, nHeight);
    NPngProc::writePngFile("out_zero_crossing.png", pOut, nWidth, nHeight, 8);
    printf("Zero crossing:        out_zero_crossing.png\n");

    // --- Повышение резкости ---
    sharpenFilter(pIn, pOut, nWidth, nHeight, 0.8);
    NPngProc::writePngFile("out_sharpen.png", pOut, nWidth, nHeight, 8);
    printf("Sharpen:              out_sharpen.png\n");

    delete[] pIn;
    delete[] pOut;
    delete[] pTmp;
    printf("\nDone!\n");
    return 0;
}