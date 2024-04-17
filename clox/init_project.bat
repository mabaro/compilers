@echo off

mkdir include
mkdir src
mkdir test
mkdir build
mkdir bin

set PROJECT=%1

echo cmake_minimum_required(VERSION 3.5) > CmakeLists.txt
echo project(%PROJECT%) >> CmakeLists.txt
echo #>> CmakeLists.txt
echo set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<0:>${PROJECT_SOURCE_DIR}/bin") # .exe and .dll >> CmakeLists.txt
echo set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "$<0:>${CMAKE_BINARY_DIR}/lib") # .so and .dylib >> CmakeLists.txt
echo set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "$<0:>${CMAKE_BINARY_DIR}/lib") # .lib and .a >> CmakeLists.txt
echo #>> CmakeLists.txt
echo add_executable(%PROJECT% src/main.cpp) >> CmakeLists.txt
echo #>> CmakeLists.txt
echo add_custom_command( >> CmakeLists.txt
echo    TARGET %PROJECT% POST_BUILD >> CmakeLists.txt
echo    COMMAND echo ***************** >> CmakeLists.txt
echo    COMMAND echo Running output...  >> CmakeLists.txt
echo    COMMAND echo ***************** >> CmakeLists.txt
echo    COMMAND %PROJECT%  >> CmakeLists.txt
echo    VERBATIM)  >> CmakeLists.txt


echo #include ^<cstdio^> > src\main.cpp
echo int main(int argc, char* argv) { >> src\main.cpp
echo   printf("Hello World!"); >> src\main.cpp
echo   return 0; >> src\main.cpp
echo } >> src\main.cpp
