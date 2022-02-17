# 4asteroids

## Instructions

Clone this repository:
```
git clone --recurse-submodules https://github.com/tomasorti/4asteroids
cd 4asteroids\SFML
git checkout 2.5.x && git br -v
```

Generate Visual Studio Solution with CMake.  
For this, you should have the environment prepared.
```
mkdir build
cd build
cmake ..
start SFML.sln
```

From Visual Studio, build the project.  
The .lib & .dll files should be generated

Now you can go to the root directory and use `.\build.bat` to compile or Ctrl+Shift+B from VScode.