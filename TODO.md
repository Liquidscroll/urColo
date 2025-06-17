## TODO

### Documentation
- **Comments**
  - Add comments to every function and complicated sections of code. 
  - Explain platform-specific code paths such as `WndProc`, context setup and each `#ifdef _WIN32` section.
- **Project docs**
  - Update `README.md` with:
    - Windows & Linux build / run steps (PowerShell `build.ps1`, `run.ps1`, Visual Studio generator, tool-chain versions).
    - How to generate the Doxygen documentation.
  - Add a CONTRIBUTING section that describes code style, formatting, and doc-generation workflow.

### Code readability & style
- Run `clang-format` repo-wide; document how to invoke it.
- Expand inline comments where algorithms are non-obvious  
  (e.g. k-means in **`PaletteGenerator`**, contrast formulas).
- Clarify all magic numbers and formulas.
- Remove or implement placeholder files (`Gui/Window.h`, `Window.cpp`) and stray *“TODO: implement”* blocks.
- **Strip all existing Doxygen comments** (the GUI is not intended as a public API).
- Enforce consistent naming conventions and file headers.

### Cross-platform architecture
- **Window / ImGui refactor**
  - Centralise duplicated ImGui setup in helper functions.
  - Share window-management logic between Windows and Linux.
  - Document and clarify the lifetime of `_hDC`, `_hRC`, and GLFW window objects.
  - Ensure all #ifdef _WIN32 blocks have comments explaining why they exist.
  - Move all #pragma clang/GCC/warning to centeral header, then include this header where portable-file-dialogs.h appears.
    - Ensure all #pragma clang/GCC/warning have comments explaining why they exist.
- **Script parity**
  - Audit PowerShell and *nix shell scripts for feature parity; add usage comments.
- **Validation**
  - After refactor, verify the full build and run flow on both Windows and Linux.

### Testing & continuous integration
- Increase unit-test (doctest) coverage:
  - Serialization, model loading, highlight parsing.
  - Windows-specific dialogs and paths.
