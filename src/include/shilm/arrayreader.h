#pragma once
#include "reader.h"
#include <vector>
#include <cstring>
#include <algorithm>

namespace shilm {
namespace io {

class ArrayReader : public BinaryReader {
public:
    ArrayReader(const std::vector<uint8_t>& data, int endianness, int bitorder) :
        BinaryReader(endianness, bitorder), mData(data) {
    }

    bool eof() const {
        return mIndex >= mData.size();
    }

    virtual uint8_t read() override {
        if (eof()) {
            return -1;
        }
        return mData[mIndex++];
    }
	virtual std::size_t read(char* dest, std::size_t size) override {
		size = std::min(mData.size() - mIndex, size);
		std::memcpy(dest, &mData[0] + mIndex, size);
		return size;
	}
private:
    std::vector<uint8_t> mData;
    unsigned int mIndex = 0;
};

}
}
