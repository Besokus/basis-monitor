$appMain = Get-Content "$PSScriptRoot\..\app\main.cpp" -Raw -ErrorAction Stop
$cmake = Get-Content "$PSScriptRoot\..\CMakeLists.txt" -Raw -ErrorAction Stop

$requiredPaths = @(
    "$PSScriptRoot\..\include\basis_monitor\ctp\md_api_session.h",
    "$PSScriptRoot\..\include\basis_monitor\ctp\md_spi_bridge.h",
    "$PSScriptRoot\..\src\ctp\md_api_session.cpp",
    "$PSScriptRoot\..\src\ctp\md_spi_bridge.cpp",
    "$PSScriptRoot\..\include\basis_monitor\platform\linux_compat.h",
    "$PSScriptRoot\..\src\config\config_loader.cpp",
    "$PSScriptRoot\..\include\basis_monitor\logging\logger.h",
    "$PSScriptRoot\..\src\logging\logger.cpp",
    "$PSScriptRoot\..\config\ctp.ini",
    "$PSScriptRoot\..\run.sh",
    "$PSScriptRoot\..\vendor\ctp\live\include\ThostFtdcMdApi.h",
    "$PSScriptRoot\..\vendor\ctp\live\include\ThostFtdcUserApiDataType.h",
    "$PSScriptRoot\..\vendor\ctp\live\include\ThostFtdcUserApiStruct.h",
    "$PSScriptRoot\..\vendor\ctp\live\lib\linux\thostmduserapi_se.so",
    "$PSScriptRoot\..\vendor\ctp\data_collect\DataCollect.h",
    "$PSScriptRoot\..\vendor\ctp\data_collect\LinuxDataCollect.so"
)

$checks = @(
    @{
        Name = "cmake includes md session sources"
        Ok = $cmake.Contains('src/ctp/md_api_session.cpp') -and $cmake.Contains('src/ctp/md_spi_bridge.cpp')
    },
    @{
        Name = "app wires market data session"
        Ok = $appMain.Contains('MdApiSession')
    },
    @{
        Name = "app waits for first market data"
        Ok = $appMain.Contains('WaitForFirstMarketData')
    },
    @{
        Name = "md session fails fast on negative login request returns"
        Ok = $cmake.Contains('src/ctp/md_api_session.cpp') -and ((Get-Content "$PSScriptRoot\..\src\ctp\md_api_session.cpp" -Raw).Contains('LoginRequestAccepted'))
    },
    @{
        Name = "md session fails fast on negative subscribe request returns"
        Ok = $cmake.Contains('src/ctp/md_api_session.cpp') -and ((Get-Content "$PSScriptRoot\..\src\ctp\md_api_session.cpp" -Raw).Contains('SubscriptionRequestAccepted'))
    },
    @{
        Name = "subscription success aggregates callback errors"
        Ok = ((Get-Content "$PSScriptRoot\..\src\ctp\md_spi_bridge.cpp" -Raw).Contains('subscription_error_seen_'))
    },
    @{
        Name = "app prints market-data startup summary"
        Ok = $appMain.Contains('FrontMdAddr') -and $appMain.Contains('InstrumentID')
    }
)

foreach ($path in $requiredPaths) {
    $checks += @{
        Name = "required path exists: $([System.IO.Path]::GetFileName($path))"
        Ok = Test-Path $path
    }
}

$failed = @($checks | Where-Object { -not $_.Ok })
if ($failed.Count -gt 0) {
    $failed | ForEach-Object {
        Write-Error ("FAILED: " + $_.Name)
    }
    exit 1
}

Write-Host "Basis monitor market-data migration checks passed."
