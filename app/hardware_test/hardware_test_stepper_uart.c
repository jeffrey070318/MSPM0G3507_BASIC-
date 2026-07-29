#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_STEPPER_UART

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "stepper.h"
#include "transparent_uart.h"

#define STEPPER_UART_RX_CHUNK_SIZE 16U
#define STEPPER_UART_LINE_SIZE     32U
#define STEPPER_UART_DEFAULT_SPEED 400U

static Stepper_Device_t stepper;
static TransparentUART_Device_t stepper_uart;
static char line_buffer[STEPPER_UART_LINE_SIZE];
static uint8_t line_length;

volatile uint32_t hardware_test_stepper_command_count;
volatile uint32_t hardware_test_stepper_error_count;
volatile uint32_t hardware_test_stepper_rx_drop_count;
volatile int32_t hardware_test_stepper_position_steps;
volatile uint16_t hardware_test_stepper_speed_sps;
volatile bool hardware_test_stepper_running;

static Device_Status_e StepperUARTSend(const char *text)
{
    return TransparentUART_Send(
        &stepper_uart, (uint8_t *) text, (uint16_t) strlen(text));
}

static char *NextToken(char **cursor)
{
    char *token = strtok(*cursor, " \t\r\n");
    *cursor = NULL;
    return token;
}

static void Uppercase(char *text)
{
    while ((text != NULL) && (*text != '\0')) {
        if ((*text >= 'a') && (*text <= 'z')) {
            *text = (char) (*text - ('a' - 'A'));
        }
        text++;
    }
}

static uint16_t ParseSpeed(const char *token)
{
    long speed = (token != NULL) ? strtol(token, NULL, 10)
                                 : STEPPER_UART_DEFAULT_SPEED;
    if (speed < (long) STEPPER_MIN_SPEED_SPS) {
        speed = STEPPER_MIN_SPEED_SPS;
    }
    if (speed > (long) STEPPER_MAX_SPEED_SPS) {
        speed = STEPPER_MAX_SPEED_SPS;
    }
    return (uint16_t) speed;
}

static Device_Status_e HandleMoveCommand(char **cursor)
{
    char *steps_token = NextToken(cursor);
    if (steps_token == NULL) {
        return DEVICE_ERROR;
    }

    long steps = strtol(steps_token, NULL, 10);
    if ((steps == 0) || (steps == LONG_MIN) ||
        (steps < (long) INT32_MIN) || (steps > (long) INT32_MAX)) {
        return DEVICE_ERROR;
    }

    Stepper_Direction_e direction =
        (steps > 0) ? STEPPER_DIR_UP : STEPPER_DIR_DOWN;
    uint32_t step_count =
        (steps > 0) ? (uint32_t) steps : (uint32_t) -steps;

    return Stepper_Move(
        &stepper, direction, step_count, ParseSpeed(NextToken(cursor)));
}

static void HandleLine(char *line)
{
    char *cursor = line;
    char *command = NextToken(&cursor);
    if (command == NULL) {
        return;
    }

    Uppercase(command);

    Device_Status_e status = DEVICE_ERROR;
    if ((strcmp(command, "M") == 0) || (strcmp(command, "MOVE") == 0)) {
        status = HandleMoveCommand(&cursor);
    } else if ((strcmp(command, "S") == 0) ||
               (strcmp(command, "STOP") == 0)) {
        Stepper_Stop(&stepper);
        status = DEVICE_OK;
    } else if (strcmp(command, "?") == 0) {
        status = DEVICE_OK;
    }

    if (status == DEVICE_OK) {
        hardware_test_stepper_command_count++;
        (void) StepperUARTSend("OK\r\n");
    } else {
        hardware_test_stepper_error_count++;
        (void) StepperUARTSend("ERR\r\n");
    }
}

static void PollUART(void)
{
    uint8_t data[STEPPER_UART_RX_CHUNK_SIZE];
    uint16_t received = 0U;

    if (TransparentUART_Read(&stepper_uart, data, sizeof(data), &received) !=
        DEVICE_OK) {
        return;
    }

    for (uint16_t i = 0U; i < received; ++i) {
        char ch = (char) data[i];
        if ((ch == '?') && (line_length == 0U)) {
            line_buffer[0] = '?';
            line_buffer[1] = '\0';
            HandleLine(line_buffer);
        } else if ((ch == '\n') || (ch == '\r')) {
            if (line_length > 0U) {
                line_buffer[line_length] = '\0';
                HandleLine(line_buffer);
                line_length = 0U;
            }
        } else if (line_length < (STEPPER_UART_LINE_SIZE - 1U)) {
            line_buffer[line_length++] = ch;
        } else {
            line_length = 0U;
            hardware_test_stepper_rx_drop_count++;
            (void) StepperUARTSend("ERR LINE\r\n");
        }
    }
}

Device_Status_e HardwareTestInit(void)
{
    if (Stepper_Init(&stepper) != DEVICE_OK) {
        return DEVICE_ERROR;
    }

    if (TransparentUART_Init(&stepper_uart, TRANSPARENT_UART_PORT_1) !=
        DEVICE_OK) {
        return DEVICE_ERROR;
    }

    (void) StepperUARTSend("STEPPER UART READY\r\n");
    return DEVICE_OK;
}

void HardwareTestRun(void)
{
    PollUART();
    Stepper_Task(&stepper, STEPPER_TASK_PERIOD_MS);

    hardware_test_stepper_position_steps = stepper.position_steps;
    hardware_test_stepper_speed_sps = stepper.speed_sps;
    hardware_test_stepper_running = stepper.running;
}

#endif
