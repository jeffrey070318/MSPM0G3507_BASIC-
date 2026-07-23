$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$outputDir = Join-Path $projectRoot "build-host-tests"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

function Invoke-HostTest {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Name,
        [Parameter(Mandatory = $true)]
        [string[]] $Arguments
    )

    $output = Join-Path $outputDir "$Name.exe"
    & gcc -std=c11 -Wall -Wextra -Werror @Arguments -o $output
    if ($LASTEXITCODE -ne 0) {
        throw "$Name compile failed"
    }
    & $output
    if ($LASTEXITCODE -ne 0) {
        throw "$Name failed"
    }
    Write-Host "PASS $Name"
}

Push-Location $projectRoot
try {
    Invoke-HostTest "encoder_decode_test" @(
        "-Ibsp/encoder",
        "tests/encoder_decode_test.c",
        "bsp/encoder/bsp_encoder_decode.c"
    )
    Invoke-HostTest "bsp_pwm_shared_test" @(
        "-Itests/stubs", "-Ibsp/pwm",
        "tests/bsp_pwm_shared_test.c", "bsp/pwm/bsp_pwm.c"
    )
    Invoke-HostTest "bsp_usart_state_test" @(
        "-Ibsp/usart",
        "tests/bsp_usart_state_test.c", "bsp/usart/bsp_usart_state.c"
    )
    Invoke-HostTest "bsp_gpio_test" @(
        "-Itests/stubs", "-Ibsp/gpio",
        "tests/bsp_gpio_test.c", "bsp/gpio/bsp_gpio.c"
    )
    Invoke-HostTest "bsp_iic_test" @(
        "-Itests/stubs", "-Ibsp", "-Ibsp/iic",
        "tests/bsp_iic_test.c", "bsp/iic/bsp_iic.c"
    )
    Invoke-HostTest "bsp_log_test" @(
        "-Itests/stubs", "-Ibsp/log",
        "tests/bsp_log_test.c", "bsp/log/bsp_log.c"
    )
}
finally {
    Pop-Location
}
