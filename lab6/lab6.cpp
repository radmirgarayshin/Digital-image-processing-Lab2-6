#include "lab6.h"
#include "PngProc.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstring>

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

void binarize(const uint8_t* img, uint8_t* out, int width, int height, int threshold) {
    int N = width * height;
    for (int i = 0; i < N; i++)
        out[i] = (img[i] >= threshold) ? 255 : 0;
}

// Union-Find ��� ������������� ��������
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

int labelConnectedComponents(const uint8_t* binary, int* labels, int width, int height) {
    int N = width * height;
    std::vector<int> tmp(N, 0);
    UF uf;
    uf.init(N + 1);
    int nextLabel = 1;

    // ������ ������
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            if (binary[idx] == 0) { tmp[idx] = 0; continue; }

            int left = (x > 0) ? tmp[y * width + (x - 1)] : 0;
            int above = (y > 0) ? tmp[(y - 1) * width + x] : 0;

            if (left == 0 && above == 0) {
                tmp[idx] = nextLabel++;
            }
            else if (left != 0 && above == 0) {
                tmp[idx] = left;
            }
            else if (left == 0 && above != 0) {
                tmp[idx] = above;
            }
            else {
                tmp[idx] = left;
                uf.unite(left, above);
            }
        }
    }

    // ������ ������: �������������
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

std::vector<RegionInfo> computeRegionProperties(
    const int* labels, int width, int height, int numLabels)
{
    std::vector<RegionInfo> regions(numLabels + 1);
    for (int i = 1; i <= numLabels; i++) {
        regions[i].label = i;
        regions[i].area = 0;
        regions[i].cx = regions[i].cy = 0;
        regions[i].mu20 = regions[i].mu02 = regions[i].mu11 = 0;
        regions[i].circularity = 0;
    }

    std::vector<double> sumX(numLabels + 1, 0), sumY(numLabels + 1, 0);

    // ������� 0-�� � 1-�� �������
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

    // ����������� ������� 2-�� �������
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int lbl = labels[y * width + x];
            if (lbl == 0) continue;
            double dx = x - regions[lbl].cx;
            double dy = y - regions[lbl].cy;
            regions[lbl].mu20 += dx * dx;
            regions[lbl].mu02 += dy * dy;
            regions[lbl].mu11 += dx * dy;
        }
    }

    // ���������� � ���������� ��������� �� ���� ����� ����������� ��������
    for (int i = 1; i <= numLabels; i++) {
        if (regions[i].area == 0) continue;
        double a = (double)regions[i].area;
        regions[i].mu20 /= a;
        regions[i].mu02 /= a;
        regions[i].mu11 /= a;

        // ����������� �������� ������� ����������
        double trace = regions[i].mu20 + regions[i].mu02;
        double det = regions[i].mu20 * regions[i].mu02
            - regions[i].mu11 * regions[i].mu11;
        double disc = sqrt(std::max(0.0, trace * trace / 4.0 - det));
        double lam1 = trace / 2.0 + disc;
        double lam2 = trace / 2.0 - disc;

        if (lam1 < 1e-10)
            regions[i].circularity = 1.0;
        else
            regions[i].circularity = lam2 / lam1; // 1 = ����, 0 = �����
    }

    return regions;
}

int countCircularRegions(const std::vector<RegionInfo>& regions,
    int minArea, double circThreshold)
{
    int count = 0;
    for (size_t i = 1; i < regions.size(); i++) {
        if (regions[i].area > minArea && regions[i].circularity >= circThreshold)
            count++;
    }
    return count;
}

void saveLabeledImage(const int* labels, int width, int height,
    int numLabels, const char* filename)
{
    std::vector<uint8_t> out(width * height);
    for (int i = 0; i < width * height; i++) {
        if (labels[i] == 0)
            out[i] = 0;
        else
            out[i] = (uint8_t)(30 + (labels[i] % 15) * 15);
    }
    NPngProc::writePngFile(filename, out.data(), (size_t)width, (size_t)height, 8);
}