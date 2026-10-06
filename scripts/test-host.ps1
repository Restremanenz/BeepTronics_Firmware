$ErrorActionPreference = 'Stop'
$projectPath = Split-Path -Parent $PSScriptRoot
Push-Location $projectPath
try {
    New-Item -ItemType Directory -Force '.test-build' | Out-Null
    & g++ -std=c++11 -Wall -Wextra -Werror -Iinclude -Isrc -Itest/host/stubs `
        test/host/test_main.cpp src/core/VarioFilter.cpp src/core/ClimbTone.cpp `
        src/core/Lk8ex1.cpp src/drivers/MS5611.cpp -o .test-build/tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Host test compilation failed.' }
    & ./.test-build/tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Host tests failed.' }
} finally {
    Pop-Location
}
