@echo off
setlocal
echo ====================================================================
echo   Circular Array Beamforming & DOA Estimation
echo ====================================================================

set "PATH=C:\Users\Aneena\AppData\Local\Programs\w64devkit\bin;C:\Users\Aneena\AppData\Local\Programs\Python\Python312\Scripts;C:\Users\Aneena\AppData\Local\Programs\Python\Python312;%PATH%"

echo.
echo [1/3] Building C++ Project (CMake ^& Ninja)...
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER="C:/Users/Aneena/AppData/Local/Programs/w64devkit/bin/g++.exe" -DCMAKE_MAKE_PROGRAM="C:/Users/Aneena/AppData/Local/Programs/Python/Python312/Scripts/ninja.exe"
cmake --build build

echo.
echo [2/3] Running Automated Verification Tests...
build\test_geometry.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Tests failed!
    exit /b %ERRORLEVEL%
)

echo.
echo [3/3] Running Beamforming Simulation Engine...
build\beamforming_app.exe

echo.
echo Launching Interactive Animated Scientific Visualization...
python visualization\animated_wavefronts.py

echo.
echo ====================================================================
echo Complete!
echo ====================================================================
