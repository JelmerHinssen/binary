#include <gtest/gtest.h>
#include "bitmap.h"

using namespace shilm::image;

TEST(Image, bitmapNoCompression) {
    Bitmap m = Bitmap::loadBitmap("test/img/1d.bmp");
    ASSERT_EQ(m.getWidth(), 16);
    ASSERT_EQ(m.getHeight(), 16);
    EXPECT_EQ(m.getPixel(0, 0).argb(), 0xfff7ffff);
    EXPECT_EQ(m.getPixel(1, 0).argb(), 0xff69ceff);
    EXPECT_EQ(m.getPixel(0, 15).argb(), 0xff5bc2ee);
}

TEST(Image, pngA) {
    Bitmap m = Bitmap::createFromPNG("test/img/1a.png");
    ASSERT_EQ(m.getWidth(), 16);
    ASSERT_EQ(m.getHeight(), 16);
    EXPECT_EQ(m.getPixel(0, 0).argb(), 0xfff7ffff);
    EXPECT_EQ(m.getPixel(1, 0).argb(), 0xff69ceff);
    EXPECT_EQ(m.getPixel(0, 15).argb(), 0xff5bc2ee);
}

TEST(Image, pngB) {
    Bitmap m = Bitmap::createFromPNG("test/img/1c.png");
    ASSERT_EQ(m.getWidth(), 16);
    ASSERT_EQ(m.getHeight(), 16);
    EXPECT_EQ(m.getPixel(0, 0).argb(), 0xfff7ffff);
    EXPECT_EQ(m.getPixel(1, 0).argb(), 0xff69ceff);
    EXPECT_EQ(m.getPixel(0, 15).argb(), 0xff5bc2ee);
}

TEST(Image, allSame) {
    Bitmap a = Bitmap::createFromPNG("test/img/1a.png");
    Bitmap c = Bitmap::createFromPNG("test/img/1c.png");
    Bitmap d = Bitmap::loadBitmap("test/img/1d.bmp");
    EXPECT_EQ(a, c);
    EXPECT_EQ(a, d);
    EXPECT_EQ(c, d);
}

TEST(Image, sameNote) {
    Bitmap a = Bitmap::createFromPNG("test/img/2a.png");
    Bitmap b = Bitmap::loadBitmap("test/img/2b.bmp");
    EXPECT_EQ(a, b);
}