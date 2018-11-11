# Blood Farm

A game.

# Dependencies

Engine dependencies:

* OpenGL 4.1
* [EnTT](https://github.com/skypjack/entt) - Entity Component System (MIT License)
* [SDL2](http://libsdl.org/) - Windowing & Input (ZLIB License)
* [GLEW](http://glew.sourceforge.net/) - OpenGL extension library (Modified BSD & MIT Licenses)
* [GLM](https://glm.g-truc.net/0.9.8/index.html) - OpenGL Mathematics library (The Happy Bunny/Modified MIT License)
* [spdlog](https://github.com/gabime/spdlog) - Fast logging library (MIT License)
* [LuaJIT](http://luajit.org/luajit.html) - Lua bindings and JIT runtime (MIT License)
* [FastNoiseSIMD](https://github.com/Auburns/FastNoiseSIMD) - Library of various SIMD-optimised noise functions (MIT License, note: Simplex noise is patent-encumbered when used for texture generation)
* [PhysicsFS](http://icculus.org/physfs/) - Library for filesystem abstraction (zlib License)
* [PhysFS++](https://github.com/Ybalrid/physfs-hpp) - C++ wrapper for PhysicsFS (zlib License + [notes](https://github.com/Ybalrid/physfs-hpp/blob/master/LICENSE.txt))
* [Catch2](https://github.com/catchorg/Catch2) - C++ testing library (Boost Software License)
* [cpptoml](https://github.com/skystrife/cpptoml) - TOML data file loader (MIT License)
* [dynamiclinker](https://github.com/Marqin/dynamicLinker) - portable dynamic library loader (MIT License)
* [stb_image](https://github.com/nothings/stb) - Image loader (Public domain)
* [semimap](https://github.com/hogliux/semimap) - A semi compile-/run-time associative map container (MIT License)
* [Bullet Physics](https://github.com/bulletphysics/bullet3) - Physics engine (zlib license)

Tooling dependencies:

* [Assimp](http://assimp.org/) - Asset importer (BSD License)

All of the dependencies, except for SDL2, GLEW and OpenGL come packaged as git submodules within this repository, in the `deps` directory.

# Building

Make sure you have SDL2 and GLEW installed and clone the BloodFarm repository:

```
git clone git@github.com:danielytics/BloodFarm.git
cd BloodFarm
git submodule update --init
``` 

You then need to compile and install Bullet:

```
cd deps/bullet3/build3
cmake ..
make install
cd ../../..
```

Finally, build this repository:

```
cd build
cmake ..
make
```

You can now run the `BloodFarm` binary from the project root.

Note that it will fail on startup if the data files are not found in the `data` directory or `game.data` file. You can override this to load the sample game:

```
./BloodFarm --load sample
```

You can find the sample game files in `sample/`:
* `sample/sample.toml` contains the sample game configuration, used to bootstrap the game.

# License

The Blood Farm game source is released under the terms of the [MIT License](https://github.com/danielytics/BloodFarm/blob/master/LICENSE).

The game data files are not included under this license and you must have a valid license in order to run the complete BloodFarm game. The sample game (found in the `sample/`) directory is released under the terms of the MIT License and can be used as an example of how the engine works and a basis for your own games.