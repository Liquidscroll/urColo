#pragma once
#include "Colour.h"
#include "PaletteGenerator.h"
#include <GLFW/glfw3.h>
#include <random>
#include <vector>

namespace uc {
/// Manages all ImGui interactions for the application.  This front end
/// initialises ImGui, drives the frame lifecycle and draws palette widgets
/// to the screen.
struct GuiManager {
    /// Set up the ImGui context for the given window.  Optionally provide the
    /// GLSL version string used by ImGui's OpenGL backend.
    void init(GLFWwindow *wind, const char *glsl_version = "#version 330 core");
    /// Start a new ImGui frame.  Call once per iteration of the render loop.
    void newFrame();
    /// Render the current ImGui draw data to the attached window and swap
    /// buffers.
    void render();
    /// Tear down ImGui and associated backends.
    void shutdown();

  private:
    PaletteGenerator _generator;
    std::vector<uc::Palette> _palettes;

    struct DragPayload {
        int pal_idx;
        int swatch_idx;
    };

    struct PendingMove {
        int from_pal;
        int from_idx;
        int to_pal;
        int to_idx;
    };
    std::vector<PendingMove> _pendingMoves;

    void applyPendingMoves();

    void startGeneration();
    /// Draw the palette table containing each palette.
    void drawPalettes();
    /// Draw an individual palette column.
    /// @param palette palette to display
    /// @param pal_idx index of the palette in the table
    void drawPalette(uc::Palette &palette, int pal_idx);
    /// Draw a single colour swatch.
    /// @param sw swatch data being drawn
    /// @param pal_idx owning palette index
    /// @param idx index of the swatch within the palette
    /// @param swatch_px width/height in pixels of the drawn swatch
    void drawSwatch(uc::Swatch &sw, int pal_idx, int idx, float swatch_px);
    GLFWwindow *_window{};

    /// Apply the custom ImGui style and load fonts.
    void applyStyle();

    /// Handler for GLFW error callbacks.
    static void GLFWErrorCallback(int error, const char *desc);
};
} // namespace uc
