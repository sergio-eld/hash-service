# Hashing service
A small asynchronous TCP server providing a hashing functionality.

## Description
This is a sample project based on a test assignment. The server expects lines of ASCII characters terminated by an `'\n'` 
(single byte of value `10`) and for each line responds with its sha256-encoded hash in a HEX format, also terminated by
`'\n'`. For details refer to [specification.md](specification.md).  

The project is intended to demonstrate the full development lifecycle alongside with:
- building a cross-platform application (including cross-compiling)
- unit and functional testing of a system.
- deployment
- automated CI/CD using GitHub actions.

The project may and is expected to be used as a reference for solving recurring problems.

## Building
According to the specification, the target system is Ubuntu 20.04. Hence, all the required dependencies should be
installed on your machine (currently: using `apt` or `pacman` is building with msys2). 
List of the dependencies can be found in [Dockerfile.dev](Dockerfile.dev). Package versions should be compatible with 
those available on Ubuntu 20.04 (*TODO: specify versions explicitly*).    
Scripts for cross-platform package managers like Conan are not yet configured (*TODO*).  
The project can be built on your local machine or using the [Dockerfile.dev](Dockerfile.dev) image.

### CMake  

Options:  
- `BUILD_TYPE [Release|Debug|RelWithDebInfo|MinSizeRel]` build configuration. `Debug` by default.
- `ALL_WARNINGS [ON|OFF]` maximum warnings level. `ON` by default.
- `CLEAN_BUILD [ON|OFF]` treat all warnings as errors. `ON` by default
- `BUILD_TESTS [ON|OFF]` enable tests. Will require `GTest` and `Pytest`. `ON` by default. 
- `UNIT_TESTS [ON|OFF]` enable unit tests. Will require `GTest`. Depends on `BUILD_TESTS`. `ON` by default.
- `FUNCTIONAL_TESTS [ON|OFF]` enable functional tests. Will require `Pytest`. Depends on `BUILD_TESTS`. `ON` by default.

Command:
```
> mkdir build
> cd build
> cmake .. -G<GENERATOR> -DCMAKE_BUILD_TYPE=[Release|Debug|RelWithDebInfo|MinSizeRel] 
[-D[OPTION]...] 
[-DCMAKE_INSTALL_PREFIX=<path/to/output/packages>]
> cmake --build . 
> ctest .
> cmake --install .
```
Testing and installation is optional. 

### Docker
This project provides a [Dockerfile.dev](Dockerfile.dev) to build a Docker image to be used for building and 
debugging purposes.  
Building: `docker build -f ./Dockerfile.dev -t hash-service-dev .`  
Running: 
```
docker container run --rm -it -v $(pwd):/app:ro -v $(pwd) /build:/app/build hash-service-dev bash
```
This command will run the container, mapping the source directory in `read-only` mode and `./build` in `write` mode 
(to output the built files).  
When connected to the container's shell, build commands for CMake can be executed.  
The [Dockerfile.dev](Dockerfile.dev) also contains a gdbserver.

### Cross-compilation
(*TODO*)

## Deployment
CPack is used for generating installation packages.
(*TODO: Ubuntu, Windows*)

## TCP Server
### Installation
(*TODO*)

### Running
Hashing server can be run using the following command:
```
(TODO)
```
The server handles termination via `Ctrl + C`.

## CI 
Currently, a `.yml` file for GitHub actions is implemented in `.github/workflows`.  
The pipeline supports:
- Building
- Running tests (TODO: functional, i.e. using docker-compose)
- Deployment using CPack
