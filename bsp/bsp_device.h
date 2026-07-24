#ifndef BSP_DEVICE_H
#define BSP_DEVICE_H

#include "bsp_def.h"

/* YueLu-facing GPIO compatibility types. */
typedef GPIO_Regs GPIO_TypeDef;

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET,
} GPIO_PinState;

#define GPIO_PIN_0  DL_GPIO_PIN_0
#define GPIO_PIN_1  DL_GPIO_PIN_1
#define GPIO_PIN_2  DL_GPIO_PIN_2
#define GPIO_PIN_3  DL_GPIO_PIN_3
#define GPIO_PIN_4  DL_GPIO_PIN_4
#define GPIO_PIN_5  DL_GPIO_PIN_5
#define GPIO_PIN_6  DL_GPIO_PIN_6
#define GPIO_PIN_7  DL_GPIO_PIN_7
#define GPIO_PIN_8  DL_GPIO_PIN_8
#define GPIO_PIN_9  DL_GPIO_PIN_9
#define GPIO_PIN_10 DL_GPIO_PIN_10
#define GPIO_PIN_11 DL_GPIO_PIN_11
#define GPIO_PIN_12 DL_GPIO_PIN_12
#define GPIO_PIN_13 DL_GPIO_PIN_13
#define GPIO_PIN_14 DL_GPIO_PIN_14
#define GPIO_PIN_15 DL_GPIO_PIN_15
#define GPIO_PIN_16 DL_GPIO_PIN_16
#define GPIO_PIN_17 DL_GPIO_PIN_17
#define GPIO_PIN_18 DL_GPIO_PIN_18
#define GPIO_PIN_19 DL_GPIO_PIN_19
#define GPIO_PIN_20 DL_GPIO_PIN_20
#define GPIO_PIN_21 DL_GPIO_PIN_21
#define GPIO_PIN_22 DL_GPIO_PIN_22
#define GPIO_PIN_23 DL_GPIO_PIN_23
#define GPIO_PIN_24 DL_GPIO_PIN_24
#define GPIO_PIN_25 DL_GPIO_PIN_25
#define GPIO_PIN_26 DL_GPIO_PIN_26
#define GPIO_PIN_27 DL_GPIO_PIN_27
#define GPIO_PIN_28 DL_GPIO_PIN_28
#define GPIO_PIN_29 DL_GPIO_PIN_29
#define GPIO_PIN_30 DL_GPIO_PIN_30
#define GPIO_PIN_31 DL_GPIO_PIN_31

/* These are lightweight TI descriptors, not vendor HAL objects. */
typedef struct {
    UART_Regs *Instance;
} UART_HandleTypeDef;

typedef struct {
    I2C_Regs *Instance;
} I2C_HandleTypeDef;

typedef struct {
    SPI_Regs *Instance;
} SPI_HandleTypeDef;

typedef struct {
    GPTIMER_Regs *Instance;
    uint32_t Channel;
    uint32_t tclk_hz;
    uint32_t period_ticks;
    bool count_up;
} TIM_HandleTypeDef;

#define TIM_CHANNEL_1 DL_TIMER_CC_0_INDEX
#define TIM_CHANNEL_2 DL_TIMER_CC_1_INDEX
#define TIM_CHANNEL_3 DL_TIMER_CC_2_INDEX
#define TIM_CHANNEL_4 DL_TIMER_CC_3_INDEX

/* Current SysConfig-owned hardware descriptors. */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

#endif
