// urColo - window manager interface
#pragma once
#ifdef _WIN32
// Windows requires the Win32 headers for HWND and friends.
#include <windows.h>
#else
// Other platforms use GLFW for window and input management.
#include <GLFW/glfw3.h>
#endif
#include <filesystem>
#include <memory>
#include <string>
#include "../compiler_warnings.h"
UC_SUPPRESS_WARNINGS_BEGIN
#include <portable-file-dialogs.h> // third-party header
UC_SUPPRESS_WARNINGS_END

namespace uc {
class GuiManager;

class WindowManager {
  public:
#ifdef _WIN32
    // On Windows we pass the HWND so the OpenGL context can be created with
    // WGL.
    void init(GuiManager *gui, HWND hwnd,
              const char *glsl_version = "#version 130");
#else
    // Non-Windows platforms receive a GLFWwindow pointer and GLSL version.
    void init(GuiManager *gui, GLFWwindow *wind,
              const char *glsl_version = "#version 330 core");
#endif
    void newFrame();
    void render();
    void shutdown();

  private:
    GuiManager *_gui{};
#ifdef _WIN32
    // Native window handle passed in from main. Owned by the caller.
    HWND _hwnd{};
    // Device context created from _hwnd. Released in shutdown().
    HDC _hDC{};
    // OpenGL rendering context owned by WindowManager.
    HGLRC _hRC{};
#else
    // Non-owning pointer to the GLFW window created by main.
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
    // GLFW reports errors through a callback when not using Win32.
    static void GLFWErrorCallback(int error, const char *desc);
#endif
};
} // namespace uc
