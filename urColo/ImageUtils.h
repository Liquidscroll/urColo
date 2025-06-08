#pragma once
#include "Colour.h"
#include <string>
#include <vector>

namespace uc {

/// Raw image pixels in RGBA format along with colour conversions for algorithms.
struct ImageData {
    int width{0};
    int height{0};
    std::vector<unsigned char> rgba; ///< 4 * width * height bytes
    std::vector<Colour> colours;     ///< Colour representation of each pixel
};

/// Load an image file and return its pixel data and colour values.
ImageData loadImageData(const std::string &path);

/// Generate a random image of the given dimensions and return the pixels.
ImageData generateRandomImage(int width, int height);

/// Load an image and extract dominant colours using a simple k-means.
std::vector<Colour> loadImageColours(const std::string &path);

/// Generate a random image and return representative colours.
std::vector<Colour> generateRandomImageColours(int width, int height);
}
