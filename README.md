### Instant-Meshes-Core

This is a headless C++ library refactored from the
[instant-meshes](https://github.com/wjakob/instant-meshes) repo. It provides core functionalities
described in the Siggraph paper "[Instant Field-Aligned
Meshes](https://igl.ethz.ch/projects/instant-meshes/)"[^1] without any graphical user interface
(GUI).

### Changes

- Removed all GUI-related code and dependencies
- Added namespace around all code to avoid name clashes
- Replace tbb with OneTBB v2021.12.0
- Add C++20 support


[^1]: Jakob, Wenzel, et al. "Instant field-aligned meshes." ACM transactions on graphics (TOG) 34.6 (2015): 1-15.
