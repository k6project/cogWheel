@echo off
if not exist "%~dp0project" mkdir %~dp0project
cd %~dp0project
cmake -G "Visual Studio 14 2015 Win64" ..
cd %~dp0
