#include <cstdio>
#include <vector>
#include "lab6.h"
#include "PngProc.h"

int main(int argc, char* argv[]) {
    const char* fname = (argc > 1) ? argv[1] : "kotik.png";

    // Загрузка через NPngProc
    size_t nWidth = 0, nHeight = 0;
    size_t nSize = NPngProc::readPngFileGray(fname, nullptr, &nWidth, &nHeight);
    if (nSize == NPngProc::PNG_ERROR) {
        printf("Cannot load %s\n", fname);
        return 1;
    }
    uint8_t* img = new uint8_t[nSize];
    NPngProc::readPngFileGray(fname, img, &nWidth, &nHeight);

    int width  = (int)nWidth;
    int height = (int)nHeight;
    printf("Image: %s (%dx%d)\n\n", fname, width, height);

    // 1. Порог Оцу
    int thresh = otsuThreshold(img, width, height);
    printf("Otsu threshold: %d\n\n", thresh);

    // 2. Бинаризация
    std::vector<uint8_t> binary(width * height);
    binarize(img, binary.data(), width, height, thresh);
    NPngProc::writePngFile("out_binary.png", binary.data(), nWidth, nHeight, 8);
    printf("Saved: out_binary.png\n");

    // 3. Разметка связных компонент
    std::vector<int> labels(width * height);
    int numLabels = labelConnectedComponents(binary.data(), labels.data(), width, height);
    printf("Connected components: %d\n\n", numLabels);

    saveLabeledImage(labels.data(), width, height, numLabels, "out_labeled.png");
    printf("Saved: out_labeled.png\n\n");

    // 4. Геометрические моменты
    auto regions = computeRegionProperties(labels.data(), width, height, numLabels);

    printf("Regions with area > 30:\n");
    printf("%-6s %-8s %-8s %-8s %-8s %-8s %-8s %-8s\n",
        "Label", "Area", "cx", "cy", "mu20", "mu02", "mu11", "Circ.");

    int bigCount = 0;
    for (int i = 1; i <= numLabels; i++) {
        if (regions[i].area > 30) {
            bigCount++;
            printf("%-6d %-8d %-8.1f %-8.1f %-8.2f %-8.2f %-8.2f %-8.3f\n",
                regions[i].label, regions[i].area,
                regions[i].cx, regions[i].cy,
                regions[i].mu20, regions[i].mu02, regions[i].mu11,
                regions[i].circularity);
        }
    }
    printf("\nTotal regions with area > 30: %d\n", bigCount);

    // 5. Подсчёт круглых областей
    double circThresh = 0.7;
    int circles = countCircularRegions(regions, 30, circThresh);
    printf("Circle-like regions (area > 30, circularity >= %.1f): %d\n",
        circThresh, circles);

    delete[] img;
    return 0;
}