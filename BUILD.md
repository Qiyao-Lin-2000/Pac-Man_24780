# Build Instructions

## Prerequisites
- CMake (version 3.10 or higher)
- C++ compiler (MinGW-w64 GCC or Visual Studio)
- OpenGL libraries

## Minimal Build

```powershell
# Create build directory
mkdir build
cd build

# Configure CMake (generator depends on your compiler)
cmake .. -G "MinGW Makefiles"

# Build the project
cmake --build . --target TheWanderingEarth --config Release

# Run the game
.\TheWanderingEarth.exe
```

**Note**: The generator (`-G "MinGW Makefiles"`) depends on the compiler you are using:
- **MinGW-w64**: Use `-G "MinGW Makefiles"`
- **Visual Studio**: Use `-G "Visual Studio 17 2022"` (or your VS version)
- **Other**: Check available generators with `cmake --help`

## Build Steps

### 1. First Time Setup

Open PowerShell or Command Prompt in the project root directory:

```powershell
# Create build directory
mkdir build
cd build

# Configure CMake (generator depends on your compiler)
cmake .. -G "MinGW Makefiles"

# Build the project
cmake --build . --target TheWanderingEarth --config Release
```

### 2. Subsequent Builds

After the first build, you only need to rebuild:

```powershell
cd build
cmake --build . --target TheWanderingEarth --config Release
.\TheWanderingEarth.exe
```

### 3. Run the Game

```powershell
.\TheWanderingEarth.exe
```

## Quick Build (One Command)

From the project root:

```powershell
cd build; cmake .. -G "MinGW Makefiles"; cmake --build . --target TheWanderingEarth --config Release
```

## Clean Build

If you encounter build issues, clean and rebuild:

```powershell
# From project root
Remove-Item -Recurse -Force build
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build . --target TheWanderingEarth --config Release
```

## Troubleshooting

### Error: "Cannot open output file TheWanderingEarth.exe: Permission denied"
- **Solution**: Close the running game executable, then rebuild

### Error: "CMake Error: Generator does not match"
- **Solution**: Delete `build/CMakeCache.txt` and `build/CMakeFiles/`, then run `cmake ..` again

### Error: "No CMAKE_C_COMPILER could be found"
- **Solution**: Install MinGW-w64 or Visual Studio with C++ support

### Error: Using wrong compiler (arm-none-eabi-gcc)
- **Problem**: CMake is finding ARM embedded compiler instead of x86_64 MinGW
- **Solution**: The CMakeLists.txt now automatically avoids ARM compilers. If you still have issues:
  ```powershell
  cd build
  Remove-Item -Recurse -Force CMakeFiles,CMakeCache.txt -ErrorAction SilentlyContinue
  cmake .. -G "MinGW Makefiles"
  ```

## Build Configurations

### Release Build (Default)
```powershell
cmake --build . --target TheWanderingEarth --config Release
```

### Debug Build
```powershell
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --target TheWanderingEarth --config Debug
```

