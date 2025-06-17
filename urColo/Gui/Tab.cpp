#include "Tab.h"
#include "../Gui.h"

namespace uc {
// Simple constructor storing the title and owning GuiManager pointer.
Tab::Tab(std::string title, GuiManager *manager)
    : _title(std::move(title)), _manager(manager) {}
} // namespace uc
