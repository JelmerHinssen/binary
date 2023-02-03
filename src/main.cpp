#include "bitmap.h"
#include <iostream>
#include <fstream>
#include "filewriter.h"
#include "filereader.h"
#include "memory.h"
#include <signal.h>

using namespace std;
using namespace shilm::io;
using namespace shilm::image;

void sig(int sign) {
    cerr << "Signal: " << sign << endl;
    char names[16][256] ={{0}};
    shilm::memory::Stacktrace<16>::getStacktrace().getNames(names);
    for (int i = 0; i < 16; i++) {
        cout << names[i] << endl;
    }
    //signal(sign, SIG_DFL);
    //raise(sign);
    exit(1);
}

int main() {
    // Bitmap bmp = Bitmap::createFromPNG("backgroundmain.png");
    // cout << "Read" << endl;
    // ofstream of("backgroundmain.bmp", ios::binary);
    // BinaryStreamWriter bof(of, BinaryStreamWriter::LITTLE_ENDIAN, BinaryStreamReader::LEAST_SIGNIFICANT_BIT_FIRST_FLIPPED);
    // bof << bmp;
    signal(SIGSEGV, sig);
    shilm::memory::initStacktrace();
    int* a = new int;
    a = nullptr;
    *a = 6;
    // of.close();
}


