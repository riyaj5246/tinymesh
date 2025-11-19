TinyMesh
===

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/2fd9a7f621e44654ad8b81bc38138662)](https://www.codacy.com/manual/tatsy/tinymesh?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=tatsy/tinymesh&amp;utm_campaign=Badge_Grade)
![Windows CI](https://github.com/tatsy/tinymesh/workflows/Windows%20CI/badge.svg)
![MacOS CI](https://github.com/tatsy/tinymesh/workflows/MacOS%20CI/badge.svg)
![Ubuntu CI](https://github.com/tatsy/tinymesh/workflows/Ubuntu%20CI/badge.svg)
[![License: MPL 2.0](https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)

> TinyMesh is a light-weight mesh processing library in C/C++.

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/tatsy425)


Modules
---

Here is the list of modules and reference papers for that.

*   **Geometric properties**
    *   Principal curvatures [[Rusinkiewicz 2004]](https://ieeexplore.ieee.org/document/1335277)
    *   Heat kernel signatures [[Sun et al. 2009]](https://onlinelibrary.wiley.com/doi/10.1111/j.1467-8659.2009.01515.x)
*   **Smoothing**
    *   Laplacian smoothing
    *   Taubin smoothing [[Taubin 1995]](https://dl.acm.org/doi/10.1145/218380.218473)
    *   Implicit fairing [[Desbrun 1999]](https://dl.acm.org/doi/10.1145/311535.311576)
*   **Denoising**
    *   Normal Gaussian filter [[Ohtake et al. 2001]](https://www.semanticscholar.org/paper/Mesh-Smoothing-by-Adaptive-and-Anisotropic-Gaussian-Ohtake-Belyaev/19b431c843f4b37d2218e7efcd8f64b6ff589c1f)
    *   Normal bilateral filter [[Zheng et al. 2011]](https://ieeexplore.ieee.org/document/5674028)
    *   L0 mesh smoothing [[He and Schaefer 2013]](https://dl.acm.org/doi/10.1145/2461912.2461965)
*   **Remeshing**
    *   Uniform triangulation [[Hoppe 1996]](https://dl.acm.org/doi/10.1145/237170.237216)
*   **Simplification**
    *   Quadric error metrics (QEM) [[Garland and Heckbert 1997]](https://dl.acm.org/doi/10.1145/258734.258849)
*   **Hole filling**
    *   Min-area hole filling [[Barequet and Sharir 1995]](https://www.sciencedirect.com/science/article/pii/016783969400011G?via%3Dihub)
    *   Min-dihedral angle [[Liepa 2003]](http://diglib.eg.org/handle/10.2312/SGP.SGP03.200-206)
    *   Advancing front [[Zhao et al. 2007]](https://link.springer.com/article/10.1007/s00371-007-0167-y)

Install
---

The module is tested its compilation using the following compilers.

*   **Windows** - Visual Studio 2022 (Microsoft Windows Server 2022)
*   **MacOS** - Apple Clang 11.0 (MacOS 11.7)
*   **Linux** - LLVM Clang 11.0, GNU C Compiler 9.4.0 (Ubuntu 22.04 LTS)

### Branch Info

Successful Optimizations:
- all: Holds all working optimizations such as unordered map, JEmalloc, and compiler flag optimizations. 
- compiler_flags: Contains the compiler optimizations in the main CMake file.
- kmak/allocator: Contains the JEMalloc allocator switching. 
- unordered_map: Replaces an inefficient data structure with an unordered map

Attempted Optimizations:
- heap-optimization: Optimized simplify QEM by using an alternative heap construction method.
- prev: Changes to halfedge.h where we add a previous pointer instead of just traversing the list to get the item before.
- kmak/remesh-indices-randomization: This branch stores code for the remesh function's randomization indices vector where we replace a custom loop with library functions that do the same thing.

Note: There are other branches that contain other experimental work for you to explore as well.


### Library and examples (C++)

You can build a shared library and all the examples by `CMake` with the following commands. We ran and tested our implementation on the CS 598 APE VM.

```shell
git clone https://github.com/riyaj5246/tinymesh
cd tinymesh
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON ..
cmake --build . --config Release --parallel 2
```

Run examples
---

Before running the code, you must download JE Malloc. For unix/linux users, please run this:

```shell
sudo apt-get install libjemalloc-dev
```

For Windows users, please download release 5.3.0 from JEMalloc through [here](https://github.com/jemalloc/jemalloc/releases).

#### C++ 

Here are example runs you can make to run the renderer. Be sure to execute these in the root folder. There are also more models for you to play around with in data/models/. We used bunny.ply as our small test case and christmas_bear.ply as our larger test case.

```shell
time ./scripts/run_with_allocator.sh jemalloc -- ./build/bin/example_simplify data/models/bunny.ply
time ./scripts/run_with_allocator.sh jemalloc -- ./build/bin/example_denoise data/models/bunny.ply
time ./scripts/run_with_allocator.sh jemalloc -- ./build/bin/example_read_write data/models/bunny.ply
time ./scripts/run_with_allocator.sh jemalloc -- ./build/bin/example_remesh data/models/bunny.ply
time ./scripts/run_with_allocator.sh jemalloc -- ./build/bin/example_smooth data/models/bunny.ply
```

Note: These run commands above will only work in the all or kmak/allocator branch. If you are in any other branch, please run commands in this form instead.

```shell
time ./build/bin/example_simplify data/models/bunny.ply
time ./build/bin/example_denoise data/models/bunny.ply
time ./build/bin/example_read_write data/models/bunny.ply
time ./build/bin/example_remesh data/models/bunny.ply
time ./build/bin/example_smooth data/models/bunny.ply
```

Gallery
---

#### Remeshing

<table>
  <tr>
    <td width="30%">Input</td>
    <td width="30%">Remesh</td>
    <td width="30%">Remesh (bottom part)</td>
  </tr>
  <tr>
    <td width="30%"><img src="figures/bunny_before.png" width="100%"/></td>
    <td width="30%"><img src="figures/bunny_remesh_1.png" width="100%"/></td>
    <td width="30%"><img src="figures/bunny_remesh_2.png" width="100%"/></td>
  </tr>
</table>


#### Simplification

<table>
  <tr>
    <td width="30%">Input</td>
    <td width="30%">Simplify (50000 faces)</td>
    <td width="30%">Simplify (10000 faces)</td>
  </tr>
  <tr>
    <td width="30%"><img src="figures/dragon_before.png" width="100%"/></td>
    <td width="30%"><img src="figures/dragon_simplify_50000.png" width="100%"/></td>
    <td width="30%"><img src="figures/dragon_simplify_10000.png" width="100%"/></td>
  </tr>
</table>

#### Denoising (L0 mesh smoothing)

<table>
  <tr>
    <td width="30%">Original</td>
    <td width="30%">Noisy</td>
    <td width="30%">Denoise</td>
  </tr>
  <tr>
    <td width="30%"><img src="figures/fandisk_before.png" width="100%"/></td>
    <td width="30%"><img src="figures/fandisk_noise.png" width="100%"/></td>
    <td width="30%"><img src="figures/fandisk_denoise_l0.png" width="100%"/></td>
  </tr>
</table>

#### Hole filling

<table>
  <tr>
    <td width="30%">Original</td>
    <td width="30%">Hole filled (minimum dihedral angles)</td>
    <td width="30%">Hole filled (advancing front)</td>
  </tr>
  <tr>
    <td width="30%"><img src="figures/bunny_holes.png" width="100%"/></td>
    <td width="30%"><img src="figures/bunny_hole_fill_min_dihedral.png" width="100%"/></td>
    <td width="30%"><img src="figures/bunny_hole_fill_adv_front.png" width="100%"/></td>
  </tr>
</table>


Notice
---

The functions provided by this repo are not perfect and their process will fail for problematic meshes, e.g., with non-manifold faces. In such cases, you can fix the problem by repairing the mesh using [MeshFix](https://github.com/MarcoAttene/MeshFix-V2.1).

License
---

Mozilla Public License v2 (c) Tatsuya Yatagawa 2020-2021
