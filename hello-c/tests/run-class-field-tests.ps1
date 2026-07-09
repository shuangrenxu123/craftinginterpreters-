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

    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $actualLines = & $HelloExe $ScriptPath 2>&1
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

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
        $actualLines = & $HelloExe $ScriptPath 2>&1
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if ($exitCode -eq 0) {
        throw "测试失败：$Name 应该失败，但实际成功。输出：$($actualLines -join [Environment]::NewLine)"
    }

    Write-Host "PASS $Name"
}

Assert-LoxOutput `
    -Name '没有字段时 init 构造函数可被调用' `
    -ScriptPath (Join-Path $PSScriptRoot 'class_init_only.lox') `
    -ExpectedLines @(
        'Alice'
    )

Assert-LoxOutput `
    -Name '类字段可在构造函数中赋值并由方法读取' `
    -ScriptPath (Join-Path $PSScriptRoot 'class_fields_basic.lox') `
    -ExpectedLines @(
        'Alice',
        '20',
        'Alice',
        'class fields basic done'
    )

Assert-LoxOutput `
    -Name '声明字段默认值为 nil 且之后可赋值' `
    -ScriptPath (Join-Path $PSScriptRoot 'class_fields_default_nil.lox') `
    -ExpectedLines @(
        'nil',
        'filled',
        'class fields default nil done'
    )

Assert-LoxOutput `
    -Name '字段支持字面量默认值' `
    -ScriptPath (Join-Path $PSScriptRoot 'class_fields_default_literals.lox') `
    -ExpectedLines @(
        'unnamed',
        '3',
        'true',
        'nil',
        '5',
        'class fields default literals done'
    )

Assert-LoxOutput `
    -Name '字段默认值表达式在类声明时求值一次' `
    -ScriptPath (Join-Path $PSScriptRoot 'class_fields_default_expression_once.lox') `
    -ExpectedLines @(
        '1',
        '1',
        '1',
        'class fields default expression once done'
    )

Assert-LoxOutput `
    -Name '不同实例拥有独立字段表' `
    -ScriptPath (Join-Path $PSScriptRoot 'class_fields_instance_isolation.lox') `
    -ExpectedLines @(
        '2',
        '10',
        'class fields instance isolation done'
    )

Assert-LoxFailure `
    -Name '不能给未声明字段赋值' `
    -ScriptPath (Join-Path $PSScriptRoot 'class_fields_dynamic_set_error.lox')

Assert-LoxFailure `
    -Name '不能读取未声明字段' `
    -ScriptPath (Join-Path $PSScriptRoot 'class_fields_undeclared_get_error.lox')
