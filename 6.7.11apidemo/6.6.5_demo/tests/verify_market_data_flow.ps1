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
        Name = "menu option 2 waits for market-data subscribe response"
        Ok = ([regex]::Matches($case2Block, [regex]::Escape('WaitForSingleObject(xinhao, INFINITE);'))).Count -ge 3
    },
    @{
        Name = "menu option 2 waits for first market data tick"
        Ok = $case2Block.Contains('WaitForSingleObject(g_hMdDataEvent, 10000)')
    },
    @{
        Name = "menu option 2 clears trailing newline before waiting for exit key"
        Ok = $case2Block.Contains("cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');")
    },
    @{
        Name = "menu option 2 no longer uses quote subscription for verification"
        Ok = -not $case2Block.Contains('ash.SubscribeForQuoteRsp();')
    },
    @{
        Name = "depth callback prints concise md tick marker"
        Ok = $mainH.Contains('[MD_TICK]')
    },
    @{
        Name = "depth callback prints first successful market-data marker"
        Ok = $mainH.Contains('[MARKET_DATA_OK]')
    },
    @{
        Name = "depth callback tracks whether first tick was already received"
        Ok = $mainH.Contains('m_hasReceivedFirstMarketData')
    },
    @{
        Name = "depth callback signals first market data event"
        Ok = $mainH.Contains('SetEvent(g_hMdDataEvent);')
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
