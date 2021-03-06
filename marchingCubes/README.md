# miniIsosurface/marchingCubes #

Marching Cubes is an algorithm that creates a polygonal mesh to approximate
an isosurface from a three-dimensional discrete scalar field. The discrete
scalar field is subdivided into uniform cubes. For each cube, the values at
each corner are compared to the isovalue to determine a possible
configuration of triangles. Connected together, the triangles from all
cubes will form the polygonal mesh.

Each vertex of a triangle is located on an edge of the cube. The location
is approximated by linearly interpoloating from the two endpoints of that
edge. Similarly, the normal value of the surface located at each vertex is
approximated by linerly interpolating from the normals located at the two
endpoints of the edge where the normals are equal to the gradient,
approximated by the difference method.

This implementation of Marching Cubes has several implementations. The
reference implementation is done without any parallelization. The openmp
implementation uses openMP.

# C++11 usage #

miniIsosurface/marchingCubes makes use of C++11 features. Namely
* `auto` type-specifier
* range based for-loops
* The `std::vector<T>::data` member function
* The `std::vector<T>::emplace_back` member function
* The `std::array<T, N>` class
* The `std::to_string` function

# Build Instructions #
1. Clone the repository
```
git clone git@gitlab.sandia.gov:visMiniapps/miniIsosurface.git
```
2. Create a build directory. mantevo-marching-mubes does not require an
   out-of-tree build, but it is cleaner.
```
mkdir miniIsosurface/mcBuild
cd miniIsosurface/mcBuild
```
3. Invoke CMake from your build directory, pointing to the
   miniIsosurface/marchingCubes source directory. The two flags,
   `BUILD_OPENMP` and `BUILD_MPI`, must be turned on to build all
   implementations under the CMake build system.
```
cmake /path/to/miniIsosurface/marchingCubes -DBUILD_OPENMP=On -DBUILD_MPI=On
```
4. Invoke GNU make from the build directory.
```
make
```

After compiling, the following executables will be created:
* `./serial/serial`
* `./openmp/openmp`
* `./openmpDupFree/openmpDupFree`
* `./mpi/mpi`
* `./openmpAndMpi/openmpAndMpi`
* `./tests/SameContentsCheck`

For all executables, the `input_file`, `output_file` and `isoval` flags
must be set. Upon running, each executable creates a yaml file describing
performance and output characteristics.

The contents of the yaml file is also printed to console. To specify the
yaml output file name, the flag is `yaml_output_file`.

Some executables have additional flags. To print out all flags for an
executable, use the `help` flag.

## Kokkos Build Instructions ##

The Kokkos implementation does not use CMake. See the Kokkos
[README](kokkos/README.md) for the Marching Cubes with Kokkos specific
build instrucions.


## License ##

miniIsosurface is distributed under the OSI-approved BSD 3-clause License.
See [LICENSE.txt](../LICENSE.txt) for details.

Copyright (c) 2017
National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
certain rights in this software.
