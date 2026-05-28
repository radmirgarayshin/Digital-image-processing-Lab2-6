#include <stdio.h>
#include <time.h>
#include "PngProc.h"
#include "lab3.h"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: lab3 <input.png> [angle]\n");
        return -1;
    }

    const char* inputFile = argv[1];
    double angle = 45.0;
    if (argc >= 3) angle = atof(argv[2]);

    // Читаем изображение
    size_t nWidth = 0, nHeight = 0;
    size_t nSize = NPngProc::readPngFileGray(inputFile, nullptr, &nWidth, &nHeight);
    if (nSize == NPngProc::PNG_ERROR) {
        printf("Error reading: %s\n", inputFile);
        return -1;
    }
    unsigned char* pIn = new unsigned char[nSize];
    NPngProc::readPngFileGray(inputFile, pIn, &nWidth, &nHeight);
    printf("Image: %s (%zux%zu), angle: %.1f deg\n\n", inputFile, nWidth, nHeight, angle);

    size_t outW, outH;
    getRotatedSize(nWidth, nHeight, angle, outW, outH);
    unsigned char* pOut = new unsigned char[outW * outH];

    // === Benchmark: switch INSIDE loop (old) vs switch OUTSIDE loop (new) ===
    printf("=== Benchmark: switch inside loop vs outside loop (BILINEAR) ===\n");
    clock_t tOld1 = clock();
    rotateImageOld(pIn, nWidth, nHeight, pOut, outW, outH, angle, BILINEAR);
    clock_t tOld2 = clock();
    double timeOld = (double)(tOld2 - tOld1) / CLOCKS_PER_SEC;
    printf("  Switch INSIDE loop  (old): %.4f sec\n", timeOld);

    clock_t tNew1 = clock();
    rotateImage(pIn, nWidth, nHeight, pOut, outW, outH, angle, BILINEAR);
    clock_t tNew2 = clock();
    double timeNew = (double)(tNew2 - tNew1) / CLOCKS_PER_SEC;
    printf("  Switch OUTSIDE loop (new): %.4f sec\n", timeNew);
    if (timeOld > 1e-6 && timeNew > 1e-6)
        printf("  Speedup: %.2fx\n\n", timeOld / timeNew);
    else if (timeOld > 1e-6)
        printf("  (New version is too fast to measure)\n\n");
    else
        printf("\n");

    // === Interpolation methods ===
    printf("=== Interpolation results ===\n");

    // Ближайший сосед
    clock_t t1 = clock();
    rotateImage(pIn, nWidth, nHeight, pOut, outW, outH, angle, NEAREST_NEIGHBOR);
    clock_t t2 = clock();
    NPngProc::writePngFile("out_nearest.png", pOut, outW, outH, 8);
    printf("Nearest neighbor: %.4f sec  -> out_nearest.png\n", (double)(t2 - t1) / CLOCKS_PER_SEC);

    // Билинейная
    t1 = clock();
    rotateImage(pIn, nWidth, nHeight, pOut, outW, outH, angle, BILINEAR);
    t2 = clock();
    NPngProc::writePngFile("out_bilinear.png", pOut, outW, outH, 8);
    printf("Bilinear:         %.4f sec  -> out_bilinear.png\n", (double)(t2 - t1) / CLOCKS_PER_SEC);

    // Бикубическая
    t1 = clock();
    rotateImage(pIn, nWidth, nHeight, pOut, outW, outH, angle, BICUBIC);
    t2 = clock();
    NPngProc::writePngFile("out_bicubic.png", pOut, outW, outH, 8);
    printf("Bicubic:          %.4f sec  -> out_bicubic.png\n", (double)(t2 - t1) / CLOCKS_PER_SEC);

    delete[] pIn;
    delete[] pOut;
    return 0;
}