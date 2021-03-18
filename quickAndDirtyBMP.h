#ifndef QUICK_DIRTY_H_INCLUDED
#define QUICK_DIRTY_H_INCLUDED

// this is just a very crude implementation of how to read a bmp file
// please do not do this at home, in this case take for granted a lot of things
// I used gimp 2.1 to export the image as bmp with and alpha channel

#include <cstdint>

struct Pixel {    // this struct will contain all the pixel data
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
};

uint8_t bmpRead(const char *path/*The path to the file to open*/, int &width/*will contain the width of the image*/, int &height/*will contain the height of the image*/, Pixel *&pPixelArray/*will point to an array of width * height length of pixel structs or nullptr if an error is found*/); // will read the image specified in path


#endif