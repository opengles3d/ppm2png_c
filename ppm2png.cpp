// ppm2png.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "libpng/png.h"

#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char** argv)
{

    const std::string sourceFilename = argv[1];
    const std::string destFilename = argv[2];

    std::ifstream fd(sourceFilename);
    if (!fd) {
        std::cerr << "Failed to read " << sourceFilename << std::endl;
        return 1;
    }

    // validate file header
    constexpr auto FileHeader = "P3";
    std::string line;
    fd >> line;
    if (line != FileHeader) {
        std::cerr << "Failed to read " << sourceFilename << ", bad header!\n";
        return 1;
    }

    int width = 0;
    int height = 0;
    fd >> width >> height;
    if (width <= 0 || height <= 0) {
        std::cerr << "Failed to read " << sourceFilename << ", bad dimensions!\n";
        return 1;
    }

    int bpp = 0;
    fd >> bpp;
    if (bpp <= 0 || bpp >= 256) {
        std::cerr << "Failed to read " << sourceFilename << ", bad color size!\n";
        return 1;
    }

    // prepare the data buffer
    constexpr auto BPP = 3;
    std::size_t imageSize = width * height * BPP;

    auto imageData = std::make_unique<unsigned char[]>(imageSize);

    std::size_t row = 0;
    std::string r, g, b;
    for (row = 0; row < imageSize && fd; row += BPP) {
        fd >> r >> g >> b;
        // Windows bitmaps are BGR
        imageData[row] = static_cast<unsigned char> (std::stoi(r));
        imageData[row + 1] = static_cast<unsigned char> (std::stoi(g));
        imageData[row + 2] = static_cast<unsigned char> (std::stoi(b));
    }


    FILE* fp2;
    int r0 = fopen_s(&fp2, destFilename.c_str(), "wb");
    if (!fp2 || r0) {
        // dealing with error
        std::cerr << "Failed to create " << destFilename << "\n";
        return 1;
    }

    // 1. Create png struct pointer
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        // dealing with error
        std::cerr << "Failed to create " << destFilename << "\n";
        return 1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        // dealing with error
        std::cerr << "Failed to create " << destFilename << "\n";
        return 1;
    }

    // 2. Set png info like width, height, bit depth and color type
    //    in this example, I assumed grayscale image. You can change image type easily
    int bit_depth = 8;

    png_init_io(png_ptr, fp2);
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, \
        PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, \
        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // 3. Convert 1d array to 2d array to be suitable for png struct
    //    I assumed the original array is 1d
    png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr, sizeof(png_bytepp) * height);
    for (int i = 0; i < height; i++) {
        row_pointers[i] = (png_bytep)png_malloc(png_ptr, width * BPP);
    }
    for (int hi = 0; hi < height; hi++) {
        for (int wi = 0; wi < width; wi++) {
            // imageData is source data that we convert to png
            row_pointers[hi][wi * 3 + 0] = imageData[(wi + width * hi) * 3 + 0];
            row_pointers[hi][wi * 3 + 1] = imageData[(wi + width * hi) * 3 + 1];
            row_pointers[hi][wi * 3 + 2] = imageData[(wi + width * hi) * 3 + 2];
        }
    }

    // 4. Write png file and clean up
    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 0;
}
