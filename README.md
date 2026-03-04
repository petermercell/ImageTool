# ImageTool

A Nuke NDK plugin that computes live image statistics — similar to CurveTool but without the need to render. It creates a solid colour image representing the Average, High, Median, or Low value of the input (like the Stat node in Shake).

**Original author:** Gerard Benjamin Pierre
([IMDb](http://www.imdb.com/name/nm0682633/))

**Original Nukepedia page:** [ImageTool on Nukepedia](https://www.nukepedia.com/tools/plugins/colour/imagetool/)

## Description

ImageTool analyses the input image and outputs a constant colour based on the selected statistic mode. Unlike CurveTool, the result is computed live — no pre-render pass required. This makes it handy for quick colour sampling, normalisation reference, or feeding into expressions and grade nodes on the fly.

## Building from Source

### Requirements

- CMake 3.16+
- Nuke NDK (matching your target Nuke version)
- C++17 compatible compiler

### Build

```bash
mkdir build && cd build
cmake .. -DNUKE_INSTALL_PATH=/path/to/Nuke17.0
cmake --build . --config Release
```

### Install

Copy the resulting `ImageTool.so` (Linux), `ImageTool.dylib` (macOS), or `ImageTool.dll` (Windows) to your `~/.nuke` directory or any path on `NUKE_PATH`.

## Usage

1. Add an **ImageTool** node after your plate.
2. Select a **mode** — Average, High, Median, or Low.
3. The output is a solid colour image matching the computed statistic of the input.

## License

This project is licensed under the **BSD 3-Clause License** — see the [LICENSE](LICENSE) file for details.

Copyright (c) 2011, Gerard Benjamin Pierre. All rights reserved.

## Credits

- **Gerard Benjamin Pierre** — original author and all plugin logic.
- **Peter Mercell** — Nuke 17 compilation and distribution.
