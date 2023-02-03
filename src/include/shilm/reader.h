#pragma once
#include "binaryexport.h"
#include <stdint.h>
#include <iostream>

namespace shilm {
	namespace io {


		class SHILM_IO_EXPORT Reader {
		public:
			virtual uint8_t read() = 0;
			/**
			Reads <code>size</code> bytes from the stream to <code>dest</code>.
			Returns the number of bytes read.
			*/
			virtual std::size_t read(char* dest, std::size_t size);
			virtual bool eof() const = 0;
		};

		class SHILM_IO_EXPORT BinaryReader : public Reader {
		public:
			static constexpr int BIG_ENDIAN = 0, LITTLE_ENDIAN = 1;
			static constexpr int MOST_SIGNIFICANT_BIT_FIRST = 1, LEAST_SIGNIFICANT_BIT_FIRST = 0;
			static constexpr int MOST_SIGNIFICANT_BIT_FIRST_FLIPPED = 3, LEAST_SIGNIFICANT_BIT_FIRST_FLIPPED = 2;
			BinaryReader(int endianness, int bitorder);
			uint8_t readByte();
			uint16_t readShort();
			uint32_t readInt();
			uint64_t readLong();

			int8_t readSignedByte() { return readByte(); }
			int16_t readSignedShort() { return readShort(); }
			int32_t readSignedInt() { return readInt(); }
			int64_t readSignedLong() { return readLong(); }

			template<typename T>
			BinaryReader& operator>>(T& result) {
				result = readNumber<T>();
				return *this;
			}

			std::string readString();
			std::string readString(std::size_t size);
			char* readData(std::size_t size);

			template<typename T = int>
			T readBits(int count) {
				return (T)readbits(count);
			}
			uint8_t readBit();

			inline std::size_t getPosition() const {
				return mPosition;
			}
			inline unsigned short getBitPosition() const {
				return mBitPosition;
			}
			inline virtual void setEndianness(int end) {
				//flushByte();
				mEndianness = end;
			}
			inline virtual void setBitorder(int order) {
				//flushByte();
				mBitorder = order;
				mMsbf = (mBitorder & 1) == 1;
				mFlipped = (mBitorder & 2) == 2;
				if (!mMsbf) {
					mBitmask = 1;
					mShiftsize = 0;
				} else {
					mBitmask = 0x80;
					mShiftsize = 7;
				}
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
			void flushByte();
		private:
			uint64_t readbits(int count);
			template<typename T>
			T readNumber() {
				flushByte();
				T result = 0;
				for (unsigned int i = 0; i < sizeof(T); i++) {
					T byte = read();
					mPosition++;
					if (mEndianness == BIG_ENDIAN) {
						result <<= 8;
						result += byte;
					} else {
						result += byte << (i * 8);
					}
				}
				return result;
			}
		private:
			int mEndianness, mBitorder;
			bool mMsbf;
			bool mFlipped;
			uint8_t mBitmask, mShiftsize;
			uint8_t mBufferByte;
			std::size_t mPosition;
			unsigned short mBitPosition;
		};
	}
}

