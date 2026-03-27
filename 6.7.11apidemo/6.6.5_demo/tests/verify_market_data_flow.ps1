$mainCpp = Get-Content "$PSScriptRoot\..\main.cpp" -Raw
$mainH = Get-Content "$PSScriptRoot\..\main.h" -Raw

$mdLoginBlock = [regex]::Match(
    $mainH,
    '(?s)void ReqUserLogin\(\)\s*\{.*?LogRequestReturnStatus\("MdApi::ReqUserLogin", num\);'
).Value
$case2Block = [regex]::Match(
    $mainCpp,
    '(?s)string g_chFrontMdaddr = getConfig\("config", "FrontMdAddr"\);.*?return 0;'
).Value

$checks = @(
    @{
        Name = "md login fills user id"
        Ok = $mdLoginBlock -match '(?m)^\s*strcpy_s\(reqUserLogin\.UserID, g_chUserID\);'
    },
    @{
        Name = "md login fills password"
        Ok = $mdLoginBlock -match '(?m)^\s*strcpy_s\(reqUserLogin\.Password, g_chPassword\);'
    },
    @{
        Name = "menu option 2 reads InstrumentID from config"
        Ok = $case2Block.Contains('getConfig("config", "InstrumentID")')
    },
    @{
        Name = "menu option 2 subscribes market data"
        Ok = $case2Block.Contains('ash.SubscribeMarketData();')
    },
    @{
        Name = "menu option 2 no longer uses quote subscription for verification"
        Ok = -not $case2Block.Contains('ash.SubscribeForQuoteRsp();')
    },
    @{
        Name = "depth callback prints concise md tick marker"
        Ok = $mainH.Contains('[MD_TICK]')
    }
)

$failed = @($checks | Where-Object { -not $_.Ok })
if ($failed.Count -gt 0) {
    $failed | ForEach-Object {
        Write-Error ("FAILED: " + $_.Name)
    }
    exit 1
}

Write-Host "All market-data flow checks passed."
