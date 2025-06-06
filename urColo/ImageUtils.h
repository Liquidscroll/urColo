#pragma once
#include "Colour.h"
#include <string>
#include <vector>

namespace uc {
/// Load an image and extract dominant colours using a simple k-means.
std::vector<Colour> loadImageColours(const std::string& path);
}
