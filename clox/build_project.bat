set BUILD_PATH=build
set SOURCE_PATH=.
cmake -B %BUILD_PATH% -S %SOURCE_PATH%
cmake --build %BUILD_PATH%