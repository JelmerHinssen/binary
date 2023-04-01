#include "memory.h"
#include <fstream>
#include <Windows.h>

using namespace std;

using namespace shilm::memory;
using namespace shilm::io;


void* Allocator::allocate(size_t size, bool isarray) {
	bool reopen = !log;
	if (reopen) {
		log.open("log.txt", "a");
		log << "Allocating after destruction!!! " << endl;
	}
	char* p = (char*)malloc(size + sizeof(DWORD) + sizeof(size_t));
	if (!p) {
		throw bad_alloc();
	}
	Stacktrace<TRACE_SIZE> trace = Stacktrace<TRACE_SIZE>::getStacktrace(2);
	log << "Trace hash is " << trace.hash;
	StacktraceInfo* info;
	if ((info = traces.get(trace.hash)) != nullptr) {
		log << " this trace already exists";
	} else {
		if ((info = traces.add(trace)) == nullptr) {
			// TODO cleanup
			overflow = true;
		}
	}
	log << ". It ";
	if (isarray) {
		log << "allocated " << size << " bytes of memory for array at " << (void*)p;
	} else {
		log << "allocated " << size << " bytes of memory at " << (void*)p;
	}
	log << endl;
	totalAllocated += size;
	totalObjects++;
	if (info) {
		info->allocatedMemory += size;
		info->allocatedObjects++;
	}
	log.flush();
	if (reopen) {
		log << "Closing again..." << endl;
		log << "Unallocated memory: " << totalAllocated << endl;
		log << "Total objects: " << totalObjects << endl;
		log.close();
	}
	*((DWORD*)p) = trace.hash;
	p += sizeof(DWORD);
	*((size_t*)p) = size;
	p += sizeof(size_t);
	return p;
}

void Allocator::release(void* p, bool isarray) {
	if (!p) return;
	char* data = ((char*)p) - sizeof(DWORD) - sizeof(size_t);
	int offset = 0;
	ULONG hash = *((DWORD*)(data + offset));
	offset += sizeof(DWORD);
	size_t size = *((size_t*)(data + offset));
	offset += sizeof(size_t);
	bool reopen = !log;
	if (reopen) {
		log.open("log.txt", "a");
		log << "Releasing after destruction!!! " << endl;
	}
	log << "Unallocating " << (isarray ? "array " : "") << p;
	log << ". It is actually " << (void*)data << ".";
	log << " The trace was " << hash << ".";
	log << " There were " << size << " bytes.";
	log << endl;
	StacktraceInfo* info = traces.get(hash);
	if (info) {
		info->allocatedMemory -= size;
		info->allocatedObjects--;
	}
	totalAllocated -= size;
	totalObjects--;
	log.flush();
	if (reopen) {
		log << "Closing again..." << endl;
		log << "Unallocated memory: " << totalAllocated << endl;
		log << "Total objects: " << totalObjects << endl;
		log.close();
	}
	free(data);
}
Allocator::~Allocator() {
	log << "Unallocated memory: " << totalAllocated << endl;
	log << "Total objects: " << totalObjects << endl;
	char names[TRACE_SIZE][256];
	for (const StacktraceInfo& info : traces) {
		if (info.allocatedObjects > 0) {
			log << info.allocatedObjects << " objects, ";
			log << info.allocatedMemory << " bytes were created at hash " << info.hash << ": ";
			info.getNames(names);
			for (unsigned int i = 0; i < info.size; i++) {
				log << names[i] << " ";
			}
			log << endl;
		}
	}
	if (overflow) {
		log << "Possibly more" << endl;
	}
	log.close();
}

inline Allocator::HashMap::Iterator& Allocator::HashMap::Iterator::operator++() {
	pos++;
	while (pos >= map->buckets[bucket].size && bucket < BUCKETS) {
		pos = 0;
		bucket++;
	}
	return *this;
}


#ifdef SHILM_ENABLE_ALLOCATOR

void* operator new(size_t size) {
    //std::cout << "new" << std::endl;
	return shilm::memory::Allocator::getDefault().allocate(size, false);
}

void operator delete(void* p) {
    /*std::cout << "delete" << std::endl;
    char names[8][256];
    shilm::memory::Stacktrace<8>::getStacktrace(0).getNames(names);
    for (int i = 0; i < 8; i++) {
        std::cout << names[i] << std::endl;
    }*/
	shilm::memory::Allocator::getDefault().release(p, false);
}

void* operator new[](size_t size) {
	return shilm::memory::Allocator::getDefault().allocate(size, true);
}

void operator delete[](void* p) {
	shilm::memory::Allocator::getDefault().release(p, true);
}

#endif
