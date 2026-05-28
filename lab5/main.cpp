#include <stdio.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <cmath>
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

    // === Timing benchmark: sort vs Huang ===
    // Используем кроп 512x512 чтобы бенчмарк завершился быстро
    const size_t benchW = (nWidth  > 512) ? 512 : nWidth;
    const size_t benchH = (nHeight > 512) ? 512 : nHeight;
    std::vector<unsigned char> benchIn(benchW * benchH);
    std::vector<unsigned char> benchOut(benchW * benchH);
    for (size_t y = 0; y < benchH; ++y)
        for (size_t x = 0; x < benchW; ++x)
            benchIn[y * benchW + x] = pIn[y * nWidth + x];

    printf("\n=== Median filter: sort vs Huang algorithm (crop %zux%zu) ===\n", benchW, benchH);
    printf("  Size |  Sort (ms) |  Huang (ms) |  Speedup\n");
    printf("  -----|------------|-------------|----------\n");

    const int numSizes = 10;
    const int aperSizes[] = { 3, 5, 7, 9, 11, 13, 15, 17, 19, 21 };
    double timeSort[numSizes], timeHuang[numSizes];

    for (int si = 0; si < numSizes; ++si) {
        int ks = aperSizes[si];
        clock_t t0, t1;

        t0 = clock();
        medianFilterSort(benchIn.data(), benchOut.data(), benchW, benchH, ks, ks);
        t1 = clock();
        timeSort[si] = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;

        t0 = clock();
        medianFilterHuang(benchIn.data(), benchOut.data(), benchW, benchH, ks, ks);
        t1 = clock();
        timeHuang[si] = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;

        double speedup = (timeHuang[si] > 0.001) ? timeSort[si] / timeHuang[si] : 0.0;
        printf("  %2dx%-2d |   %8.1f |    %8.1f |  %.2fx\n",
            ks, ks, timeSort[si], timeHuang[si], speedup);
    }

    // === Timing chart PNG ===
    // Two bars per aperture size: Sort (gray=160) and Huang (white=255)
    int chartW  = 560;  // 10 groups * (26+30) px + margins
    int chartH  = 280;
    int marginL = 50, marginB = 40, marginT = 20, marginR = 20;
    int plotW   = chartW - marginL - marginR;
    int plotH   = chartH - marginT - marginB;
    int groupW  = plotW / numSizes;   // pixels per group
    int barW    = groupW / 2 - 2;    // bar width

    double maxTime = 0;
    for (int i = 0; i < numSizes; ++i)
        maxTime = std::max(maxTime, std::max(timeSort[i], timeHuang[i]));
    if (maxTime < 1.0) maxTime = 1.0;

    std::vector<unsigned char> chart(chartW * chartH, 30);  // dark background

    // Grid lines
    for (int gi = 1; gi <= 5; ++gi) {
        int gy = marginT + plotH - (int)(plotH * gi / 5.0);
        for (int x = marginL; x < marginL + plotW; ++x)
            chart[gy * chartW + x] = 70;
    }

    // Bars
    for (int si = 0; si < numSizes; ++si) {
        int gx = marginL + si * groupW;

        // Sort bar (gray = 160)
        int hSort = (int)(plotH * timeSort[si] / maxTime + 0.5);
        for (int y = marginT + plotH - hSort; y < marginT + plotH; ++y)
            for (int x = gx + 1; x < gx + 1 + barW; ++x)
                if (x >= 0 && x < chartW && y >= 0 && y < chartH)
                    chart[y * chartW + x] = 160;

        // Huang bar (white = 240)
        int hHuang = (int)(plotH * timeHuang[si] / maxTime + 0.5);
        for (int y = marginT + plotH - hHuang; y < marginT + plotH; ++y)
            for (int x = gx + barW + 3; x < gx + 2 * barW + 3; ++x)
                if (x >= 0 && x < chartW && y >= 0 && y < chartH)
                    chart[y * chartW + x] = 240;
    }

    // Baseline
    for (int x = marginL; x < marginL + plotW; ++x)
        chart[(marginT + plotH) * chartW + x] = 200;
    // Left axis
    for (int y = marginT; y <= marginT + plotH; ++y)
        chart[y * chartW + marginL] = 200;

    NPngProc::writePngFile("timing_chart.png", chart.data(), chartW, chartH, 8);
    printf("\nTiming chart saved: timing_chart.png\n");
    printf("Legend: gray bar = sort-based,  white bar = Huang algorithm\n\n");

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