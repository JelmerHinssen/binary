#include "zlib.h"

using namespace shilm::io;

BlockedZlibReader::~BlockedZlibReader() {
}

BlockedZlibReader::BlockedZlibReader(BinaryReader& parent, BinaryWriter& dest):
    ZlibReader(mReader, false), mReader(parent), mDest(dest) {
}

void BlockedZlibReader::readBlock(std::size_t blockSize) {
    //if (mFirst) {
        resume();
        mFirst = false;
    //}
	std::size_t readCount = 0;
	std::size_t startPosition = mReader.getPosition();
    blockSize += mReader.getSize();
	mDest.flush();
    while (readCount < blockSize - 3 && !eof()) {
        if (!eob() || (blockSize - readCount >= MAX_HUFFMANBLOCK_LENGTH)) {
            mDest.write(read());
            readCount = mReader.getPosition() - startPosition;
        } else {
            break;
        }
    }
    mReader.createBuffer(blockSize - readCount);
    pause();
}

void BlockedZlibReader::finish() {
    //if (mFirst) {
    resume();
    mFirst = false;
    //}
	mDest.flush();
    while (!eof()) {
        uint8_t val = read();
        if (!eof()) {
            mDest.write(val);
        }
    }
    close();
}
