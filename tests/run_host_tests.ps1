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
    Invoke-HostTest "imu_module_test" @(
        "-Itests/module_io_stubs", "-Imodule/imu",
        "tests/imu_module_test.c", "module/imu/imu.c"
    )
    Invoke-HostTest "vofa_module_test" @(
        "-Itests/module_io_stubs", "-Imodule/vofa",
        "tests/vofa_module_test.c", "module/vofa/vofa.c"
    )
    Invoke-HostTest "transparent_uart_test" @(
        "-Itests/module_io_stubs", "-Imodule/transparent_uart",
        "tests/transparent_uart_test.c",
        "module/transparent_uart/transparent_uart.c"
    )
    Invoke-HostTest "vision_test" @(
        "-Itests/module_io_stubs", "-Imodule/transparent_uart",
        "-Imodule/vision", "tests/vision_test.c", "module/vision/vision.c"
    )
    Invoke-HostTest "ins_test" @(
        "-Itests/ins_stubs", "-Itests/module_io_stubs",
        "-Imodule/imu", "-Imodule/algorithm", "-Imodule/ins",
        "tests/ins_test.c", "module/ins/ins.c", "module/algorithm/pid.c"
    )
    Invoke-HostTest "servo_test" @(
        "-Itests/motor_stubs", "-Itests/stubs", "-Imodule/servo",
        "tests/servo_test.c", "module/servo/servo.c"
    )
    Invoke-HostTest "key_test" @(
        "-Itests/motor_stubs", "-Itests/stubs", "-Imodule/key",
        "tests/key_test.c", "module/key/key.c"
    )
    Invoke-HostTest "electromagnet_test" @(
        "-Itests/stubs", "-Ibsp/gpio", "-Imodule/electromagnet",
        "tests/electromagnet_test.c", "module/electromagnet/electromagnet.c"
    )
    Invoke-HostTest "photoelectric_test" @(
        "-Itests/stubs", "-Ibsp/gpio", "-Imodule/photoelectric",
        "tests/photoelectric_test.c", "module/photoelectric/photoelectric.c"
    )
    Invoke-HostTest "indicator_test" @(
        "-Itests/indicator_stubs", "-Itests/motor_stubs", "-Itests/stubs",
        "-Imodule/led", "-Imodule/buzzer",
        "tests/indicator_test.c", "module/led/led.c", "module/buzzer/buzzer.c"
    )
    Invoke-HostTest "indicator_init_failure_test" @(
        "-Itests/indicator_stubs", "-Itests/motor_stubs", "-Itests/stubs",
        "-Imodule/led", "-Imodule/buzzer",
        "tests/indicator_init_failure_test.c",
        "module/led/led.c", "module/buzzer/buzzer.c"
    )
    Invoke-HostTest "pid_controller_test" @(
        "-Imodule/algorithm",
        "tests/pid_controller_test.c", "module/algorithm/pid.c"
    )
    Invoke-HostTest "motor_driver_test" @(
        "-Itests/motor_stubs", "-Itests/stubs", "-Imodule/motor/driver",
        "tests/motor_driver_test.c",
        "module/motor/driver/motor_driver.c",
        "module/motor/driver/drv8701e_driver.c",
        "module/motor/driver/tb6612_driver.c"
    )
    Invoke-HostTest "motor_closed_loop_test" @(
        "-Itests/motor_stubs", "-Itests/stubs",
        "-Imodule", "-Imodule/motor", "-Imodule/motor/driver",
        "tests/motor_closed_loop_test.c",
        "module/motor/motor.c", "module/algorithm/pid.c",
        "module/motor/driver/motor_driver.c",
        "module/motor/driver/drv8701e_driver.c",
        "module/motor/driver/tb6612_driver.c"
    )
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
