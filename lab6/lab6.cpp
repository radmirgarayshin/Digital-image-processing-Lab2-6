#include "lab6.h"
#include "PngProc.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ================================================================
//  Порог Оцу
// ================================================================

int otsuThreshold(const uint8_t* img, int width, int height) {
    int N = width * height;
    int hist[256] = {};
    for (int i = 0; i < N; i++) hist[img[i]]++;

    double total = (double)N;
    double sum = 0;
    for (int i = 0; i < 256; i++) sum += i * hist[i];

    double sumB = 0, wB = 0, maxVar = 0;
    int threshold = 0;

    for (int t = 0; t < 256; t++) {
        wB += hist[t];
        if (wB == 0) continue;
        double wF = total - wB;
        if (wF == 0) break;

        sumB += t * hist[t];
        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;

        double var = wB * wF * (mB - mF) * (mB - mF);
        if (var > maxVar) {
            maxVar = var;
            threshold = t;
        }
    }
    return threshold;
}

// ================================================================
//  Бинаризация
// ================================================================

void binarize(const uint8_t* img, uint8_t* out, int width, int height, int threshold) {
    int N = width * height;
    for (int i = 0; i < N; i++)
        out[i] = (img[i] > threshold) ? 255 : 0;
}

// ================================================================
//  Union-Find для двухпроходной разметки
// ================================================================

struct UF {
    std::vector<int> parent;
    void init(int n) {
        parent.resize(n);
        for (int i = 0; i < n; i++) parent[i] = i;
    }
    int find(int x) {
        return parent[x] == x ? x : parent[x] = find(parent[x]);
    }
    void unite(int a, int b) {
        parent[find(a)] = find(b);
    }
};

// ================================================================
//  Разметка 4-связных компонент (два прохода + Union-Find)
// ================================================================

int labelConnectedComponents(const uint8_t* binary, int* labels, int width, int height) {
    int N = width * height;
    std::vector<int> tmp(N, 0);
    UF uf;
    uf.init(N + 1);
    int nextLabel = 1;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            if (binary[idx] == 0) { tmp[idx] = 0; continue; }

            int left  = (x > 0) ? tmp[y * width + (x - 1)] : 0;
            int above = (y > 0) ? tmp[(y - 1) * width + x] : 0;

            if (left == 0 && above == 0) {
                tmp[idx] = nextLabel++;
            } else if (left != 0 && above == 0) {
                tmp[idx] = left;
            } else if (left == 0 && above != 0) {
                tmp[idx] = above;
            } else {
                tmp[idx] = left;
                uf.unite(left, above);
            }
        }
    }

    std::vector<int> remap(nextLabel, -1);
    int numLabels = 0;
    for (int i = 0; i < N; i++) {
        if (tmp[i] == 0) { labels[i] = 0; continue; }
        int root = uf.find(tmp[i]);
        if (remap[root] == -1) remap[root] = ++numLabels;
        labels[i] = remap[root];
    }
    return numLabels;
}

// ================================================================
//  Геометрические признаки: моменты + периметр + circEig + circIso
// ================================================================

std::vector<RegionInfo> computeRegionProperties(
    const int* labels, int width, int height, int numLabels)
{
    std::vector<RegionInfo> regions(numLabels + 1);
    for (int i = 1; i <= numLabels; i++) {
        regions[i] = RegionInfo{};
        regions[i].label = i;
    }

    std::vector<double> sumX(numLabels + 1, 0), sumY(numLabels + 1, 0);

    // Моменты 0-го и 1-го порядков
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int lbl = labels[y * width + x];
            if (lbl == 0) continue;
            regions[lbl].area++;
            sumX[lbl] += x;
            sumY[lbl] += y;
        }
    }
    for (int i = 1; i <= numLabels; i++) {
        if (regions[i].area == 0) continue;
        regions[i].cx = sumX[i] / regions[i].area;
        regions[i].cy = sumY[i] / regions[i].area;
    }

    // Центральные моменты 2-го порядка + периметр (4-связность)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int lbl = labels[y * width + x];
            if (lbl == 0) continue;

            double dx = x - regions[lbl].cx;
            double dy = y - regions[lbl].cy;
            regions[lbl].mu20 += dx * dx;
            regions[lbl].mu02 += dy * dy;
            regions[lbl].mu11 += dx * dy;

            // Граничный пиксель = есть хотя бы один 4-сосед с другой меткой
            int nx[] = { x - 1, x + 1, x,     x };
            int ny[] = { y,     y,     y - 1, y + 1 };
            for (int k = 0; k < 4; ++k) {
                int bx = nx[k], by = ny[k];
                if (bx < 0 || bx >= width || by < 0 || by >= height
                    || labels[by * width + bx] != lbl) {
                    regions[lbl].perimeter++;
                    break;
                }
            }
        }
    }

    // Нормализация и коэффициенты округлости
    for (int i = 1; i <= numLabels; i++) {
        if (regions[i].area == 0) continue;
        double a = (double)regions[i].area;
        regions[i].mu20 /= a;
        regions[i].mu02 /= a;
        regions[i].mu11 /= a;

        // circEig = lam_min / lam_max
        double trace = regions[i].mu20 + regions[i].mu02;
        double det   = regions[i].mu20 * regions[i].mu02
                     - regions[i].mu11 * regions[i].mu11;
        double disc  = sqrt(std::max(0.0, trace * trace / 4.0 - det));
        double lam1  = trace / 2.0 + disc;
        double lam2  = trace / 2.0 - disc;
        regions[i].circEig = (lam1 < 1e-10) ? 1.0 : lam2 / lam1;

        // circIso = 4*pi*Area / Perimeter^2
        int P = regions[i].perimeter;
        regions[i].circIso = (P > 0) ? (4.0 * M_PI * a / ((double)P * P)) : 0.0;
    }

    return regions;
}

