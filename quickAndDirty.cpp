#include "quickAndDirtyBMP.h"
#include <fstream>
#include <iostream>

uint8_t bmpRead(const char *path /*The path to the file to open*/, int &width /*will contain the width of the image*/, int &height /*will contain the height of the image*/,
                Pixel *&pPixelArray /*will point to an array of width * height length of pixel structs or nullptr if an error is found*/) {
    std::fstream imageFile(path, std::ios::in | std::ios::binary);    // I open the file in binary input
    uint32_t pixelOffset;                                             // the offsets to the beginning of pixel data
    imageFile.seekg(10);                                              // I jump 10 bytes
    imageFile.read((char *) &pixelOffset, 4);                         // read the offset
    imageFile.seekg(18);                                              // jump to 18 bytes
    imageFile.read((char *) &width, 4);
    imageFile.read((char *) &height, 4);
    std::cout << "The width is " << width << " and the height is " << height << std::endl;
    uint16_t bitsPerPixel;                        // how many bits are used to store one pixel
    imageFile.seekg(2, std::ios_base::cur);       // jump 2 bytes ahead
    imageFile.read((char *) &bitsPerPixel, 2);    // read the bits per pixel
    std::cout << "Bits per pixels " << bitsPerPixel << std::endl;

    imageFile.seekg(pixelOffset);    // go to the beginning of the image
    pPixelArray = new Pixel[width * height];
    for (int row = height - 1; row >= 0; row--) {                // I start from the last row as bitmap stores the row bottom to top
        int rowSize = ((bitsPerPixel * width + 31) / 32) * 4;    // calculate how many bytes per line are used, as that may include some padding bytes
        for (int column = 0; column < width; column++) {         // read each pixel in reverse order cause bitmap è simpatico
            imageFile.read((char *) &pPixelArray[row * width + column].blue, 1);
            imageFile.read((char *) &pPixelArray[row * width + column].green, 1);
            imageFile.read((char *) &pPixelArray[row * width + column].red, 1);
            if (bitsPerPixel == 32)
                imageFile.read((char *) &pPixelArray[row * width + column].alpha, 1);
            else
                pPixelArray[row * width + column].alpha = 255;
        }
        int paddingToSkip = rowSize - bitsPerPixel / 8 * width;
        imageFile.seekg(paddingToSkip, std::ios_base::cur);    // skip the padding bytes
    }
    imageFile.close();
    return 0;    // done quick and dirty as promised
}
