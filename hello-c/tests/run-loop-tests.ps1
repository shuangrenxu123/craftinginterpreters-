$ErrorActionPreference = 'Stop'

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$HelloExe = Join-Path $ProjectRoot 'build/hello.exe'

if (-not (Test-Path -LiteralPath $HelloExe)) {
    throw "找不到解释器可执行文件：$HelloExe。请先构建 hello 项目。"
}

function Assert-LoxOutput {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Name,

        [Parameter(Mandatory = $true)]
        [string] $ScriptPath,

        [Parameter(Mandatory = $true)]
        [string[]] $ExpectedLines
    )

    $actualLines = & $HelloExe $ScriptPath 2>&1
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0) {
        throw "测试失败：$Name 退出码为 $exitCode。输出：$($actualLines -join [Environment]::NewLine)"
    }

    $actualText = $actualLines -join "`n"
    $expectedText = $ExpectedLines -join "`n"

    if ($actualText -ne $expectedText) {
        throw "测试失败：$Name`n期望：`n$expectedText`n实际：`n$actualText"
    }

    Write-Host "PASS $Name"
}

Assert-LoxOutput `
    -Name 'for 循环执行并支持 break' `
    -ScriptPath (Join-Path $PSScriptRoot 'for_break.lox') `
    -ExpectedLines @(
        '3',
        'for break done'
    )

Assert-LoxOutput `
    -Name 'while 循环执行并支持 break' `
    -ScriptPath (Join-Path $PSScriptRoot 'while_break.lox') `
    -ExpectedLines @(
        '3',
        '3',
        'while break done'
    )

Assert-LoxOutput `
    -Name 'while 循环同时支持 break 和 continue' `
    -ScriptPath (Join-Path $PSScriptRoot 'while_break_continue.lox') `
    -ExpectedLines @(
        '8',
        '3',
        '5',
        'while break continue done'
    )

Assert-LoxOutput `
    -Name 'for 循环同时支持 break 和 continue' `
    -ScriptPath (Join-Path $PSScriptRoot 'for_break_continue.lox') `
    -ExpectedLines @(
        '8',
        '4',
        'for break continue done'
    )