// ================================================================
//  Подсчёт округлых областей (по circIso)
// ================================================================

int countCircularRegions(const std::vector<RegionInfo>& regions,
    int minArea, double circMin, double circMax)
{
    int count = 0;
    for (size_t i = 1; i < regions.size(); i++) {
        if (regions[i].area > minArea
            && regions[i].circIso >= circMin
            && regions[i].circIso <= circMax)
            count++;
    }
    return count;
}

// ================================================================
//  Сохранение всех областей (разные серые уровни)
// ================================================================

void saveLabeledImage(const int* labels, int width, int height,
    int numLabels, const char* filename)
{
    std::vector<uint8_t> out(width * height, 0);
    for (int i = 0; i < width * height; i++) {
        if (labels[i] == 0) continue;
        out[i] = (uint8_t)(30 + (labels[i] % 15) * 15);
    }
    NPngProc::writePngFile(filename, out.data(), (size_t)width, (size_t)height, 8);
}

// ================================================================
//  Сохранение только округлых областей (остальное зачернить)
//  Использует circIso >= circThreshold
// ================================================================

void saveCircularOnly(const int* labels, const std::vector<RegionInfo>& regions,
    int width, int height,
    int minArea, double circMin, double circMax, const char* filename)
{
    std::vector<uint8_t> out(width * height, 0);
    for (int i = 0; i < width * height; i++) {
        int lbl = labels[i];
        if (lbl <= 0) continue;
        const RegionInfo& r = regions[lbl];
        if (r.area > minArea && r.circIso >= circMin && r.circIso <= circMax)
            out[i] = 255;
    }
    NPngProc::writePngFile(filename, out.data(), (size_t)width, (size_t)height, 8);
}

// ================================================================
//  Генерация тестового изображения: круги, квадраты, ромбы
//  Фон — чёрный (0), фигуры — белые (255)
// ================================================================

void generateTestImage(uint8_t* img, int width, int height)
{
    memset(img, 0, width * height);

    // Вспомогательные лямбды
    auto setPixel = [&](int x, int y) {
        if (x >= 0 && x < width && y >= 0 && y < height)
            img[y * width + x] = 255;
    };

    // --- 3 круга ---
    struct { int cx, cy, r; } circles[] = {
        { 50,  50, 30 },
        { 150, 60, 22 },
        { 80,  160, 38 }
    };
    for (auto& c : circles) {
        for (int y = c.cy - c.r - 1; y <= c.cy + c.r + 1; ++y)
            for (int x = c.cx - c.r - 1; x <= c.cx + c.r + 1; ++x)
                if ((x - c.cx) * (x - c.cx) + (y - c.cy) * (y - c.cy) <= c.r * c.r)
                    setPixel(x, y);
    }

    // --- 2 квадрата (оси выровнены) ---
    struct { int x0, y0, side; } squares[] = {
        { 180, 30,  50 },
        { 190, 140, 35 }
    };
    for (auto& s : squares) {
        for (int y = s.y0; y < s.y0 + s.side; ++y)
            for (int x = s.x0; x < s.x0 + s.side; ++x)
                setPixel(x, y);
    }

    // --- 2 ромба (квадраты, повёрнутые на 45°) ---
    struct { int cx, cy, d; } diamonds[] = {
        { 50,  230, 28 },  // d = полуось
        { 155, 230, 20 }
    };
    for (auto& d : diamonds) {
        for (int y = d.cy - d.d; y <= d.cy + d.d; ++y)
            for (int x = d.cx - d.d; x <= d.cx + d.d; ++x)
                if (abs(x - d.cx) + abs(y - d.cy) <= d.d)
                    setPixel(x, y);
    }
}
