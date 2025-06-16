#pragma once
#ifdef _WIN32
#include <windows.h>
#else
#include <GLFW/glfw3.h>
#endif
#include <filesystem>
#include <memory>
#include <string>
#if defined(__clang__) // ──── Clang ────────────────────────────────
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wshadow"

#elif defined(__GNUC__) // ──── GCC / MinGW ─────────────────────────
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wshadow"

#elif defined(_MSC_VER) // ──── MSVC ─────────────────────────────────
// C4668: 'identifier' is not defined as a preprocessor macro
// C4456/4457/4458: declaration of 'x' hides previous local
#pragma warning(push)
#pragma warning(disable : 4668 4456 4457 4458)
#endif

#include <portable-file-dialogs.h> //  third-party header

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace uc {
class GuiManager;

class WindowManager {
  public:
#ifdef _WIN32
    void init(GuiManager *gui, HWND hwnd,
              const char *glsl_version = "#version 130");
#else
    void init(GuiManager *gui, GLFWwindow *wind,
              const char *glsl_version = "#version 330 core");
#endif
    void newFrame();
    void render();
    void shutdown();

  private:
    GuiManager *_gui{};
#ifdef _WIN32
    HWND _hwnd{};
    HDC _hDC{};
    HGLRC _hRC{};
#else
    GLFWwindow *_window{};
#endif
    bool _savePopup{false};
    std::string _lastSavePath;
    std::string _palettePath{"palettes.json"};
    std::unique_ptr<pfd::open_file> _loadDialog;
    std::unique_ptr<pfd::save_file> _paletteSaveDialog;
    std::unique_ptr<pfd::save_file> _modelSaveDialog;
    std::unique_ptr<pfd::open_file> _modelLoadDialog;

    void applyStyle();
    void saveModel(const std::filesystem::path &path);
    void loadModel(const std::filesystem::path &path);
#ifndef _WIN32
    static void GLFWErrorCallback(int error, const char *desc);
#endif
};
} // namespace uc
