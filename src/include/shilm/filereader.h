#pragma once
#include "reader.h"
#include "buffer.h"


namespace shilm {
	namespace io {
		class SHILM_IO_EXPORT BinaryStreamReader : public BufferedReader {
		public:
			BinaryStreamReader(std::istream& stream, int endianness, int bitorder, unsigned int buffersize = 1 << 15);
			virtual std::size_t readraw(char* dest, std::size_t size) override;
			inline virtual bool eof() const override {
				return mEof;
			}
		private:
			static const int BUFFER_SIZE = 1024;
			std::istream& mStream;
			uint8_t mBuffer[BUFFER_SIZE];
			unsigned int mBufferSize = 0;
			unsigned int mBufferPos = 0;
			bool mEof = false;
		};
	}
}
