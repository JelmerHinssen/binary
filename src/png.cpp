#include "bitmap.h"
#include "zlib.h"
#include <fstream>
#include "arrayreader.h"
#include "arraywriter.h"
#include "filereader.h"

using namespace shilm;
using namespace shilm::image;
using namespace shilm::io;
using namespace std;

class PNGStream : public BinaryWriter {
public:
    PNGStream(BinaryWriter& parent, int width, int height, int interlace, int bpp):
        BinaryWriter(parent.getEndianness(), getBitOrder()),
        mStream(parent), mWidth(width), mHeight(height),
        mRowByteSize((width * bpp + 7) / 8),
        mBytespp((bpp + 7) / 8),
        mInterlace(interlace),
        mRow(-1), mCol(mRowByteSize + 8) {
        mRows[0] = new char[mRowByteSize + 8];
        mRows[1] = new char[mRowByteSize + 8];
        memset(mRows[0], 0, mRowByteSize + 8);
        memset(mRows[1], 0, mRowByteSize + 8);
    };
    virtual void write(uint8_t byte) override;
    uint8_t calculate(uint8_t input);
    inline virtual void flushStream() override {
        mStream.flushStream();
    }
    virtual ~PNGStream() {
        delete[] mRows[0];
        delete[] mRows[1];
    }
private:
    BinaryWriter& mStream;
    int mWidth, mHeight;
    int mRowByteSize;
    int mBytespp;
    int mInterlace;
    int mRow;
    int mCol;
    int mFilter = 0;
    int mActiveRow = 0;
    char* mRows[2];
};

static const int NONE = 0, SUB = 1, UP = 2;
static const int AVERAGE = 3, PAETH = 4;

inline uint8_t PNGStream::calculate(uint8_t input) {
    //return input;
    uint8_t a = mRows[mActiveRow][mCol - mBytespp]; // left
    uint8_t b = mRows[1 - mActiveRow][mCol]; // above
    uint8_t c = mRows[1 - mActiveRow][mCol - mBytespp]; // upper left
    switch (mFilter) {
    case NONE:
        return input;
    case SUB:
        return input + a;
    case UP:
        return input + b;
    case AVERAGE:
        return input + (a + b) / 2;
    case PAETH: {
        uint8_t p = a + b - c;
        uint8_t pa = abs(p - a);
        uint8_t pb = abs(p - b);
        uint8_t pc = abs(p - c);
        if (pa <= pb && pa <= pc) return input + a;
        else if (pb <= pc) return input + b;
        else return input + c;
    }
    default:
        return -1;
    }
}

void PNGStream::write(uint8_t byte) {
    if (mCol >= mRowByteSize + 8) {
        mCol = 8;
        mRow++;
        mFilter = byte;
        if (mFilter != 0) {
            mFilter = byte;
        }
        mActiveRow = 1 - mActiveRow;
        return;
    }
    if (mRow == 35 && mCol > 260) {
       mRow = 35;
    }
    mRows[mActiveRow][mCol] = calculate(byte);
    mStream.write(mRows[mActiveRow][mCol]);
    mCol++;
}

Bitmap Bitmap::createFromPNG(const std::string& filename) {
    using namespace std;
    ifstream in(filename, ios::binary);
    const int ENDIAN = BinaryReader::BIG_ENDIAN;
    const int BITORDER = BinaryReader::MOST_SIGNIFICANT_BIT_FIRST;
    BinaryStreamReader bin(in, ENDIAN, BITORDER);
    Bitmap bmp(0, 0, 1, nullptr);
    bool PLTE = false;
    bool IDAT = false;
    bool lastIDAT = false;
    if (bin.readInt() != 0x89504e47) throw runtime_error("Invalid signature 1 in " + filename);
    if (bin.readInt() != 0x0d0a1a0a) throw runtime_error("Invalid signature 2 in " + filename);
    unsigned int length = bin.readInt();
    unsigned int chunkCode = bin.readInt();
    if (chunkCode != 0x49484452) throw runtime_error("First chunk must be IHDR");
    bmp.width = bin.readInt();
    bmp.height = bin.readInt();
    int bpp = bin.readByte();
    int colorType = bin.readByte();
    int compression = bin.readByte();
    int filter = bin.readByte();
    int interlace = bin.readByte();
    unsigned int pos = 13;
    for (unsigned int i = 0; i < length - pos; i++) {
        bin.readByte();
    }
    if (colorType > 0 && bpp > 8) {
        throw runtime_error("bpp > 16 not supported for color image");
    }
    if (colorType == 2) {
        bmp.bitsPerPixel = 3 * bpp;
    }
    if (colorType == 3) {
        bmp.bitsPerPixel = bpp;
    }
    if (colorType >= 4) {
        bmp.bitsPerPixel = 4 * bpp;
    }
    int signature = bin.readInt();
    //cout << "width: " << bmp.width << endl;
    //cout << "height: " << bmp.height << endl;
    //cout << "bpp: " << bpp << endl;
    //cout << "colorType: " << colorType << endl;
    //cout << "compression: " << compression << endl;
    //cout << "filter: " << filter << endl;
    //cout << "interlace: " << interlace << endl;
    //cout << "signature: " << signature << endl;
    bmp.rowbytesize = (bmp.bitsPerPixel * bmp.width + 7) / 8;
    ArrayWriter output(ENDIAN, BITORDER);
    PNGStream png(output, bmp.width, bmp.height, interlace, bmp.bitsPerPixel);
    BlockedZlibReader zlib(bin, png);
    while (length = bin.readInt(), chunkCode = bin.readInt(), chunkCode != 0x49454e44) { // While not IEND
        pos = 0;
        switch (chunkCode) {
        case 0x504c5445: // PLTE
            if (PLTE) throw runtime_error("PLTE already appeared");
            if (IDAT) throw runtime_error("PLTE after IDAT");
            PLTE = true;
            if (colorType == 0 || colorType == 4) throw runtime_error("PLTE can't appear with colorType " + to_string(colorType));
            if (length % 3 > 0) throw runtime_error("PLTE size isn't divisible by 3");
            if (colorType != 3) {
                break;
            }
            bmp.colorTable.reserve(length / 3);
            for (unsigned int i = 0; i < length / 3; i++) {
                bmp.colorTable.push_back(0xff000000 | bin.readBits(24));
            }
            pos = length;
            break;
        case 0x49444154: // IDAT
            if (colorType == 3 && !PLTE) throw runtime_error("Missing PLTE");
            if (IDAT && !lastIDAT) throw runtime_error("Gap between IDAT");
            if (!IDAT) {
                int compressionMethod = bin.readByte();
                int flags = bin.readByte();
                pos = 2;
                //zlib.start();
            }
            zlib.readBlock(length - pos);
            pos = length;
            IDAT = true;
            break;
        case 0x49484452: // IHDR
            throw runtime_error("Multiple IHDR");
            break;
        }
        lastIDAT = chunkCode == 0x49444154; // IDAT
        for (unsigned int i = 0; i < length - pos; i++) { // Skip block
            bin.readByte();
        }
        signature = bin.readInt();
    }
    zlib.finish();
    signature = bin.readInt();
    cout << output.getData().size() << endl;
    ArrayReader input(output.getData(), ENDIAN, BITORDER);
    bmp.loadData(1, input, 0, true);
    Bitmap flipped(bmp.flip(false, true));
    Bitmap colorFormatted(colorType == 6 ? flipped.ARGBtoRGBA() : flipped);
    return colorFormatted;
}
