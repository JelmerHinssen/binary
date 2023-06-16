#include "bitmap.h"
#include <iostream>
#include "bassert.h"
#include <fstream>
#include <cstring>
#include <vector>
#include "filereader.h"

using namespace shilm;
using namespace shilm::image;
using namespace shilm::io;
using namespace std;

Color Bitmap::getPixel(int x, int y) {
    return Color(data + y * rowbytesize + (bitsPerPixel * x / 8), (unsigned char) bitsPerPixel, (unsigned char) (x * bitsPerPixel % 8));
}

const Color Bitmap::getPixel(int x, int y) const {
    return Color(data + y * rowbytesize + (bitsPerPixel * x / 8), (unsigned char) bitsPerPixel, (unsigned char) (x * bitsPerPixel % 8));
}

Bitmap::~Bitmap() {
    if (ownsData) {
        delete[] data;
    }
}

Bitmap::Bitmap(int width, int height, int bpp, int align, io::BinaryReader& reader):
    width(width), height(height), bitsPerPixel(bpp), useTable(false) {
    loadData(align, reader, 0, false);
}

Bitmap::Bitmap(io::BinaryReader& reader, bool useColorTable) {
    ownsData = false;
    reader.readShort(); // skip 2 bytes
    reader.readInt();   // skip 4 reserved bytes
    int dataPoint = reader.readInt();
    //cout << dataPoint << endl;
    int headerSize = reader.readInt();
    int compression = 0;
    int imageSize = 0;
    int numColors = 0;
    int ctbpp = 32; // Colortable bpp
    if (headerSize < 12) {
        throw std::runtime_error("Invalid header: " + to_string(headerSize) + " < 12");
    }
    if (headerSize == 12) {
        width = reader.readShort();
        height = reader.readShort();
        reader.readShort(); // skip 2 bytes: color planes
        bitsPerPixel = reader.readShort();
        ctbpp = 24;
    } else if (headerSize >= 54) {
        reader.readInt(); // skip 4 bytes?
        width = reader.readInt();
        height = reader.readInt();
        reader.readShort(); // skip 2 bytes: color planes
        bitsPerPixel = reader.readShort();
        compression = reader.readInt();
        imageSize = reader.readInt();
        reader.readInt(); // skip 4 bytes: horizontal resolution
        reader.readInt(); // skip 4 bytes: vertical resolution
        numColors = reader.readInt(); // 4 bytes: number of colors in palette
        reader.readInt(); // skip 4 bytes: number of important colors
        for (int i = 0; i < headerSize - 54; i++) { // Skip remaining header
            reader.readByte();
        }
    } else {
        throw std::runtime_error("Invalid header");
    }
    int bpp = bitsPerPixel;
    if (!(bpp == 1 || bpp == 2 || bpp == 4 || bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32)) {
        throw std::runtime_error("Invalid format");
    }
    if (compression != 0)
        throw std::runtime_error("Compression not supported");
    rowbytesize = ((bitsPerPixel * width + 3) / 32) * 4;
    if (imageSize == 0) {
        imageSize = rowbytesize * height;
    }
    if (imageSize != rowbytesize * height)
        throw std::runtime_error("Size doesn't match");
    int pos = 14 + headerSize;
    if (dataPoint != pos) {
        // Color table
        if (bitsPerPixel <= 16) {
            if (numColors == 0) {
                numColors = 1 << bitsPerPixel;
            }
            colorTable.reserve(numColors);
            for (int i = 0; i < numColors; i++) {
                colorTable.push_back(reader.readBits(ctbpp));
                if (ctbpp == 24) {
                    colorTable.back() |= 0xff000000;
                }
            }
            pos += 4 * numColors;
        }
    }
    if (dataPoint > pos) {
        throw runtime_error("Invalid datapoint");
    }
    for (int i = 0; i < dataPoint - pos; i++) { // Skip bytes between header and data
        reader.readByte();
    }
    if (!useColorTable) {
        ctbpp = 0;
    }
    loadData(4, reader, ctbpp, false);
}

