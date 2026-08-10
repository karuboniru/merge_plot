# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
mkdir build
cmake -S . -B build
cmake --build build
```

Dependencies: ROOT framework, nlohmann_json (≥3.2.0), Boost (program_options). Uses C++23, builds with Ninja/CMake.

## Running the Tools

```bash
# Main histogram merging/plotting tool
./build/merge_plot input.json

# Other executables
./build/merge_stack input.json
./build/draw2d input.json
./build/drawall2d input.json
./build/root_divide file1.root file2.root output.root
./build/root_minus file1.root file2.root output.root
./build/plot_spline [options] plot_names
```

See `input.json.example` for the configuration format.

## Architecture

This is a ROOT-based scientific visualization toolkit for neutrino physics (comparing NuWro, GiBUU, GENIE simulation outputs).

**Main tool (`main.cxx` → `merge_plot`):** Reads a JSON config, loads histograms from multiple ROOT files, applies transformations (scaling, rebinning, normalization, shape normalization, cumulative), overlays them on a single canvas, and exports to EPS/PNG/PDF.

**Shared styling (`include/tools.h`):** Global ROOT style setup, axis/padding helpers, logarithmic binning utilities, and C++23 concept-based templates for generic histogram/graph handling. All executables include this header.

**Spline tools (`plot_spline*.cxx`):** Specialized for cross-section visualization — fit splines to graph data, support neutrino interaction channels (nu_e_C12, nu_mu_C12, etc.) and per-target scaling factors. CLI uses Boost program_options.

**Utility executables:** `root_divide`, `root_minus` operate directly on ROOT files (histogram arithmetic). `draw2d`, `drawall2d`, `2dproj`, `2d_renormalize` handle 2D histogram workflows. `merge_stack` stacks histograms.

Each `.cxx` file compiles to its own standalone executable (13 total, defined in `CMakeLists.txt`).

No automated test suite — correctness is verified by visual inspection of output plots.
