# Galaxy

Galaxy is a small C++23 Vulkan demonstration project made for a talk at UnFUG
Furtwangen. It renders a GPU-driven star field and simple galaxy simulation
using compute shaders written in Slang.

The program creates a Vulkan window, uploads randomized star data, advances the
star positions on the GPU, projects the stars into screen coordinates, and draws
the result into a storage image before copying it to the swapchain.

## Requirements

- A Linux system with a Vulkan-capable GPU and driver.
- Vulkan loader, headers, and validation layer support.
- A C++23-capable compiler.
- [xmake](https://xmake.io/) for building.
- `slangc` on `PATH` for compiling the `.slang` shaders to SPIR-V.
- System libraries used by the current build, including GLFW-related X11/Wayland
  dependencies, `libxml2`, `zlib`, and ICU.

The repository includes a Nix flake development shell with xmake, Clang, LLDB,
and the system libraries expected by the build.

## Quick Start

From the repository root:

```sh
xmake
./build/linux/x86_64/<debug/release>/galaxy
```

Replace `<debug/release>` with the mode you built. The default mode is `debug`.

Run the binary from the repository root. `xmake run` changes the working
directory to the binary directory, which prevents the program from finding the
compiled shader files because shader paths are relative to the repository root.

## Project Structure

- `src/main.cpp` starts the application and runs the `galaxy::Galaxy` loop.
- `src/galaxy.cpp` initializes the simulation resources and records the
  per-frame compute and presentation commands.
- `src/gfx.cpp` owns the Vulkan instance, device, swapchain, command buffer, and
  window setup.
- `src/galaxy/star_data.cpp` uploads star positions, tints, weights, screen
  coordinates, and velocities into GPU buffers.
- `include/` contains the public project headers and push constant definitions.
- `shaders/` contains the Slang compute shaders and their generated `.spirv`
  files.
- `third_party/glfwpp/` contains the bundled GLFW C++ wrapper headers used by
  the windowing code.

## Rendering and Simulation

The frame loop uses three compute passes:

1. `sim.slang` updates star positions with a simple pairwise gravitational
   calculation and ping-pongs between two position buffers.
2. `calculate_screen_coords.slang` projects the current world positions through
   the camera view-projection matrix into screen coordinates.
3. `draw.slang` shades a 640x480 storage image from the projected star data and
   star tints, then the image is copied to the swapchain image for presentation.

The current demo uses 2,048 stars. That value is hard-coded in both the C++ code
and shaders.

## Caveats

- The executable currently expects to be launched from the repository root
  because shader files are loaded through relative paths such as
  `./shaders/sim.slang.spirv`.
- The render target and dispatch sizes are fixed at 640x480 in the current
  implementation.
- The Vulkan setup is Linux-focused and explicitly enables Wayland/XCB surface
  extensions and the Khronos validation layer.
- The demo code favors clarity for a talk over production-level portability,
  configuration, and error recovery.

## Testing Status

There are no automated tests in this repository. The current validation path is
to build with xmake and run the demo manually from the repository root on a
machine with a working Vulkan stack.

## License

This project is licensed under the MIT License. See [LICENSE.md](LICENSE.md).