void Bitmap::loadData(int align, io::BinaryReader& reader, int ctbpp, bool flipEndianness) {
    int bpp = bitsPerPixel;
    useTable = !colorTable.empty() && ctbpp > 0;
    int rowsize = ((bitsPerPixel * width + (align * 8 - 1)) / (align * 8)) * align;
    int extraBits = rowsize * 8 - width * bitsPerPixel;
    if (useTable) {
        bitsPerPixel = ctbpp;
    }
    rowbytesize = (bitsPerPixel * width + 7) / 8;
    data = new char[rowbytesize * height];
    ownsData = true;
	if (useTable) {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				if (useTable) {
					(*this)[i][j] = colorTable[reader.readBits(bpp)];
				} else {
					(*this)[i][j] = reader.readBits(bpp);
				}
			}
			reader.readBits(extraBits);
		}
	} else if (extraBits > 0) {
		for (int i = 0; i < height; i++) {
			reader.read(data + i * rowbytesize, rowbytesize);
		}
		for (int i = 0; i < extraBits / 8; i++) {
			reader.readByte();
		}
	} else {
		reader.read(data, height * rowbytesize);
	}
	if (flipEndianness && bitsPerPixel > 8) {
		char swapbyte;
		int swapwidth = bitsPerPixel / 8;
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < rowbytesize; j += swapwidth) {
				for (int k = 0; k < swapwidth / 2; k++) {
					swapbyte = data[i * rowbytesize + j + k];
					data[i * rowbytesize + j + k] = data[i * rowbytesize + j + swapwidth - k - 1];
					data[i * rowbytesize + j + swapwidth - k - 1] = swapbyte;
				}
			}
		}
	}
}

Bitmap::Bitmap(const Bitmap& other):
    width(other.width), height(other.height),
    bitsPerPixel(other.bitsPerPixel), rowbytesize(other.rowbytesize),
    useTable(other.useTable), colorTable(other.colorTable) {
    if (other.ownsData) { // deep copy
        ownsData = true;
        data = new char[rowbytesize * height];
        std::memcpy(data, other.data, rowbytesize * height);
    } else { // shallow copy
        ownsData = false;
        data = other.data;
    }
}

Bitmap::Bitmap(Bitmap&& other):
    width(other.width), height(other.height),
    bitsPerPixel(other.bitsPerPixel), rowbytesize(other.rowbytesize),
    ownsData(other.ownsData), data(other.data),
    useTable(other.useTable), colorTable(std::move(other.colorTable)) {
    other.ownsData = false; // prevent other from deleting their data
}

void Bitmap::operator=(const Bitmap& other) {
    width = other.width;
    height = other.height;
    bitsPerPixel = other.bitsPerPixel;
    rowbytesize = other.rowbytesize;
    if (ownsData) {
        delete[] data;
    }
    if (other.ownsData) { // deep copy
        ownsData = true;
        data = new char[rowbytesize * height];
        std::memcpy(data, other.data, rowbytesize * height);
    } else { // shallow copy
        ownsData = false;
        data = other.data;
    }
    useTable = other.useTable;
    colorTable = other.colorTable;
}

void Bitmap::operator=(Bitmap&& other) {
    width = other.width;
    height = other.height;
    bitsPerPixel = other.bitsPerPixel;
    rowbytesize = other.rowbytesize;
    if (ownsData) {
        delete[] data;
    }

    useTable = other.useTable;
    colorTable = std::move(other.colorTable);
    ownsData = other.ownsData;
    other.ownsData = false; // prevent other from deleting their data
}


Bitmap Bitmap::loadBitmap([[maybe_unused]] const std::string& filename, [[maybe_unused]] bool useColorTable) {
    ifstream in(filename, ios::binary);
    const int ENDIAN = BinaryReader::LITTLE_ENDIAN;
    const int BITORDER = BinaryReader::MOST_SIGNIFICANT_BIT_FIRST;
    BinaryStreamReader bin(in, ENDIAN, BITORDER);
    return Bitmap(bin, useColorTable);
}

