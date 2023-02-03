#include "zlib.h"
#include <exception>

using namespace shilm::io;

void ZlibReader::add(HuffmanTreeNode* node, int code, int codeLength, int value) {
    if (codeLength == 0) {
        node->value = value;
    } else {
        int8_t branch = (code & (1 << (codeLength - 1))) >> (codeLength - 1);
        if (!node->children[branch]) {
            node->children[branch] = new HuffmanTreeNode();
        }
        add(node->children[branch], code, codeLength - 1, value);
    }
}

ZlibReader::HuffmanTreeNode* ZlibReader::HuffmanTreeNode::generate(int* lengths, int count) {
    HuffmanTreeNode* root = new HuffmanTreeNode();
    constexpr int MAX_BITS = 16;
    int bl_count[MAX_BITS + 1] = {0};
    for (int i = 0; i < count; i++) {
        if (lengths[i]) {
            bl_count[lengths[i]]++;
        }
    }
    int code = 0;
    bl_count[0] = 0;
    int next_code[MAX_BITS + 1] = {0};
    for (int bits = 1; bits <= MAX_BITS; bits++) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    for (int i = 0; i < count; i++) {
        int len = lengths[i];
        if (len != 0) {
            add(root, next_code[len]++, len, i);
        }
    }
    return root;
}

