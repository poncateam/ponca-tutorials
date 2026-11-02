# ponca-tutorials

This repository holds some code and tutorials to use [Ponca](https://poncateam.github.io/ponca/) for the most common operations such as normal estimation, point cloud to mesh or curvature. Tutorials are organized by categories (what they do) and by dependencies.

## Tutorial list

### MarchingCube (Surface reconstruction) 

* `tutorials/MarchingCube/igl/`: Usage of [libigl](https://libigl.github.io/) marching cube implementation to reconstruct a surface. 
* `tutorials/MarchingCube/cgal/`: Usage of [CGAL](https://www.cgal.org/) (>= 6.1, [3D Isosurfacing](https://doc.cgal.org/latest/Isosurfacing_3/index.html) package) marching cube implementation to reconstruct a surface.

### Curvature estimation

* `tutorials/Curvature`: Estimates of principal curvatures and their direction with plain ponca. 

## Building the tutorials

The tutorials are written in C++ and need to be built before running. Only [CMake](https://cmake.org/) and a C++ compiler are required; every other dependency (including Ponca) will be downloaded and linked automatically.

### Building all tutorials (command line)

Within the root directory, perform the following instructions in order to build all examples:

```bash
cmake -B build .
cmake --build build --parallel
```

Executables will be placed within the `build` directory with the same structure as the projects (e.g., `tutorials/Mesh/igl/` for igl-based marching cube).

### Building a single tutorial

Tutorials are conceived as standalone projects. They can therefore be built independently. The steps described above are also valid within any subdirectory containing code (e.g., `tutorials/MarchingCube/igl/`). It is also possible to extract the few files into another directory if you prefer.

## Running the tutorials

Each tutorial can be used as a command line interface to the Ponca library. Options may differ depending on the tutorial; use `executable -h` to see a list of available options.

### Common options

A few options common to every tutorial are listed here: 

* `-h,--help`: Print details about every options supported by a tutorial. 
* `-i,--input`: Input point cloud. The accepted input format is PLY, where points are specified through "x,y,z" fields for positions and "nx,ny,nz" for normal data. 
* `-m,--method`: Which method to run. 
* `-o,--output`: Output point cloud. PLY is the preferred output format, but depending on the tutorial some more formats may be available. If found, the first occurrence of "{method}" can be used in this path in order to identify outputs based on the method. 
* `-s,--scale`: The scale at which analysis should be performed. When left empty, the code will try to choose a "good" default value.

## Helper scripts 

The script folder provides a few helpers to get you started with some results. To use them, first install the necessary dependencies by running:

```bash 
pip install -r requirements.txt
```

You may want to use a separate virtual environment, such as [venv](https://docs.python.org/3/library/venv.html) or [conda](https://anaconda.org).

### Download data

To get started, we provide a few scanned point clouds from the 3D-ARD project. You can download them using the download_data.py script:

```bash
python scripts/download_data.py [optional_data_location]
```

By default, the files are saved in the `data/` folder. Note that this script does a little more than simply download the files: when normals are not provided in a file, it also tries to compute them, so that most methods can work with the data.

### Viewing data

We also provide a minimal viewer built on the [polyscope](https://polyscope.run/py/) library. 

```bash
# To display data as a point cloud
python scripts/viewer.py cloud ref.ply [computed_files...]
# To display data as a mesh (falls back to a point cloud if no triangulation data is found) 
python scripts/viewer.py mesh ref.ply [computed_files...]
```