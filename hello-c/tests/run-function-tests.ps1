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
    -Name '函数声明和无参调用' `
    -ScriptPath (Join-Path $PSScriptRoot 'function_basic.lox') `
    -ExpectedLines @(
        'hello'
    )

Assert-LoxOutput `
    -Name '函数参数、返回值和默认 nil 返回' `
    -ScriptPath (Join-Path $PSScriptRoot 'function_parameters_return.lox') `
    -ExpectedLines @(
        '6',
        'inside noReturn',
        'nil'
    )

Assert-LoxOutput `
    -Name '函数嵌套调用' `
    -ScriptPath (Join-Path $PSScriptRoot 'function_nested_calls.lox') `
    -ExpectedLines @(
        '20'
    )

Assert-LoxOutput `
    -Name '递归函数调用' `
    -ScriptPath (Join-Path $PSScriptRoot 'function_recursion.lox') `
    -ExpectedLines @(
        '55'
    )

Assert-LoxOutput `
    -Name '函数局部变量不覆盖全局变量' `
    -ScriptPath (Join-Path $PSScriptRoot 'function_local_scope.lox') `
    -ExpectedLines @(
        'local',
        'global'
    )

Assert-LoxOutput `
    -Name '闭包返回后仍能读取捕获变量' `
    -ScriptPath (Join-Path $PSScriptRoot 'function_closure_capture.lox') `
    -ExpectedLines @(
        'captured'
    )

Assert-LoxOutput `
    -Name '闭包能保留状态且实例互不共享' `
    -ScriptPath (Join-Path $PSScriptRoot 'function_closure_state.lox') `
    -ExpectedLines @(
        '1',
        '2',
        '1',
        '3',
        '2'
    )

Assert-LoxFailure `
    -Name '函数实参数量错误会失败' `
    -ScriptPath (Join-Path $PSScriptRoot 'function_arity_error.lox')
