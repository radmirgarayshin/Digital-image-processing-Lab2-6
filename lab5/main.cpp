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

    printf("\n=== Median filter: Fast3x3 / Sort(nth_elem) / Huang (crop %zux%zu) ===\n", benchW, benchH);
    printf("  Size | Fast3x3(ms)| Sort (ms)  | Huang (ms) | Sort/Huang\n");
    printf("  -----|------------|------------|------------|----------\n");

    const int numSizes = 10;
    const int aperSizes[] = { 3, 5, 7, 9, 11, 13, 15, 17, 19, 21 };
    double timeFast[numSizes], timeSort[numSizes], timeHuang[numSizes];

    for (int si = 0; si < numSizes; ++si) {
        int ks = aperSizes[si];
        clock_t t0, t1;

        // Fast 3x3 (только для ks=3)
        if (ks == 3) {
            t0 = clock();
            medianFilter3x3Fast(benchIn.data(), benchOut.data(), benchW, benchH);
            t1 = clock();
            timeFast[si] = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;
        } else {
            timeFast[si] = -1.0;
        }

        t0 = clock();
        medianFilterSort(benchIn.data(), benchOut.data(), benchW, benchH, ks, ks);
        t1 = clock();
        timeSort[si] = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;

        t0 = clock();
        medianFilterHuang(benchIn.data(), benchOut.data(), benchW, benchH, ks, ks);
        t1 = clock();
        timeHuang[si] = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;

        double speedup = (timeHuang[si] > 0.001) ? timeSort[si] / timeHuang[si] : 0.0;
        if (timeFast[si] >= 0)
            printf("  %2dx%-2d |   %8.1f |   %8.1f |   %8.1f |  %.2fx\n",
                ks, ks, timeFast[si], timeSort[si], timeHuang[si], speedup);
        else
            printf("  %2dx%-2d |          - |   %8.1f |   %8.1f |  %.2fx\n",
                ks, ks, timeSort[si], timeHuang[si], speedup);
    }

    // === Timing line chart PNG ===
    // Три ломаные линии: Sort (серый=160), Huang (белый=240), Fast3x3 (точка, светлый=200)
    int chartW  = 560;
    int chartH  = 280;
    int marginL = 50, marginB = 40, marginT = 20, marginR = 20;
    int plotW   = chartW - marginL - marginR;
    int plotH   = chartH - marginT - marginB;

    double maxTime = 0;
    for (int i = 0; i < numSizes; ++i)
        maxTime = std::max(maxTime, std::max(timeSort[i], timeHuang[i]));
    if (maxTime < 1.0) maxTime = 1.0;

    std::vector<unsigned char> chart(chartW * chartH, 30);  // тёмный фон

    // Горизонтальные линии сетки
    for (int gi = 1; gi <= 5; ++gi) {
        int gy = marginT + plotH - (int)(plotH * gi / 5.0);
        for (int x = marginL; x < marginL + plotW; ++x)
            chart[gy * chartW + x] = 60;
    }

    // Вспомогательная лямбда: пиксель точки на графике
    auto plotPx = [&](int si, double t, unsigned char col, int radius) {
        int px = marginL + si * plotW / (numSizes - 1);
        int py = marginT + plotH - (int)(plotH * t / maxTime + 0.5);
        for (int dy = -radius; dy <= radius; ++dy)
            for (int dx = -radius; dx <= radius; ++dx) {
                int nx = px + dx, ny = py + dy;
                if (nx >= 0 && nx < chartW && ny >= 0 && ny < chartH)
                    chart[ny * chartW + nx] = col;
            }
    };

    // Вспомогательная лямбда: линия между двумя точками (алгоритм Брезенхема)
    auto drawLine = [&](int x0, int y0, int x1, int y1, unsigned char col) {
        int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;
        while (true) {
            if (x0 >= 0 && x0 < chartW && y0 >= 0 && y0 < chartH)
                chart[y0 * chartW + x0] = col;
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 <  dx) { err += dx; y0 += sy; }
        }
    };

    // Ломаные линии для Sort и Huang
    for (int si = 0; si < numSizes - 1; ++si) {
        int x0 = marginL + si * plotW / (numSizes - 1);
        int x1 = marginL + (si + 1) * plotW / (numSizes - 1);
        int ySort0 = marginT + plotH - (int)(plotH * timeSort[si]     / maxTime + 0.5);
        int ySort1 = marginT + plotH - (int)(plotH * timeSort[si + 1] / maxTime + 0.5);
        int yHu0   = marginT + plotH - (int)(plotH * timeHuang[si]     / maxTime + 0.5);
        int yHu1   = marginT + plotH - (int)(plotH * timeHuang[si + 1] / maxTime + 0.5);
        drawLine(x0, ySort0, x1, ySort1, 160);  // Sort — серый
        drawLine(x0, yHu0,   x1, yHu1,   240);  // Huang — белый
    }

    // Точки на линиях + маркер Fast3x3 для ks=3
    for (int si = 0; si < numSizes; ++si) {
        plotPx(si, timeSort[si],  160, 2);
        plotPx(si, timeHuang[si], 240, 2);
        if (timeFast[si] >= 0)
            plotPx(si, timeFast[si], 200, 3);  // Fast3x3 — звёздочка
    }

    // Оси
    for (int x = marginL; x < marginL + plotW; ++x)
        chart[(marginT + plotH) * chartW + x] = 200;
    for (int y = marginT; y <= marginT + plotH; ++y)
        chart[y * chartW + marginL] = 200;

    NPngProc::writePngFile("timing_chart.png", chart.data(), chartW, chartH, 8);
    printf("\nTiming chart saved: timing_chart.png\n");
    printf("Legend: gray line = Sort(nth_elem), white line = Huang, circle at 3x3 = Fast3x3\n\n");

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