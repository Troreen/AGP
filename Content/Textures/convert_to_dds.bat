@echo off
setlocal enabledelayedexpansion

echo Starting DDS Conversion...
echo ----------------------------------------

:: 1. Convert BaseColor maps -> BC7_UNORM_SRGB
for %%f in ("*_BaseColor.png") do (
    if exist "%%f" (
        echo [BC7 sRGB] Processing: %%f
        texconv.exe -y -f BC7_UNORM_SRGB -m 0 "%%f"
    )
)


for %%f in ("*_C.png") do (
    if exist "%%f" (
        echo [BC7 sRGB] Processing: %%f
        texconv.exe -y -f BC7_UNORM_SRGB -m 0 "%%f"
    )
)


:: 2. Convert OcclusionRoughnessMetallic maps -> BC7_UNORM (Linear)
for %%f in ("*_OcclusionRoughnessMetallic.png") do (
    if exist "%%f" (
        echo [BC7 Linear] Processing: %%f
        texconv.exe -y -f BC7_UNORM -m 0 "%%f"
    )
)

for %%f in ("*_ORM.png") do (
    if exist "%%f" (
        echo [BC7 Linear] Processing: %%f
        texconv.exe -y -f BC7_UNORM -m 0 "%%f"
    )
)


:: 3. Convert Normal maps -> BC5_UNORM (BC5-N)
for %%f in ("*_Normal.png") do (
    if exist "%%f" (
        echo [BC5-N] Processing: %%f
        texconv.exe -y -f BC5_UNORM -m 0 "%%f"
    )
)

echo ----------------------------------------
echo Conversion Complete!
pause