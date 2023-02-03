#include "writer.h"
#include "filewriter.h"

namespace shilm {
namespace io {


size_t Writer::write(const char* src, size_t size) {
	for (unsigned int i = 0; i < size; i++) {
		write(src[i]);
	}
	return size;
}


BinaryWriter::BinaryWriter(int endianness, int bitorder):
    mEndianness(endianness), mBitorder(bitorder),
    mBufferByte(0), mPosition(0), mBitPosition(0),
	mMsbf((mBitorder & 1) == 1),
	mFlipped((mBitorder & 2) == 2) {

}

void BinaryWriter::flushByte() {
    if (mBitPosition > 0) {
        mBitPosition = 0;
        mPosition++;
        write(mBufferByte);
        mBufferByte = 0;
    }
}

void BinaryWriter::writebits(uint64_t value, int count) {
    using namespace std;
    //cout << "Value: " << value << endl;
    for (int i = 0; i < count; i++) {
        if (isFlipped()) {
            writeBit((uint8_t)( (value & (1 << i)) >> i));
        } else {
            writeBit((uint8_t) value & 1);
            value >>= 1;
        }
    }
}

void BinaryWriter::writeBit(uint8_t bit) {
    if (mBitPosition >= 8) {
        flushByte();
    }
    if (!isMostSignificantBitFirst()) {
        mBufferByte |= (bit << mBitPosition);
    } else {
        mBufferByte |= (bit << (7 - mBitPosition));
    }
    mBitPosition++;
}

void BinaryStreamWriter::write(uint8_t b) {
	mBuffer.add(b, *this);
}

size_t BinaryStreamWriter::write(const char* src, size_t size) {
	mBuffer.add((unsigned char*) src, size, *this);
	return size;
}

void BinaryStreamWriter::flushStream() {
	mStream.write((char*) &mBuffer[0], mBuffer.size());
    mStream.flush();
}

void BinaryWriter::writeByte(uint8_t val) {
    writeNumber<uint8_t>(val);
}
void BinaryWriter::writeShort(uint16_t val) {
    writeNumber<uint16_t>(val);
}
void BinaryWriter::writeInt(uint32_t val) {
    writeNumber<uint32_t>(val);
}
void BinaryWriter::writeLong(uint64_t val) {
    if (mEndianness == LITTLE_ENDIAN) {
        writeInt((uint32_t) val);
        writeInt((uint32_t) (val >> 32));
    } else {
        writeInt((uint32_t) (val >> 32));
        writeInt((uint32_t) val);
    }
}

void BinaryWriter::writeString(const std::string& str) {
    writeInt(str.size());
    write(str.c_str(), str.size());
}

void BinaryWriter::writeCString(const char* str){
    size_t len = strlen(str);
    write(str, len + 1);
}
}
}

