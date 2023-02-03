#include "file.h"

using namespace shilm::io;

File& File::operator<<(const char* s) {
	if (fp) {
		fputs(s, fp);
	}
	return *this;
}

File& File::operator<<(int s) {
	if (fp) {
		fprintf(fp, "%d", s);
	}
	return *this;
}

File& File::operator<<(unsigned long s) {
	if (fp) {
		fprintf(fp, "%lu", s);
	}
	return *this;
}

File& File::operator<<(size_t s) {
	if (fp) {
		fprintf(fp, "%u", s);
	}
	return *this;
}

File& File::operator<<(void* s) {
	if (fp) {
		fprintf(fp, "%p", s);
	}
	return *this;
}

File& File::operator<<(File& (*func)(File& file)) {
	if (fp) {
		return func(*this);
	}
	return *this;
}

void File::flush() {
	if (fp) {
		fflush(fp);
	}
}

void File::close() {
	if (fp) {
		flush();
		fclose(fp);
		fp = nullptr;
	}
}

void File::open(const char* filename, const char* mode) {
	if (!fp) {
		fopen_s(&fp, filename, mode);
	}
}

File& shilm::io::endl(File& file) {
	return file << "\n";
}
