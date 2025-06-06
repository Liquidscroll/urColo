## TODO

1. **Rename palettes**
   - Add an `ImGui::InputText` field next to each palette name.
   - Store updates back into `Palette::_name`.
   - Persist renamed palettes when saving to JSON.

2. **File menu**
   - Create a menu bar with `File` dropdown.
   - Provide actions: `Save to JSON`, `Load from JSON`, and `Quit`.
   - Use existing `to_json`/`from_json` helpers for serialization.

3. **Colour generation algorithms**
   - Research alternative algorithms (e.g. k‑means, gradient based, learned models).
   - Allow user to choose algorithm in UI settings.

4. **Contrast test report**
   - Add a button to run WCAG contrast checks.
   - Combine all swatches marked as foreground with those marked as background.
   - Display pass/fail results visually in a popup or table.

5. **Bulk palette actions**
   - Add controls to set every swatch in a palette to foreground/background.
   - Provide "lock all" and "unlock all" options.

6. **Batch generation options**
   - Let the user choose to generate all palettes together or individually.
   - Maintain current behaviour for per‑palette generation as an option.

7. **Expose palette tags**
   - `_tags` field exists in `Palette` but has no UI.
   - Allow adding and editing tags to organise palettes.

8. **Rename swatches**
   - Allow editing of each `Swatch::_name` via `ImGui::InputText`.

9. **Implement `Model` module**
   - `Model.cpp` and `Model.h` should provide a self-learning palette model.
   - Feed known "good" palettes and accepted user palettes into the model.
   - Use the model to refine colour generation over time.

10. **Testing improvements**
    - Add unit tests for colour conversions and palette generation.
    - Cover JSON save/load operations.

11. **Image import for palette generation**
    - Integrate `stb_image` to load images.
    - Extract dominant colours to seed palette generation.
