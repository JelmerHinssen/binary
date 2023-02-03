#pragma once
#include "binaryexport.h"
#include "writer.h"
#include "reader.h"
#include <cstring>
#include <algorithm>

namespace shilm {
	namespace io {
		class SHILM_IO_EXPORT Buffer {
		public:
			Buffer(unsigned int bufferSize) :
				mBufferCapacity(bufferSize),
				mBufferSize(0),
				mBuffer(new unsigned char[bufferSize]) {
			}
			virtual ~Buffer() {
				delete[] mBuffer;
			}
			inline void flush(BinaryWriter& writer) {
				//writer.write((char*) mBuffer, mBufferSize);
				writer.flushStream();
				mBufferSize = 0;
			}
			
			inline void addCyclic(unsigned char val) {
				mBuffer[mBufferSize++] = val;
				if (mBufferSize == mBufferCapacity) {
					mBufferSize = 0;
				}
			}
			inline void add(unsigned char val) {
				mBuffer[mBufferSize++] = val;
			}
			inline void add(unsigned char val, BinaryWriter& writer) {
				mBuffer[mBufferSize++] = val;
				if (mBufferSize == mBufferCapacity) {
					flush(writer);
				}
			}
			/**
			Copies <code>size</code bytes from <code>val</code> to the buffer.
			*/
			inline void append(unsigned char* val, unsigned int size) {
				//std::memcpy(mBuffer + mBufferSize, val, size);
				std::memmove(mBuffer + mBufferSize, val, size);
				mBufferSize += size;
			}
			/**
			Copies <code>size</code> bytes from <code>val</code> to the buffer. Doesn't flush
			*/
			inline void add(unsigned char* val, unsigned int size) {
				while (size + mBufferSize >= mBufferCapacity) {
					unsigned int part = mBufferCapacity - mBufferSize;
					size -= part;
					append(val, part);
					val += part;
					mBufferSize = 0;
				}
				append(val, size);
			}
			/**
			Copies <code>size</code> bytes from <code>val</code> to the buffer and flushes whenever needed
			*/
			inline void add(unsigned char* val, unsigned int size, BinaryWriter& writer) {
				while (size + mBufferSize >= mBufferCapacity) {
					unsigned int part = mBufferCapacity - mBufferSize;
					size -= part;
					append(val, part);
					val += part;
					flush(writer);
				}
				append(val, size);
			}
			/**
			Appends <code>size</code> bytes from the <code>position</code> to the buffer.
			Flushing when it needs to and restarting at the beginning of the buffer.
			If <code>position > capacity()</code> undefined behaviour
			*/
			inline void copy(unsigned int position, unsigned int size, BinaryWriter& writer) {
				throw std::runtime_error("Not implemented yet");
				while (size + position >= mBufferCapacity) {
					size -= (mBufferCapacity - position);
					append(mBuffer + position, mBufferCapacity - position);
					flush(writer);
					position = 0;
				}
				append(mBuffer + position, size);
			}
			/**
			Appends <code>size</code> bytes from the <code>position</code> to the buffer.
			Restarting at the beginning of the buffer when it reaches the end. Doesn't flush
			If <code>position > capacity()</code> undefined behaviour
			If <code>size > capacity()</code> undefined behaviour
			*/
			inline void copy(unsigned int position, unsigned int size) {
				/*if (position == mBufferSize) {
					mBufferSize += size;
					mBufferSize %= mBufferCapacity;
					return;
				}*/
				// TODO Fix this
				if (position + size > mBufferSize) { 
					if (position < mBufferSize) { // Source overlaps with destination
						unsigned int chunkSize = mBufferSize - position;
						while (chunkSize < size) {
							copyNoOverlap(position, chunkSize);
							size -= chunkSize;
							chunkSize *= 2;
						}
						copyNoOverlap(position, size);
					} else { // Source loops around buffer
						copyNoOverlap(position, size);
					}
				} else { // Source is continuous
					add(mBuffer + position, size);
				}
				
			}
			inline unsigned char& operator[](unsigned int position) {
				return mBuffer[position];
			}
			inline unsigned int capacity() const {
				return mBufferCapacity;
			}
			inline unsigned int size() const {
				return mBufferSize;
			}
			inline void reset() {
				mBufferSize = 0;
			}
		private:
			inline void copyNoOverlap(unsigned int position, unsigned int size) {
				while (position + size > mBufferCapacity) { // Source loops
					add(mBuffer + position, mBufferCapacity - position);
					size -= mBufferCapacity - position;
				}
				add(mBuffer + position, size);
			}
			const unsigned int mBufferCapacity;
			unsigned int mBufferSize;
			unsigned char* const mBuffer;
		};

		class SHILM_IO_EXPORT BufferedReader : public BinaryReader {
		public:
			BufferedReader(int endianness, int bitorder, std::size_t bufferSize) :
				BinaryReader(endianness, bitorder),
				mBufferCapacity(bufferSize),
				mBufferSize(0),
				mBufferPos(0),
				mBuffer(new unsigned char[bufferSize]) {}
			virtual ~BufferedReader() {
				delete[] mBuffer;
			}
			inline void refill() {
				//writer.write((char*) mBuffer, mBufferSize);
				//writer.flushStream
				mBufferSize = readraw((char*) mBuffer, mBufferCapacity);
				mBufferPos = 0;
			}
			virtual std::size_t readraw(char* dest, std::size_t size) = 0;
			inline uint8_t read() override {
				if (eof()) {
					return (uint8_t) -1;
				}
				if (mBufferPos == mBufferSize) {
					refill();
				}
				uint8_t val = mBuffer[mBufferPos++];
				if (mBufferPos == mBufferSize) {
					refill();
				}
				return val;
			}
			inline std::size_t read(char* dest, std::size_t size) override {
				if (mBufferSize - mBufferPos > size) {
					std::memcpy(dest, mBuffer + mBufferPos, size);
					mBufferPos += size;
					return size;
				}
				std::size_t byteCount = 0;
				while (size > 0 && !eof()) {
					std::size_t chunkSize = std::min(size, mBufferSize - mBufferPos);
					std::memcpy(dest, mBuffer + mBufferPos, chunkSize);
					dest += chunkSize;
					size -= chunkSize;
					byteCount += chunkSize;
					refill();
				}
				return byteCount;
			}
			inline unsigned char& operator[](std::size_t position) {
				return mBuffer[position];
			}
			inline std::size_t capacity() const {
				return mBufferCapacity;
			}
			inline std::size_t size() const {
				return mBufferSize;
			}
			inline std::size_t position() const {
				return mBufferPos;
			}
		private:
			const std::size_t mBufferCapacity;
			std::size_t mBufferSize;
			std::size_t mBufferPos;
			unsigned char* const mBuffer;
		};
	}
}