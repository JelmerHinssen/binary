#include <gtest/gtest.h>
#include "bitmap.h"

using namespace shilm::image;

TEST(Image, bitmapNoCompression) {
    Bitmap m = Bitmap::loadBitmap("test/img/1d.bmp");
    ASSERT_EQ(m.getWidth(), 16);
    ASSERT_EQ(m.getHeight(), 16);
    EXPECT_EQ(m.getPixel(0, 0), 0xfff7ffff);
    EXPECT_EQ(m.getPixel(1, 0), 0xff69ceff);
    EXPECT_EQ(m.getPixel(0, 15), 0xff5bc2ee);
}