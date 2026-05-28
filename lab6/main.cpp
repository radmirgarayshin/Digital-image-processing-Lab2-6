#include <cstdio>
#include <vector>
#include <cmath>
#include "lab6.h"
#include "PngProc.h"

// Вспомогательная функция: запустить полный пайплайн на изображении
// circMin/circMax: диапазон circIso для кругов
// Круги (дискретные): ~1.2-1.3; квадраты: ~0.78-0.83; ромбы: ~1.6-1.7
// Верхний порог нужен, т.к. ромб с 4-связным периметром даёт circIso~1.6 > circMin
static void runPipeline(const uint8_t* img, int width, int height,
    const char* tag, int minArea, double circMin, double circMax)
{
    printf("=== Pipeline: %s (%dx%d) ===\n", tag, width, height);

    // 1. Порог Оцу
    int thresh = otsuThreshold(img, width, height);
    printf("  Otsu threshold: %d\n", thresh);

    // 2. Бинаризация
    std::vector<uint8_t> binary(width * height);
    binarize(img, binary.data(), width, height, thresh);

    char fname[256];
    snprintf(fname, sizeof(fname), "out_%s_binary.png", tag);
    NPngProc::writePngFile(fname, binary.data(), width, height, 8);
    printf("  Saved: %s\n", fname);

    // 3. Разметка связных компонент
    std::vector<int> labels(width * height);
    int numLabels = labelConnectedComponents(binary.data(), labels.data(), width, height);
    printf("  Connected components: %d\n", numLabels);

    snprintf(fname, sizeof(fname), "out_%s_labeled.png", tag);
    saveLabeledImage(labels.data(), width, height, numLabels, fname);
    printf("  Saved: %s\n", fname);

    // 4. Геометрические признаки
    auto regions = computeRegionProperties(labels.data(), width, height, numLabels);

    // 5. Таблица признаков
    printf("\n  %-5s %-7s %-7s %-7s %-8s %-8s %-10s %-10s\n",
        "Label", "Area", "cx", "cy", "circEig", "Perim", "circIso", "Shape?");
    for (int i = 1; i <= numLabels; i++) {
        if (regions[i].area <= minArea) continue;
        const char* shape = (regions[i].circIso >= circMin && regions[i].circIso <= circMax) ? "CIRCLE" :
                            (regions[i].circEig >= 0.90) ? "square/diam" : "other";
        printf("  %-5d %-7d %-7.1f %-7.1f %-8.3f %-8d %-10.3f %-10s\n",
            i, regions[i].area,
            regions[i].cx, regions[i].cy,
            regions[i].circEig,
            regions[i].perimeter,
            regions[i].circIso,
            shape);
    }

    // 6. Подсчёт и сохранение только округлых областей
    int circles = countCircularRegions(regions, minArea, circMin, circMax);
    printf("\n  Circular regions (area > %d, circIso in [%.2f, %.2f]): %d\n",
        minArea, circMin, circMax, circles);

    snprintf(fname, sizeof(fname), "out_%s_circles_only.png", tag);
    saveCircularOnly(labels.data(), regions, width, height,
        minArea, circMin, circMax, fname);
    printf("  Saved (circles only): %s\n\n", fname);
}

int main(int argc, char* argv[])
{
    // ===== A: Тестовое изображение из файла =====
    // test_shapes.png: чёрный фон, белые фигуры — 3 круга, 2 квадрата, 2 ромба
    {
        const char* testFile = "test_shapes.png";
        size_t tW = 0, tH = 0;
        size_t tSize = NPngProc::readPngFileGray(testFile, nullptr, &tW, &tH);
        if (tSize == NPngProc::PNG_ERROR) {
            printf("Cannot load %s — skipping test pipeline\n\n", testFile);
        } else {
            std::vector<uint8_t> testImg(tSize);
            NPngProc::readPngFileGray(testFile, testImg.data(), &tW, &tH);
            printf("Test image: %s (%zux%zu)\n\n", testFile, tW, tH);
            // circIso диапазон [0.85, 1.40]: круги ~1.24, квадраты ~0.82, ромбы ~1.63
            runPipeline(testImg.data(), (int)tW, (int)tH, "test", 30, 0.85, 1.40);
        }
    }

    // ===== B: Реальное изображение (аргумент командной строки) =====
    if (argc >= 2) {
        size_t nWidth = 0, nHeight = 0;
        size_t nSize = NPngProc::readPngFileGray(argv[1], nullptr, &nWidth, &nHeight);
        if (nSize == NPngProc::PNG_ERROR) {
            printf("Cannot load %s\n", argv[1]);
        } else {
            std::vector<uint8_t> img(nSize);
            NPngProc::readPngFileGray(argv[1], img.data(), &nWidth, &nHeight);
            runPipeline(img.data(), (int)nWidth, (int)nHeight, "real", 30, 0.85, 1.40);
        }
    } else {
        printf("(No real image provided; run with: lab6 <image.png>)\n");
    }

    printf("Done!\n");
    return 0;
}
