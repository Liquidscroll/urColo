#include "Tab.h"
#include "../Gui.h"

namespace uc {
Tab::Tab(std::string title, GuiManager *manager)
    : _title(std::move(title)), _manager(manager) {}
} // namespace uc
