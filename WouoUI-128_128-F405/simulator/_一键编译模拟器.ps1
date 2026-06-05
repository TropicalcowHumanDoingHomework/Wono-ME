<#
═══════════════════════════════════════════════════════════════════════
          WouoUI Simulator - 一键编译脚本 (PowerShell)

  【AI 使用方法】
  第 1 步: 打开终端（确保在项目根目录或 simulator 目录下）
  第 2 步: 运行以下命令即可编译:

      cd simulator\build
      cmake --build . --config Release

  或者一步到位（从项目根目录）:

      cmake --build simulator\build --config Release

  【人类使用方法】
  直接双击项目根目录的 _一键编译模拟器.bat

  【输出文件】
  simulator\build\Release\wouo_sim.exe
═══════════════════════════════════════════════════════════════════════
#>

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$SimDir      = Join-Path $ProjectRoot "simulator"
$BuildDir    = Join-Path $SimDir "build"

# CMake 首次配置（如果尚未配置）
if (-not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))) {
    Write-Host "`n[*] 首次配置 CMake ..."
    if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }
    Push-Location $SimDir
    cmake -B build -S . -G "Visual Studio 17 2022" *>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[!] CMake 配置失败"
        Pop-Location
        exit 1
    }
    Pop-Location
}

Write-Host "`n[*] 正在编译 ..."
Push-Location $BuildDir
cmake --build . --config Release
$exitCode = $LASTEXITCODE
Pop-Location

if ($exitCode -ne 0) {
    Write-Host "`n[!] 编译失败" -ForegroundColor Red
    exit 1
}

Write-Host @"

╔══════════════════════════════════════════════════╗
║             编译成功 !                           ║
║                                                  ║
║  输出: simulator\build\Release\wouo_sim.exe     ║
╚══════════════════════════════════════════════════╝
"@
