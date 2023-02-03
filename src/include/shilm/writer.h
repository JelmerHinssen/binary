#pragma once
#include "binaryexport.h"
#include <stdint.h>
#include <iostream>

namespace shilm {
	namespace io {

		class SHILM_IO_EXPORT Writer {
		public:
			virtual void write(uint8_t b) = 0;
			/**
			Writes <code>size</code> bytes from src to the stream.
			Returns the number of bytes written.
			*/
			virtual size_t write(const char* src, size_t size);
			virtual void flushStream() = 0;
		};

		class SHILM_IO_EXPORT BinaryWriter : public Writer {
		public:
			static constexpr int BIG_ENDIAN = 0, LITTLE_ENDIAN = 1;
			static constexpr int MOST_SIGNIFICANT_BIT_FIRST = 1, LEAST_SIGNIFICANT_BIT_FIRST = 0;
			static constexpr int MOST_SIGNIFICANT_BIT_FIRST_FLIPPED = 3, LEAST_SIGNIFICANT_BIT_FIRST_FLIPPED = 2;
			BinaryWriter(int endianness, int bitorder);

			/**
			Writes all currently partially written bytes
			*/
			void flushByte();

			inline void flush() {
				flushByte();
				flushStream();
			}
			void writeByte(uint8_t);
			void writeShort(uint16_t);
			void writeInt(uint32_t);
			void writeLong(uint64_t);

			void writeSignedByte(int8_t b) { writeByte(b); }
			void writeSignedShort(int16_t s) { writeShort(s); }
			void writeSignedInt(int32_t i) { writeInt(i); }
			void writeSignedLong(int64_t l) { writeLong(l); }

			void writeString(const std::string& str);
			void writeCString(const char* str);

			template<typename T = int>
			void writeBits(T value, int count) {
				writebits((uint64_t)value, count);
			}
			void writeBit(uint8_t);

			inline std::size_t getPosition() const {
				return mPosition;
			}

			inline bool isMostSignificantBitFirst() const {
				return mMsbf;
			}
			inline bool isFlipped() const {
				return mFlipped;
			}

			inline int getEndianness() const {
				return mEndianness;
			}
			inline int getBitOrder() const {
				return mBitorder;
			}

		protected:
		private:
			void writebits(uint64_t value, int count);
			template<typename T>
			void writeNumber(T value) {
				flushByte();
				for (unsigned int i = 0; i < sizeof(T); i++) {
					mPosition++;
					if (mEndianness == BIG_ENDIAN) {
						write((value & (0xff << ((sizeof(T) - i - 1) * 8))) >> ((sizeof(T) - i - 1) * 8));
					} else {
						write((value & (0xff << (i * 8))) >> (i * 8));
					}
				}
			}
		private:
			int mEndianness, mBitorder;
			bool mMsbf;
			bool mFlipped;
			uint8_t mBufferByte;
			std::size_t mPosition;
			unsigned short mBitPosition;
		};
	}
}

