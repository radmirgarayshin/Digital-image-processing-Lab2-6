/*
 * Чтение TIFF-файла без использования внешних библиотек.
 * Поддерживается:
 *   - Порядок байт Intel (II, little-endian) и Motorola (MM, big-endian)
 *   - Несколько strips (StripOffsets / StripByteCounts)
 *   - Только несжатые файлы (Compression = 1)
 *   - 8 бит на пиксель (grayscale)
 */

#define _CRT_SECURE_NO_WARNINGS
#include "tiff_reader.h"
#include <cstdio>
#include <cstring>

// --- Идентификаторы тегов TIFF ---
#define TAG_IMAGE_WIDTH        256
#define TAG_IMAGE_LENGTH       257
#define TAG_BITS_PER_SAMPLE    258
#define TAG_COMPRESSION        259
#define TAG_STRIP_OFFSETS      273
#define TAG_ROWS_PER_STRIP     278
#define TAG_STRIP_BYTE_COUNTS  279

// --- Размеры типов данных TIFF ---
static const int TYPE_SZ[] = { 0,1,1,2,4,8,1,1,2,4,8,4,8 };

static bool g_bigEndian = false; // текущий порядок байт файла

// Чтение 2 байт с учётом порядка байт
static uint16_t rd16(const uint8_t* p) {
    return g_bigEndian
        ? (uint16_t)((p[0] << 8) | p[1])
        : (uint16_t)(p[0] | (p[1] << 8));
}

// Чтение 4 байт с учётом порядка байт
static uint32_t rd32(const uint8_t* p) {
    return g_bigEndian
        ? ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|(uint32_t)p[3]
        : (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

TiffImage loadTiff(const char* filename) {
    TiffImage img;
    img.valid = false;
    img.width = img.height = img.bpp = 0;

    // --- Читаем весь файл в память ---
    FILE* f = fopen(filename, "rb");
    if (!f) { img.error = "Не удалось открыть файл"; return img; }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> data((size_t)fsize);
    fread(data.data(), 1, (size_t)fsize, f);
    fclose(f);

    // --- Определяем порядок байт ---
    if      (data[0]=='I' && data[1]=='I') g_bigEndian = false; // Intel (little-endian)
    else if (data[0]=='M' && data[1]=='M') g_bigEndian = true;  // Motorola (big-endian)
    else { img.error = "Не TIFF (неверный порядок байт)"; return img; }

    // Проверяем магическое число TIFF = 42
    if (rd16(data.data() + 2) != 42) {
        img.error = "Неверное магическое число TIFF";
        return img;
    }

    // --- Читаем IFD (Image File Directory) ---
    uint32_t ifdOff = rd32(data.data() + 4);
    uint16_t nEntries = rd16(data.data() + ifdOff);

    int width = 0, height = 0, bpp = 8, compression = 1;
    std::vector<uint32_t> stripOffsets, stripByteCounts;

    // Разбираем каждую запись IFD (12 байт на запись)
    for (int i = 0; i < nEntries; i++) {
        const uint8_t* e = data.data() + ifdOff + 2 + i * 12;
        uint16_t tag   = rd16(e);
        uint16_t type  = rd16(e + 2);
        uint32_t count = rd32(e + 4);

        // Определяем размер одного элемента данного типа
        int tsz = (type >= 1 && type <= 12) ? TYPE_SZ[type] : 1;
        uint32_t totalBytes = count * (uint32_t)tsz;

        // Если данные помещаются в 4 байта — хранятся inline, иначе — по смещению
        auto getVal = [&](uint32_t idx) -> uint32_t {
            const uint8_t* src;
            uint8_t ibuf[4];
            if (totalBytes <= 4) {
                memcpy(ibuf, e + 8, 4);
                src = ibuf + idx * tsz;
            } else {
                uint32_t ptr = rd32(e + 8);
                src = data.data() + ptr + idx * tsz;
            }
            if (type == 3) return rd16(src);  // SHORT (2 байта)
            if (type == 4) return rd32(src);  // LONG  (4 байта)
            return *src;                       // BYTE  (1 байт)
        };

        switch (tag) {
        case TAG_IMAGE_WIDTH:       width       = (int)getVal(0); break;
        case TAG_IMAGE_LENGTH:      height      = (int)getVal(0); break;
        case TAG_BITS_PER_SAMPLE:   bpp         = (int)getVal(0); break;
        case TAG_COMPRESSION:       compression = (int)getVal(0); break;
        case TAG_STRIP_OFFSETS:
            for (uint32_t j = 0; j < count; j++)
                stripOffsets.push_back(getVal(j));
            break;
        case TAG_STRIP_BYTE_COUNTS:
            for (uint32_t j = 0; j < count; j++)
                stripByteCounts.push_back(getVal(j));
            break;
        }
    }

    // --- Проверки ---
    if (compression != 1) {
        img.error = "Поддерживается только несжатый TIFF (Compression=1)";
        return img;
    }
    if (bpp != 8) {
        img.error = "Поддерживается только 8 бит на пиксель";
        return img;
    }
    if (width <= 0 || height <= 0) {
        img.error = "Некорректные размеры изображения";
        return img;
    }
    if (stripOffsets.empty()) {
        img.error = "Не найдены смещения полос (StripOffsets)";
        return img;
    }

    // --- Считываем пиксели из всех strips ---
    size_t total = (size_t)width * height;
    img.pixels.resize(total);
    size_t dst = 0;
    for (size_t s = 0; s < stripOffsets.size() && dst < total; s++) {
        size_t src = stripOffsets[s];
        size_t len = (s < stripByteCounts.size()) ? stripByteCounts[s] : (total - dst);
        if (dst + len > total) len = total - dst;
        memcpy(img.pixels.data() + dst, data.data() + src, len);
        dst += len;
    }

    img.width  = width;
    img.height = height;
    img.bpp    = bpp;
    img.valid  = true;
    return img;
}
