## TODO

1. **Extend learned algorithm**
   - Persist model state to disk and retrain from palettes the user marks as good.
   - Add menu options to load and save the model JSON file.

2. **Persist user settings**
   - Remember window size, generation mode, last open/save paths and highlight selections.
   - Write a simple `settings.json` in the executable directory on exit and load it on start up.

3. **Optimise colour conversions**
   - Profile `Colour` conversions to and from LAB and cache results for repeated calls.
   - Avoid unnecessary conversions when generating palettes.

4. **Fix image loading bug**
   - Loading an image via the File menu freezes the UI. Investigate the blocking call in `pfd::open_file` and move it off the main thread.

5. **Fix KMeans generation bug**
   - When using KMeans, pressing generate twice without locking a swatch should produce a new palette.
   - Currently the previous result is reused; clear the internal image cache before each run.

6. **Add palette removal button**
   - Provide a small 'X' next to each palette name to delete it.
   - Ensure at least one palette always remains to avoid crashes.

7. **Tidy foreground/background controls**
   - Align the fg/bg check boxes horizontally beside the lock button.
   - Shorten labels from "foreground"/"background" to "fg"/"bg" for compact layout.
