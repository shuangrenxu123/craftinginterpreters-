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

function Assert-LoxFailure {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Name,

        [Parameter(Mandatory = $true)]
        [string] $ScriptPath
    )

    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $null = & $HelloExe $ScriptPath 2>&1
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if ($exitCode -eq 0) {
        throw "测试失败：$Name 应该失败，但实际成功。"
    }

    Write-Host "PASS $Name"
}

Assert-LoxOutput `
    -Name 'type str assert 基础行为' `
    -ScriptPath (Join-Path $PSScriptRoot 'native_core.lox') `
    -ExpectedLines @(
        'number',
        'bool',
        'nil',
        'string',
        'function',
        'native',
        '123',
        'false',
        'nil',
        'lox',
        '<fn sample>',
        '<native fn>',
        'native core done'
    )

Assert-LoxFailure `
    -Name 'assert false 会失败' `
    -ScriptPath (Join-Path $PSScriptRoot 'native_assert_failure.lox')
