#pragma once
#include <Windows.h>
//#pragma warning(push)
//#pragma warning(disable:4091)
#include <DbgHelp.h>
//#pragma warning(pop)
#include <fstream>
#include <iostream>
#include "file.h"
#include <cstring>

#ifndef _WIN32
#undef SHILM_ENABLE_STACKTRACE
#endif

// #define strcpy_s(dest, src) strcpy(dest, src)
// #define strcat_s(dest, size, src) strcat(dest, src)
// #define _splitpath_s(full, drive, drivesize, path, pathsize, name, namesize, ext, extsize) _splitpath(full, drive, path, name, ext)
// #define strnlen_s(str, size) strnlen(str, size)

namespace shilm {
	namespace memory {
		inline void initStacktrace() {
#ifdef SHILM_ENABLE_STACKTRACE
			SymInitialize(GetCurrentProcess(), nullptr, true);
#endif
		}
		template<int SIZE>
		struct Stacktrace {
			DWORD hash = 0;
			DWORD size = 0;
			void* trace[SIZE] = { nullptr };
			Stacktrace& operator=(const Stacktrace& tr) {
				hash = tr.hash;
				size = tr.size;
				for (DWORD i = 0; i < size; i++) {
					trace[i] = tr.trace[i];
				}
				return *this;
			}
			inline static Stacktrace<SIZE> getStacktrace(DWORD skip = 0) {
				Stacktrace trace;
#ifdef _WIN32
				trace.size = CaptureStackBackTrace(1 + skip, SIZE, trace.trace, &trace.hash);
#endif
				return trace;
			}

			void getNames(char names[SIZE][256]) const {
#ifdef SHILM_ENABLE_STACKTRACE
				SYMBOL_INFO* symbol = (SYMBOL_INFO*)alloca(sizeof(SYMBOL_INFO) + (255 * sizeof(TCHAR)));
				symbol->SizeOfStruct = sizeof(SYMBOL_INFO);// sizeof(SYMBOL_INFO) + (255 * sizeof(TCHAR));
				symbol->MaxNameLen = 256;
				IMAGEHLP_LINE line;
				line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
				DWORD64 disp;
				DWORD dispp;
#endif
				for (DWORD i = 0; i < size; i++) {
#ifdef SHILM_ENABLE_STACKTRACE
					if (SymFromAddr(GetCurrentProcess(), (DWORD64)trace[i], &disp, symbol)) {
						if (SymGetLineFromAddr(GetCurrentProcess(), (DWORD64)trace[i], &dispp, &line)) {
							strcpy_s(names[i], symbol->Name);
							strcat_s(names[i], 256, "(");
							//if (strcat_s(names[i], 256, line.FileName)) { // Path is too long
							char name[64];
							char ext[32];
							_splitpath_s(line.FileName, nullptr, 0, nullptr, 0, name, 64, ext, 32);
							strcat_s(names[i], 256, name);
							strcat_s(names[i], 256, ext);
							//}
							strcat_s(names[i], 256, "(");
							size_t len = strnlen_s(names[i], 256);
							sprintf_s(names[i] + len, 256 - len, "%lu", line.LineNumber);
							//_itoa_s(line.LineNumber, names[i] + len, 256 - len, 10);
							strcat_s(names[i], 256, ")");
							strcat_s(names[i], 256, ")");
						} else {
							strcpy_s(names[i], symbol->Name);
							sprintf_s(names[i] + symbol->NameLen, 256 - symbol->NameLen, "(%lu)", GetLastError());
						}
						//cout << symbol->Name << endl;
					} else {
						sprintf_s(names[i], 256, "%p (%lu)", trace[i], GetLastError());
					}
#else
					for (int j = 0; j < sizeof(void*) / 4; j++) {
						sprintf_s(names[i], 256, "%p", trace[i]);
					}
#endif
				}
			}
		};
		class Allocator {
		public:
			static const int TRACE_SIZE = 8;
			static Allocator& getDefault() {
				static Allocator all;
				return all;
			}
			Allocator() {
				log.open("log.txt", "w");
				log << "Allocator started" << shilm::io::endl;
				log.flush();
			}

			void* allocate(size_t size, bool isarray);

			void release(void* p, bool isarray);

			virtual ~Allocator();
		private:
			static const int BUCKETS = 127;
			static const int BUCKET_SIZE = 32;
			//static const int  = 32;
			struct StacktraceInfo : public Stacktrace<TRACE_SIZE> {
				StacktraceInfo() {
					allocatedMemory = 0;
					allocatedObjects = 0;
				}
				StacktraceInfo(const Stacktrace<TRACE_SIZE>& trace) {
					allocatedMemory = 0;
					allocatedObjects = 0;
					memcpy(this, &trace, sizeof(trace));
				}
				size_t allocatedMemory;
				size_t allocatedObjects;
			};
			struct Bucket {
				DWORD size = 0;
				StacktraceInfo traces[BUCKET_SIZE];
				StacktraceInfo* add(const Stacktrace<TRACE_SIZE>& trace) {
					return add(StacktraceInfo(trace));
				}
				StacktraceInfo* add(const StacktraceInfo& trace) {
					if (size < BUCKET_SIZE) {
						traces[size] = trace;
						size++;
						return &traces[size - 1];
					} else {
						return nullptr;
					}
				}
				StacktraceInfo* get(DWORD hash) {
					for (DWORD i = 0; i < size; i++) {
						if (traces[i].hash == hash) {
							return &traces[i];
						}
					}
					return nullptr;
				}
			};
			struct HashMap {
				Bucket buckets[BUCKETS];
				inline StacktraceInfo* add(const StacktraceInfo& trace) {
					return buckets[trace.hash % BUCKETS].add(trace);
				}
				inline StacktraceInfo* get(DWORD hash) {
					return buckets[hash % BUCKETS].get(hash);
				}
				class Iterator {
				public:
					Iterator(HashMap* map, int bucket = 0, int pos = 0) :
						map(map), bucket(bucket), pos(pos) {

					}
					inline StacktraceInfo& operator*() {
						return map->buckets[bucket].traces[pos];
					}
					inline bool operator==(const Iterator& other) {
						return other.map == map && bucket == other.bucket && pos == other.pos;
					}
					inline bool operator!=(const Iterator& other) {
						return !operator==(other);
					}
					Iterator& operator++();
				private:
					HashMap* map;
					size_t bucket;
					size_t pos;
				};
				inline Iterator begin() {
					return Iterator(this);
				}
				inline Iterator end() {
					return Iterator(this, BUCKETS);
				}
			};
			HashMap traces;
			size_t totalAllocated;
			size_t totalObjects;
			io::File log;
			//std::ofstream log;

			bool overflow = false;
		};
	}
}

#ifdef SHILM_ENABLE_ALLOCATOR

void* operator new(size_t size);
void operator delete(void* p);
void* operator new[](size_t size);
void operator delete[](void* p);

#endif
