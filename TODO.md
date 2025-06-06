## TODO

1. **Expose palette tags**
   - `_tags` field exists in `Palette` but has no UI.
   - Allow adding and editing tags to organise palettes.

2. **Finish gradient algorithm**
   - Replace placeholder implementation with actual gradient-based generation.

3. **Extend learned algorithm**
   - Persist model state to disk and retrain from saved or user-accepted palettes.

4. **Persist user settings**
   - Remember window size, generation mode and last file paths.

5. **Reorder palettes**
   - Support drag-and-drop columns to reorder palette tabs.

6. **Optimise colour conversions**
   - Profile `Colour` conversions and cache repeated calculations.

7. **Optimise UX**
   - Currently the UI hangs when a long running generation occurs. Make palette generation occur on a background thread, so that the GUI can remain responsive.
