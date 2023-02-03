#pragma once
#include "binaryexport.h"
#include "reader.h"
#include "writer.h"
#include "bassert.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

namespace shilm {
namespace image {

class SHILM_IO_EXPORT Color {
public:
    Color(char* location, unsigned char size, unsigned char offset):
        loc(location), size(size), off(std::max(8 - offset - size, 0)) {
            assert(offset + size <= 8 * sizeof(int));
            assert(offset < 8);
            assert(size < 8 || off == 0);
        }
    operator unsigned int() const {
        unsigned int val = *((unsigned int*) loc);
        if (size < 8)
            val >>= off;
        if (size < sizeof(unsigned int) * 8)
            val %= (1 << size);
        return val;
    }
    void operator=(int val) {
        int& des = *((int*) loc);
        if (size == sizeof(int) * 8) { // << 32 isn't defined so this check is nessesary
            des = val;
            return;
        }
        val %= (1 << size);
        val <<= off;
        unsigned int mask = ~(~((0xffffffffu) << size) << off);
        //std::cout << std::hex << std::setfill('0') << std::setw(8) << des << std::endl;
        //std::cout << std::hex << std::setfill('0') << std::setw(8) << val << std::endl;
        //std::cout << std::hex << std::setfill('0') << std::setw(8) << mask << std::endl;

        des = (des & mask) | val;
        //std::cout << std::hex << std::setfill('0') << std::setw(8) << des << std::endl;
    }
    void operator=(const Color& c) {
        operator=((int) c);
    }
private:
    char* loc;
    unsigned char size;
    unsigned char off;
};

class SHILM_IO_EXPORT Bitmap {
public:
    /**
    Create a bitmap with given width, height and pixeldepth from a char array
    */
    Bitmap(int width, int height, int bpp, char* data) :
        width(width), height(height), bitsPerPixel(bpp),
        data(data), ownsData(false) {
        assert(bpp == 1 || bpp == 2 || bpp == 4 || bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32);
        rowbytesize = (width * bitsPerPixel + 7) / 8;//((bitsPerPixel * width + 3) / 32) * 4;
    }
    /**
    Reads in a bitmapimage from <code>reader</code> using this format:
    https://en.wikipedia.org/wiki/BMP_file_format#DIB_header_.28bitmap_information_header.29
    */
    Bitmap(io::BinaryReader& reader, bool useColorTable = true);
    /**
    Reads in raw bitmap data from <code>reader</code>. The image has a given width and height. Each
    row is aligned per <code>align<bytes>. The settings of reader determine the endianess of the image
    */
    Bitmap(int width, int height, int bpp, int align, io::BinaryReader& reader);
    /**
    Reads a bitmap from a .bmp file
    */
    Bitmap(const std::string& filename, bool useColorTable = true);
    virtual ~Bitmap();
    Bitmap(const Bitmap& other);
    Bitmap(Bitmap&& other);
    void operator=(const Bitmap& other);
    void operator=(Bitmap&& other);
    Bitmap flip(bool horizontal, bool vertical);
    Color getPixel(int x, int y);
    const Color getPixel(int x, int y) const;
    class Proxy {
    public:
        Proxy(Bitmap& bmp, int y) : bmp(bmp), y(y){}
        Bitmap& bmp;
        int y;
    public:
        Color operator[](int x) {
            return bmp.getPixel(x, y);
        }
        const Color operator[](int x) const {
            return const_cast<const Bitmap&>(bmp).getPixel(x, y);
        }
    };
    Proxy operator[](int y) {
        return Proxy(*this, y);
    }
    const Proxy operator[](int y) const {
        return Proxy(const_cast<Bitmap&>(*this), y);
    }
    inline int getWidth() const {
        return width;
    }
    inline int getHeight() const {
        return height;
    }
    inline int getBitsPerPixel() const {
        return bitsPerPixel;
    }
    static Bitmap createFromPNG(const std::string& filename);
    Bitmap ARGBtoRGBA();
    const char* getData() const {
        return data;
    }
    char* getData() {
        return data;
    }
    char* extractData() {
        ownsData = false;
        return data;
    }
private:
    void loadData(int align, io::BinaryReader& reader, int ctbpp, bool flipEndianness);
    int width, height;
    int rowbytesize;
    int bitsPerPixel;
    char* data;
    bool ownsData;
    bool useTable;
    std::vector<unsigned int> colorTable;
    friend SHILM_IO_EXPORT io::BinaryWriter& operator<<(io::BinaryWriter& writer, const Bitmap& bmp);
};

SHILM_IO_EXPORT io::BinaryWriter& operator<<(io::BinaryWriter& writer, const Bitmap& bmp);
}
}
