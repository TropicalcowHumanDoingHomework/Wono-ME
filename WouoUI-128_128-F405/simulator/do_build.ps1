& {
    $simDir = "C:\Users\Lenovo\Documents\共享的文件\罗米奇存储总集\U盘、存储卡拷贝集\罗米奇的U盘\编程作品\外网软件&硬件项目\Wonome\WouoUI-128_128-F405\simulator"
    Set-Location $simDir
    cmake --build build --config Release 2> build_errors.txt | Out-File build_output.txt -Encoding UTF8
    Get-Content build_output.txt -Tail 30
    Write-Host "=== ERRORS ===" 
    Get-Content build_errors.txt -Tail 30
}