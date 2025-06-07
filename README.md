# urColo

## Overview
urColo is a cross-platform C++23 GUI tool for generating **accessible color palettes** (e.g. for code editors or UI themes), focusing on readability and WCAG-compliant contrast ratios. It provides an intuitive interface built with Dear ImGui running on GLFW with OpenGL 3. The tool works on Linux (tested on Arch Linux with Wayland/Hyprland) and Windows 11. urColo lets you iteratively create palettes in the perceptually uniform OKLab color space, lock colors you like, and ensure foreground/background contrast meets WCAG AA/AAA standards.

## Features
- **Perceptual OKLab Color Space**
- **WCAG Contrast Checking**
- **Iterative Generation with Locks**
- **Per-Palette or Global Generation Modes**
- **JSON Export**
- **Adaptive Learning from User-Selected Palettes**
- **Cross-Platform GUI with Dear ImGui + GLFW**
- **Image-based Palette Seeding**

## Technologies Used
- C++23
- Dear ImGui (GUI)
- GLFW 3 (window/input)
- OpenGL 3 (rendering)
- OKLab color space
- nlohmann/json (JSON export)
- stb_image.h (image loading)

## Build Instructions

All third-party dependencies are included in the `external/` directory, so no
additional packages are required other than a C++23 toolchain and GLFW.

### Linux
```bash
sudo pacman -S cmake gcc glfw

git clone https://github.com/YourUsername/urColo.git
cd urColo
mkdir build && cd build
cmake ..
make
./palettegen
```

You can also use the convenience scripts from the repository root:

```bash
# Build using all available processors
./build.sh

# Build with a specific number of threads
./build.sh 8

# Build and run the program (threads optional)
./run.sh 8
```

### Windows (MSYS2 or Visual Studio)
```bash
# With MSYS2 and pacman:
pacman -S mingw-w64-x86_64-glfw
# Clone and build with CMake or open with Visual Studio CMake support
```

## Fonts
The UI uses the [Inter](https://fonts.google.com/specimen/Inter) typeface by
default with [Hack](https://sourcefoundry.org/hack/) available for code-style
widgets. If the fonts are missing, download them from:
- Hack: <https://raw.githubusercontent.com/source-foundry/Hack/master/build/ttf/Hack-Regular.ttf>
- Inter: <https://github.com/rsms/inter/raw/refs/heads/master/docs/font-files/InterVariable.ttf> 

## Usage
- **Spacebar / Generate**: Generate a new palette
- **Generation Mode**: Choose whether colours are generated per palette or
  across all palettes at once
- **Click to Lock**: Toggle lock on a color
- **Check Contrast**: UI shows WCAG compliance between fg/bg pairs
- **Export (S key or button)**: Save palette to JSON (`palettes.json` next to the executable by default)
- **Import**: Use the File menu to choose any JSON palette file to load
- **Load Image**: Choose an image file to seed the K-Means algorithm or select
  a random image size in the palette tab

## Colour Generation Algorithms
urColo provides multiple strategies for generating new colours:

- **Random Offset** – the default algorithm picks colours by adding small
  random offsets around the average of any locked swatches. With no locks the
  values are chosen uniformly.
- **K-Means++** – locked swatches act as fixed cluster centres and the
  remaining colours are initialised using the k‑means++ strategy before
  iterating. This tends to distribute colours evenly around the locked
  selections. The number of clustering iterations can be configured in the
  palette tab when this algorithm is active.
- **Gradient** and **Learned** are placeholders for future experiments.

## Contributing
Fork the repo, create a feature branch, and submit a PR. Contributions welcome!

## License
This project is licensed under the MIT License. See the LICENSE file for details.
