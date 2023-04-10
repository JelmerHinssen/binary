#pragma once
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>

namespace shilm {
	namespace io {
		class File {
		public:
			File& operator<<(const char* s);
			File& operator<<(int s);
			File& operator<<(unsigned long s);
			File& operator<<(size_t s);
			File& operator<<(void* s);
			File& operator<<(File& (*func)(File& file));
			void flush();
			void close();
			void open(const char* filename, const char* mode);
			inline operator bool() const {
				return fp != nullptr;
			}
		private:
			FILE* fp = nullptr;
		};
		File& endl(File& file);
	}
}
