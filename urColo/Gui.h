#pragma once
#include "Colour.h"
#include <GLFW/glfw3.h>
#include <vector>

namespace uc {
struct GuiManager {
    void init(GLFWwindow *wind, const char *glsl_version = "#version 330 core");
    void newFrame();
    void render();
    void shutdown();

  private:
    std::vector<uc::Palette> _palettes;
    void drawPalettes();
    void drawPalette(uc::Palette &palette, int pal_idx);
    void drawSwatch(uc::Swatch &sw, int pal_idx, int idx, float swatch_px);
    GLFWwindow *_window{};

    static void GLFWErrorCallback(int error, const char *desc);
};
} // namespace uc
