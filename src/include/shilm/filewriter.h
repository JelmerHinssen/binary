#pragma once
#include "writer.h"
#include "buffer.h"

namespace shilm {
	namespace io {
		class SHILM_IO_EXPORT BinaryStreamWriter : public BinaryWriter {
		public:
			BinaryStreamWriter(std::ostream& stream, int endianness, int bitorder, unsigned int buffer = 1 << 17) :
				BinaryWriter(endianness, bitorder), mStream(stream),
				mBuffer(buffer) {}
			virtual void write(uint8_t b) override;
			size_t write(const char* src, size_t size) override;
			virtual void flushStream() override;
		protected:
		private:
			std::ostream& mStream;
			shilm::io::Buffer mBuffer;
		};
	}
}

