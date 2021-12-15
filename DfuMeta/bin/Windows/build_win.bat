@::

@echo.

echo .0. > .\win_build_result_codes

@if exist Release  rmdir /s/q Release
@if exist Debug    rmdir /s/q Debug

@setlocal

for /F "tokens=*" %%g in ('dir /b "C:\Program Files (x86)\Microsoft Visual Studio\2019"') do (
    set "MSBUILD_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2019\%%g\MSBuild\Current\Bin\MSBuild.exe"
)

"%MSBUILD_EXE%" DfuManifest.sln /property:Configuration=Release /property:Platform=x86
@::    /warnAsError
echo .%ERRORLEVEL%. >> .\win_build_result_codes

@::  "%MSBUILD_EXE%" DfuManifest.sln /property:Configuration=Debug /property:Platform=x86
@::  @::    /warnAsError
@::  echo .%ERRORLEVEL%. >> .\win_build_result_codes

@echo.
type .\win_build_result_codes
@type .\win_build_result_codes | ^
for /F "tokens=*" %%g in ('%SystemRoot%\system32\find.exe /v /c ".0."') do @(exit /b %%g)