io::BinaryWriter& shilm::image::operator<<(io::BinaryWriter& writer, const Bitmap& bmp) {
    int rowsize = ((bmp.bitsPerPixel * bmp.width + 31) / 32) * 4;
    int numColors = (bmp.colorTable.size() == 0 && bmp.bitsPerPixel <= 16) ? 2 : (int)bmp.colorTable.size();
    int colorTableSize = numColors * 4; // always use 32 bpp colortable
    int imageSize = bmp.height * rowsize;
    int dibHeaderSize = 40;
    int bmpHeaderSize = 14;

    // BMP header
    writer.writeByte(0x42); // 'B'
    writer.writeByte(0x4D); // 'M'
    writer.writeInt(colorTableSize + imageSize + dibHeaderSize + bmpHeaderSize); // Size of file
    writer.writeInt(0); // reserved
    writer.writeInt(dibHeaderSize + bmpHeaderSize + colorTableSize); // Data starting point

    // DIB header
    writer.writeInt(dibHeaderSize); // DIB header size
    writer.writeInt(bmp.width);     // Image width
    writer.writeInt(bmp.height);    // Image height
    writer.writeShort(1);           // Color planes
    writer.writeShort((uint16_t) bmp.bitsPerPixel);    // Bits per pixel
    writer.writeInt(0);             // Compression
    writer.writeInt(imageSize);     // Image size
    writer.writeInt(0);             // Horizontal resolution
    writer.writeInt(0);             // Vertical resolution
    writer.writeInt(numColors);             // Colors in palette
    writer.writeInt(0);             // Important colors

    // Color table
    if (bmp.colorTable.size() == 0 && bmp.bitsPerPixel <= 16) {
        writer.writeInt(0xff000000);
        writer.writeInt(0xffffffff);
    } else {
        for (int i = 0; i < numColors; i++) {
            writer.writeInt(bmp.colorTable[i]);
        }
    }

    // Data
	// Writing the entire array at once is MUCH more efficient, but isn't portable because of endianness.
	writer.write((char*)bmp.data, bmp.height * bmp.rowbytesize); 
    // int extraBits = rowsize * 8 - bmp.width * bmp.bitsPerPixel;
    /*for (int i = 0; i < bmp.height; i++) {
        for (int j = 0; j < bmp.width; j++) {
            writer.writeBits(bmp[i][j], bmp.bitsPerPixel);
        }
        writer.writeBits(0, extraBits);
    }*/
    writer.flush();
	return writer;
}

Bitmap Bitmap::flip(bool horizontal, bool vertical) {
    assert(ownsData);
    Bitmap bmp(*this);
    for (int i = 0; i < height; i++) {
        int y = vertical ? height - i - 1 : i;
        for (int j = 0; j < width; j++) {
            int x = horizontal ? width - j - 1 : j;
            bmp[y][x] = (*this)[i][j];
        }
    }
    return bmp;
}

Bitmap Bitmap::ARGBtoRGBA() {
    assert(bitsPerPixel == 32);
    Bitmap bmp(*this);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            uint32_t val = (*this)[i][j];
            bmp[i][j] = (val << 24) + (val >> 8);
        }
    }
    return bmp;
}

/*int main() {
    try {
        ifstream in("test.bmp", ios::binary);
        BinaryStreamReader str(in, BinaryReader::LITTLE_ENDIAN, BinaryReader::LEAST_SIGNIFICANT_BIT_FIRST_FLIPPED);
        Bitmap bmp(str, false);
        ofstream out("testout.bmp", ios::binary);
        BinaryStreamWriter outstr(out, BinaryWriter::LITTLE_ENDIAN, BinaryWriter::LEAST_SIGNIFICANT_BIT_FIRST_FLIPPED);
        outstr << bmp;
        out.close();
        cout << "Width: " << bmp.getWidth() << endl;
        cout << "Height: " << bmp.getHeight() << endl;
        cout << "BitsPerPixel: " << bmp.getBitsPerPixel() << endl;
        for (int i = 0; i < 20; i++)
            cout << hex << bmp[i / 5][i % 5] << dec << endl;
    } catch (const std::exception& e) {
        cout << e.what() << endl;
    }
    return 0;
}*/
