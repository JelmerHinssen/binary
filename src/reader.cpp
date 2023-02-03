#include "reader.h"
#include "filereader.h"

namespace shilm {
namespace io {

	std::size_t Reader::read(char* dest, std::size_t size) {
    for (std::size_t i = 0; i < size; i++) {
        dest[i] = read();
    }
	return size;
}

BinaryReader::BinaryReader(int endianness, int bitorder):
    mEndianness(endianness), mBitorder(bitorder),
    mBufferByte(0), mPosition(0), mBitPosition(8) {
	setBitorder(bitorder);
}

uint8_t BinaryReader::readByte() {
    return readNumber<uint8_t>();
}
uint16_t BinaryReader::readShort() {
    return readNumber<uint16_t>();
}
uint32_t BinaryReader::readInt() {
    return readNumber<uint32_t>();
}
uint64_t BinaryReader::readLong() {
    return readNumber<uint64_t>();
}

uint8_t BinaryReader::readBit() {
    if (mBitPosition >= 8) {
        mPosition++;
        mBitPosition = 0;
        mBufferByte = read();
    }

    uint8_t val;
	mBitPosition++;
	/*val = (mBufferByte & mBitmask) >> mShiftsize;
	mBufferByte >>= 1;*/
    if (!isMostSignificantBitFirst()) {
        //val = (mBufferByte & (1 << mBitPosition)) >> mBitPosition;
		val = mBufferByte & 1;
		mBufferByte >>= 1;
    } else {
        //val = (mBufferByte & (1 << (7 - mBitPosition))) >> (7 - mBitPosition);
		val = (mBufferByte & 0x80) >> 7;
		mBufferByte <<= 1;
    }
    //std::cout << "mostsign: " << isMostSignificantBitFirst() << std::endl;
    //std::cout << "bitorder: " << mBitorder << std::endl;
    //std::cout << "Bufferbyte: " << (int) mBufferByte << ", BitPosition: " << (int) mBitPosition << ", val: " << val << std::endl;
    //std::cout << "1 << bitpos: " << (1 << (7-mBitPosition)) << std::endl;
    //std::cout << "masked: " << (mBufferByte & (1 << (7 - mBitPosition))) << std::endl;
    return val;
}

uint64_t BinaryReader::readbits(int count) {
    uint64_t val = 0;
    for (int i = 0; i < count; i++) {
        if (isFlipped()) {
            val += (readBit() << i);
        } else {
            val <<= 1;
            val += readBit();
        }
    }
    return val;
}

std::string BinaryReader::readString() {
    return readString(readInt());
}

std::string BinaryReader::readString(std::size_t size) {
    if (size > 0) {
        //std::cout << "reading: " << size << std::endl;
        std::string str(size, '\0');
        //std::cout << "reading: " << str.size() << std::endl;
        read(&str[0], size);
        //std::cout << "Bitp: " << mBitPosition << std::endl;
        return str;
    } else {
        return "";
    }
}

char* BinaryReader::readData(std::size_t size) {
    char* dest = new char[size];
    read(dest, size);
    return dest;
}

BinaryStreamReader::BinaryStreamReader(std::istream& stream, int endianness, int bitorder, unsigned int buffersize):
    BufferedReader(endianness, bitorder, buffersize),
    mStream(stream) {
}

std::size_t BinaryStreamReader::readraw(char* dest, std::size_t size) {
	if (mStream.eof()) {
		mEof = true;
		return 0;
	}
	mStream.read(dest, (int) size);
	return (int) mStream.gcount();
}

void BinaryReader::flushByte() {
    mBitPosition = 8;
    mBufferByte = 0;
}

}
}
