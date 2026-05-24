#include <stdio.h>
#include "PngProc.h"
#include "lab5.h"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: lab5 <input.png>\n");
        return -1;
    }

    size_t nWidth = 0, nHeight = 0;
    size_t nSize = NPngProc::readPngFileGray(argv[1], nullptr, &nWidth, &nHeight);
    if (nSize == NPngProc::PNG_ERROR) {
        printf("Error reading: %s\n", argv[1]);
        return -1;
    }

    unsigned char* pIn = new unsigned char[nSize];
    unsigned char* pOut = new unsigned char[nSize];
    unsigned char* pTmp = new unsigned char[nSize];
    NPngProc::readPngFileGray(argv[1], pIn, &nWidth, &nHeight);
    size_t total = nWidth * nHeight;
    printf("Image: %s (%zux%zu)\n\n", argv[1], nWidth, nHeight);

    // --- Гауссов шум ---
    addGaussianNoise(pIn, pTmp, total, 20.0);
    NPngProc::writePngFile("gauss_noise.png", pTmp, nWidth, nHeight, 8);

    unsigned char* pMean = new unsigned char[nSize];
    unsigned char* pMedian = new unsigned char[nSize];
    unsigned char* pTrimmed = new unsigned char[nSize];

    meanFilter(pTmp, pMean, nWidth, nHeight, 5);
    medianFilter(pTmp, pMedian, nWidth, nHeight, 5, 5);
    trimmedMeanFilter(pTmp, pTrimmed, nWidth, nHeight, 5, 5, 3);

    NPngProc::writePngFile("gauss_mean.png", pMean, nWidth, nHeight, 8);
    NPngProc::writePngFile("gauss_median.png", pMedian, nWidth, nHeight, 8);
    NPngProc::writePngFile("gauss_trimmed.png", pTrimmed, nWidth, nHeight, 8);

    printf("=== Gaussian noise (sigma=20) ===\n");
    printf("Mean filter    PSNR: %.2f dB\n", computePSNR(pIn, pMean, total));
    printf("Median filter  PSNR: %.2f dB\n", computePSNR(pIn, pMedian, total));
    printf("Trimmed mean   PSNR: %.2f dB\n", computePSNR(pIn, pTrimmed, total));

    // --- Импульсный шум ---
    addImpulseNoise(pIn, pTmp, total, 0.05);
    NPngProc::writePngFile("impulse_noise.png", pTmp, nWidth, nHeight, 8);

    meanFilter(pTmp, pMean, nWidth, nHeight, 5);
    medianFilter(pTmp, pMedian, nWidth, nHeight, 5, 5);
    trimmedMeanFilter(pTmp, pTrimmed, nWidth, nHeight, 5, 5, 3);

    NPngProc::writePngFile("impulse_mean.png", pMean, nWidth, nHeight, 8);
    NPngProc::writePngFile("impulse_median.png", pMedian, nWidth, nHeight, 8);
    NPngProc::writePngFile("impulse_trimmed.png", pTrimmed, nWidth, nHeight, 8);

    printf("\n=== Impulse noise (density=5%%) ===\n");
    printf("Mean filter    PSNR: %.2f dB\n", computePSNR(pIn, pMean, total));
    printf("Median filter  PSNR: %.2f dB\n", computePSNR(pIn, pMedian, total));
    printf("Trimmed mean   PSNR: %.2f dB\n", computePSNR(pIn, pTrimmed, total));

    // --- Морфологические операции ---
    printf("\n=== Morphological operations ===\n");
    morphErode(pIn, pOut, nWidth, nHeight, 5);
    NPngProc::writePngFile("morph_erode.png", pOut, nWidth, nHeight, 8);
    printf("Erosion:  morph_erode.png\n");

    morphDilate(pIn, pOut, nWidth, nHeight, 5);
    NPngProc::writePngFile("morph_dilate.png", pOut, nWidth, nHeight, 8);
    printf("Dilation: morph_dilate.png\n");

    morphOpen(pIn, pOut, nWidth, nHeight, 5);
    NPngProc::writePngFile("morph_open.png", pOut, nWidth, nHeight, 8);
    printf("Opening:  morph_open.png\n");

    morphClose(pIn, pOut, nWidth, nHeight, 5);
    NPngProc::writePngFile("morph_close.png", pOut, nWidth, nHeight, 8);
    printf("Closing:  morph_close.png\n");

    delete[] pIn; delete[] pOut; delete[] pTmp;
    delete[] pMean; delete[] pMedian; delete[] pTrimmed;
    printf("\nDone!\n");
    return 0;
}