std::pair<ZlibReader::HuffmanTreeNode*, ZlibReader::HuffmanTreeNode*> ZlibReader::loadDynamicTree() {

    int hlit = mStream.readBits(5);
    int hdist = mStream.readBits(5);
    int hclen= mStream.readBits(4);
    int lengths[19] = {0};
    int alphabetorder[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    for (int i = 0; i < hclen + 4; i++) {
        lengths[alphabetorder[i]] = mStream.readBits(3);
    }
    HuffmanTreeNode* generatorRoot = HuffmanTreeNode::generate(lengths, 19);
    int alphabetLengths[286 + 32] = {0};
    for (int i = 0; i < hlit + hdist + 258; i++) {
        int code = readNextCode(generatorRoot);
        //cout << code;
        if (code < 16) {
            alphabetLengths[i] = code;
        } else {
            int extralength;
            switch (code) {
            case 16:
                extralength = mStream.readBits(2);
                for (int j = 0; j < extralength + 3; j++) {
                    alphabetLengths[i + j] = alphabetLengths[i - 1];
                }
                i += extralength + 2;
                break;
            case 17:
                extralength = mStream.readBits(3);
                for (int j = 0; j < extralength + 3; j++) {
                    alphabetLengths[i + j] = 0;
                }
                i += extralength + 2;
                break;
            case 18:
                extralength = mStream.readBits(7);
                for (int j = 0; j < extralength + 11; j++) {
                    alphabetLengths[i + j] = 0;
                }
                i += extralength + 10;
                break;
            }
            //cout << ":" << extralength;
        }
        //cout << endl;
    }
    delete generatorRoot;
    HuffmanTreeNode* realroot = HuffmanTreeNode::generate(alphabetLengths, hlit + 257);
    HuffmanTreeNode* distanceroot = HuffmanTreeNode::generate(alphabetLengths + hlit + 257, hdist + 1);
    return {realroot, distanceroot};
}

constexpr int NO_COMPRESSION = 0, FIXED_HUFFMAN = 1, DYNAMIC_HUFFMAN = 2, RESERVED = 3;
constexpr int BUFFER_SIZE = 0x8000;

void ZlibReader::nextBlock() {
	delete mHuffmanTree.first;
	delete mHuffmanTree.second;
	mHuffmanTree.first = nullptr;
	mHuffmanTree.second = nullptr;
    if (mFinal) {
        mEof = true;
        return;
    }
    mEob = false;
    mFinal = mStream.readBits(1) == 1;
    mCompression = mStream.readBits<uint8_t>(2);
    mPos = 0;
    switch (mCompression) {
        case NO_COMPRESSION: {
            mStream.flushByte();
            int16_t len = mStream.readBits<int16_t>(16);
            int16_t nlen = mStream.readBits<int16_t>(16);
            if (len != ~nlen)
                throw std::runtime_error("len != ~nlen");
            mLength = len;
            break;
        }
        case FIXED_HUFFMAN: {
            int huffmantreelengths[288];
            int distancelengths[32];
            for (int i = 0; i < 32; i++) {
                distancelengths[i] = 5;
            }
            for (int i = 0; i < 144; i++) {
                huffmantreelengths[i] = 8;
            }
            for (int i = 144; i < 256; i++) {
                huffmantreelengths[i] = 9;
            }
            for (int i = 256; i < 280; i++) {
                huffmantreelengths[i] = 7;
            }
            for (int i = 280; i < 288; i++) {
                huffmantreelengths[i] = 8;
            }
            mHuffmanTree.first = HuffmanTreeNode::generate(huffmantreelengths, 288);
            mHuffmanTree.second = HuffmanTreeNode::generate(distancelengths, 32);
            break;
        }
        case DYNAMIC_HUFFMAN: {
            mHuffmanTree = loadDynamicTree();
            break;
        }
        case RESERVED:
            throw std::runtime_error("Invalid compression format");
    }
}

 ZlibReader::ZlibReader(BinaryReader& parent, bool open, unsigned int bufferSize):
        BinaryReader(parent.getEndianness(), parent.getBitOrder()),
		mBuffer(bufferSize),
        mStream(parent) {
    using namespace std;

    //cout << (int) mStream.readByte() << endl;
    //mBuffer.resize(BUFFER_SIZE);
    mEob = true;
    if (open) {
        reopen();
    }
    //cout << "created" << endl;
}

int ZlibReader::readNextCode(HuffmanTreeNode* root) {
	// TODO Optimize the way the trees are stored
	/* Probably in the following way:
		Store an array of 256 elements of 9 bytes. The index of the array is the bitsequence that is read.
		The value determines what value and how many values can be extracted from the bitsequence in the 
		huffmantree. This is stored in this way:
		union {
			uint8_t type;
			struct {
				uint8_t type; // 0
				void* next;
			}
			struct {
				uint8_t type; // 1
				uint8_t none;
				uint16_t value;
				uint8_t none[4];
				uint8_t bitsConsumed;
			}
			struct {
			uint8_t type; // 2 ... 8
			uint8_t value[7];
			uint8_t bitsConsumed;
			}
			struct {
			uint8_t type; // 8
			uint8_t value[8];
			}
		}
		value[i] = byte i
		type = first 4 bits of value[0]
		if type == 0: 8 bits aren't enough to extract one value. Bytes 1 throug 8 contain a pointer to
		another array of 256 elements containing the remaining part of the code
		if type == 1: *((uint16_t*) &value[2]) contains the resulting value
		if type > 1: extract type values: value[1], value[2], ..., value[type]
		if type >= 1 && type < 8: value[8] contains the number of bits consumed
		if type == 8: 8 bits are consumed

		The array the pointer points to when type == 0 consists of 256 times 3 bytes:
		struct {
			uint16_t value;
			uint8_t bitsConsumed;
		}
	*/
    while(root->children[0] || root->children[1]) {
        root = root->children[mStream.readBit()];
    }
    return root->value;
}

/*void ZlibReader::addToBuffer(unsigned char val) {
    mBuffer[mBufferSize++] = val;
	if (mBufferSize == BUFFER_SIZE) {
		mBufferSize = 0;
	}
}*/


uint8_t ZlibReader::read() {
    while (mBufferPos == mBuffer.size() && !mEof) { // At limit of buffer
        if (mEob) {
            nextBlock();
        }
		if (mEof) {
			break;
		}
        if (mCompression == NO_COMPRESSION) {
            mPos++;
            if (mPos >= mLength) {
                mEob = true;
                continue;
            }
			mBuffer.addCyclic(mStream.readByte());
        } else {
            unsigned int length;
            int action = readNextCode(mHuffmanTree.first);
            if (action < 256) {
				mBuffer.addCyclic((unsigned char) action);
                continue;
            } else if (action == 256) {
                mEob = true;
                continue;
            } else if (action < 265) {
                length = action - 257 + 3;
            } else if (action < 285) {
                int block = (action - 265) / 4;
                length = (action - (265 + block * 4)) * (1 << (block + 1)) + (1 << (block + 3)) + 3 + mStream.readBits(block + 1);
            }else if (action == 285) {
                length = 258;
            } else {
                throw std::runtime_error("Invalid length");
            }
            action = readNextCode(mHuffmanTree.second);
            unsigned int distance;
            if (action < 4) {
                distance = action + 1;
            } else if (action < 30) {
                int block = (action - 4) / 2;
                distance = (action - (4 + block * 2)) * (1 << (block + 1)) + (1 << (block + 2)) + 1 + mStream.readBits(block + 1);
            } else {
                throw std::runtime_error("Invalid distance");
            }
            //cout << "length: " << length << ", distance: " << distance << endl;
			//mBuffer.copy((distance > mBuffer.size()) ? (mBuffer.capacity() + mBuffer.size() - distance) : (mBuffer.size() - distance), length);
			for (unsigned int i = 0; i < length; i++) {
				mBuffer.addCyclic(mBuffer[(distance > mBuffer.size()) ? (mBuffer.capacity() + mBuffer.size() - distance) : (mBuffer.size() - distance)]);
				//mBuffer.addCyclic(255);
			}
        }
    }
    if (mEof) {
        return (uint8_t) -1;
    }
	int bufPos = mBufferPos;
	mBufferPos++;
	if (mBufferPos == mBuffer.capacity()) {
		mBufferPos = 0;
	}
    return mBuffer[bufPos++];
}

ZlibReader::~ZlibReader() {
    delete mHuffmanTree.first;
    delete mHuffmanTree.second;
}

void ZlibReader::close() {
    mEof = true;
    mEob = true;
    pause();
}

void ZlibReader::reopen() {
    mEof = false;
    mFinal = false;
    mEob = true;
	mBuffer.reset();
    mBufferPos = 0;
    mPos = 0;
    mLength = 0;
    resume();
}

void ZlibReader::pause() {
    mStream.setEndianness(getEndianness());
    mStream.setBitorder(getBitOrder());
}

void ZlibReader::resume() {
    setEndianness(mStream.getEndianness());
    setBitorder(mStream.getBitOrder());
    mStream.setEndianness(BinaryReader::LITTLE_ENDIAN);
    mStream.setBitorder(BinaryReader::LEAST_SIGNIFICANT_BIT_FIRST_FLIPPED);
}


