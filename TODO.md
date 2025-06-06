## TODO

1. **Persist user settings**
   - Store window dimensions, generation mode, recently used file paths and
     highlight selections in a small `settings.json` next to the executable.
   - Load the file on startup and apply the values to initialise the GUI.
   - Write the file on shutdown when any setting has changed.
   - Use `nlohmann::json` for serialization and create a `Settings` struct in
     `Gui.h` to hold the fields.

2. **Fix image loading stutter**
   - Loading an image from the *File* menu currently blocks the main thread
     while pixels are read and converted to OKLab.
   - Spawn a worker thread in `GuiManager` that calls `loadImageColours()` and
     stores the result in `_imageColours` when complete.
   - Display a small progress indicator while the thread runs and disable the
     *Start Generation* button until the image is ready.

3. **Maintain highlight references**
   - Deleting or reordering swatches/palettes can leave highlight group indices
     pointing at the wrong entry, causing incorrect colours in the preview.
   - Update `_globalFgSwatch`, `_globalBgSwatch` and every `HighlightGroup`
     entry inside `applyPendingMoves()` to account for moved or removed
     swatches and palettes.
   - Add regression tests covering swatch deletion and palette moves.

4. **Undo/Redo support**
   - Record palette and swatch edits in a command stack so users can revert
     accidental changes.
   - Implement `Ctrl+Z`/`Ctrl+Y` shortcuts and corresponding menu items.
   - Commands should capture the full palette state to keep implementation
     simple.

5. **Switch to cylindrical OKLab calculations**
   - Modify `Colour` and `PaletteGenerator` so all interpolation and distance
     computations happen in cylindrical OKLab (L, C, h) instead of Cartesian
     LAB.
   - Provide conversion helpers between Cartesian and cylindrical forms and
     update existing algorithms accordingly.
