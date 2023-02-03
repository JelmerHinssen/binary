#include "bitmap.h"
#include <iostream>
#include <fstream>
#include "filewriter.h"
#include "filereader.h"

using namespace std;
using namespace shilm::io;
using namespace shilm::image;

int main() {
    Bitmap bmp = Bitmap::createFromPNG("backgroundmain.png");
    cout << "Read" << endl;
    ofstream of("backgroundmain.bmp", ios::binary);
    BinaryStreamWriter bof(of, BinaryStreamWriter::LITTLE_ENDIAN, BinaryStreamReader::LEAST_SIGNIFICANT_BIT_FIRST_FLIPPED);
    bof << bmp;
    of.close();
}


