@echo off
title Rubik-4D Cryptographic Lab & Tesseract Visualizer
echo ========================================================
echo   RUBIK-4D CRYPTOGRAPHIC SUITE ^& 4D VISUALIZER
echo ========================================================
echo Checking environment...

python -c "import customtkinter, matplotlib, PIL, numpy" >nul 2>&1
if %errorlevel% neq 0 (
    echo Installing required Python libraries...
    pip install customtkinter matplotlib pillow numpy
)

echo Launching Rubik-4D Desktop GUI...
python gui\rubik4d_app.py
pause
