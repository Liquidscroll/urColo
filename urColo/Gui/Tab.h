#pragma once

#include <string>

namespace uc {

struct GuiManager;

class Tab {

  public:
    Tab(std::string title, GuiManager *manager);
    virtual ~Tab() = default;

    virtual void drawContent() = 0;

    const std::string &getTitle() const { return _title; }

  protected:
    std::string _title;
    GuiManager *_manager;
};
} // namespace uc
