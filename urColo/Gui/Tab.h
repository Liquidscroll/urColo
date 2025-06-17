// urColo - GUI tab base class
#pragma once

#include <string>

namespace uc {

// Base class for each tab displayed in the GUI.

struct GuiManager;

class Tab {

  public:
    // Construct a tab with the given title.
    Tab(std::string title, GuiManager *manager);
    virtual ~Tab() = default;

    // Draw this tab's UI elements.
    virtual void drawContent() = 0;

    // Retrieve the display title of this tab.
    const std::string &getTitle() const { return _title; }

  protected:
    std::string _title;
    GuiManager *_manager;
};
} // namespace uc
