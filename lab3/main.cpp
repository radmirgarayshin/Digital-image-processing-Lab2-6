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

    // Ближайший сосед
    clock_t t1 = clock();
    rotateImage(pIn, nWidth, nHeight, pOut, outW, outH, angle, NEAREST_NEIGHBOR);
    clock_t t2 = clock();
    NPngProc::writePngFile("out_nearest.png", pOut, outW, outH, 8);
    printf("Nearest neighbor: %.3f sec\n", (double)(t2 - t1) / CLOCKS_PER_SEC);

    // Билинейная
    t1 = clock();
    rotateImage(pIn, nWidth, nHeight, pOut, outW, outH, angle, BILINEAR);
    t2 = clock();
    NPngProc::writePngFile("out_bilinear.png", pOut, outW, outH, 8);
    printf("Bilinear:         %.3f sec\n", (double)(t2 - t1) / CLOCKS_PER_SEC);

    // Бикубическая
    t1 = clock();
    rotateImage(pIn, nWidth, nHeight, pOut, outW, outH, angle, BICUBIC);
    t2 = clock();
    NPngProc::writePngFile("out_bicubic.png", pOut, outW, outH, 8);
    printf("Bicubic:          %.3f sec\n", (double)(t2 - t1) / CLOCKS_PER_SEC);

    delete[] pIn;
    delete[] pOut;
    return 0;
}