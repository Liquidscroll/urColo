#include <GLFW/glfw3.h>
#include <print>

#include "Gui.h"
#include "Logger.h"

int main(int, char **) {

    // start logger
    auto logger = uc::Logger();

    if (!glfwInit()) {
        std::print(stderr, "[ERROR]: Failed to initialise GLFW\n");
        return 1;
    }

    const char *glsl_version = "#version 330 core";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    GLFWwindow *wind =
        glfwCreateWindow(1280, 720, "urColo - Hello ImGUI", NULL, NULL);

    if (wind == NULL) {
        std::print(stderr, "[ERROR]: Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    uc::GuiManager gui;
    gui.init(wind, glsl_version);

    uc::Colour blk, wht;
    std::vector<uc::Colour> palette = {blk.fromSRGB(0, 0, 0, 1),
                                       wht.fromSRGB(255, 255, 255, 1)};
    std::vector<bool> locked = {false};

    while (!glfwWindowShouldClose(wind)) {
        glfwPollEvents();
        gui.newFrame();
        gui.render();
    }
    gui.shutdown();

    glfwDestroyWindow(wind);
    glfwTerminate();

    logger.shutdown();

    return 0;
}
