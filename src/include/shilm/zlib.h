#pragma once
#include "binaryexport.h"
#include "reader.h"
#include "writer.h"
#include <vector>
#include <utility>
#include "bassert.h"
#include "buffer.h"


namespace shilm {
namespace io {

class SHILM_IO_EXPORT ZlibReader : public BinaryReader {
public:
    ZlibReader(BinaryReader& parent, bool open = true, unsigned int bufferSize = 1 << 15);
    virtual ~ZlibReader();
    virtual uint8_t read() override;
    inline bool eof() const {
        return mEof;
    }
    inline bool eob() const {
        return mEob;
    }
    void close();
    void reopen();
protected:
    void pause();
    void resume();
private:
    struct HuffmanTreeNode {
        HuffmanTreeNode* children[2] = {nullptr};
        int value;
        ~HuffmanTreeNode() {
            delete children[0];
            delete children[1];
        }
        static HuffmanTreeNode* generate(int* lengths, int count);
    };
    static void add(HuffmanTreeNode* node, int code, int codeLength, int value);
    int readNextCode(HuffmanTreeNode* root);
    std::pair<HuffmanTreeNode*, HuffmanTreeNode*> loadDynamicTree();
    void nextBlock();
    //void addToBuffer(unsigned char val);
protected:
    BinaryReader& mStream;
private:
    bool mFinal = false;
	Buffer mBuffer;
    uint8_t mCompression;
    bool mEof = false; // End of file
    bool mEob = false; // End of block
    std::pair<HuffmanTreeNode*, HuffmanTreeNode*> mHuffmanTree = {nullptr, nullptr};
	unsigned int mBufferPos = 0, mPos = 0;
    unsigned int mLength = 0;
};

class SHILM_IO_EXPORT BlockedZlibReader : protected ZlibReader {
private:
    template<unsigned int SIZE>
    class PrefixReader : public BinaryReader {
    public:
        PrefixReader(BinaryReader& parent) :
            BinaryReader(parent.getEndianness(), parent.getBitOrder()), mParent(parent) {
        }
        void createBuffer(std::size_t size) {
            assert(size < SIZE);

            for (std::size_t i = 0; i < size; i++) {
                mBuffer[i] = read();
            }
            mBufferPos = 0;
            mBufferSize = size;
        }
        inline virtual bool eof() const override {
            return mBufferPos == mBufferSize && mParent.eof();
        }
        inline virtual void setEndianness(int end) {
            BinaryReader::setEndianness(end);
            mParent.setEndianness(end);
        }
        inline virtual void setBitorder(int order) {
            BinaryReader::setBitorder(order);
            mParent.setBitorder(order);
        }
        virtual uint8_t read() override {
            //std::cout << "read" << std::endl;
            if (mBufferPos < mBufferSize) {
                return mBuffer[mBufferPos++];
            }
            return mParent.read();
        }
        inline std::size_t getSize() const {
            return mBufferSize;
        }
    private:
        char mBuffer[SIZE] = {0};
        BinaryReader& mParent;
		std::size_t mBufferSize = 0;
		std::size_t mBufferPos = 0;
    };
public:
    BlockedZlibReader(BinaryReader& parent, BinaryWriter& dest);
    virtual ~BlockedZlibReader();
    /**
    Reads <code>blockSize</code> bytes of compressed data
    */
    void readBlock(std::size_t blockSize);
    /**
    Reads all remaining data
    */
    void finish();
    inline void start() {
        reopen();
    }
private:
    static constexpr int MAX_HUFFMANBLOCK_LENGTH = 300;
    PrefixReader<MAX_HUFFMANBLOCK_LENGTH> mReader;
    BinaryWriter& mDest;
    bool mFirst = true;
};

}
}
