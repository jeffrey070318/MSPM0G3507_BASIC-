#ifndef BSP_MSPM0_COMPAT_H
#define BSP_MSPM0_COMPAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U,
} HAL_StatusTypeDef;

typedef GPIO_Regs GPIO_TypeDef;

typedef enum {
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET = 1U,
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

static inline void HAL_GPIO_WritePin(
    GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin, GPIO_PinState PinState)
{
    if (PinState == GPIO_PIN_SET) {
        DL_GPIO_setPins(GPIOx, GPIO_Pin);
    } else {
        DL_GPIO_clearPins(GPIOx, GPIO_Pin);
    }
}

static inline void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin)
{
    DL_GPIO_togglePins(GPIOx, GPIO_Pin);
}

static inline GPIO_PinState HAL_GPIO_ReadPin(
    GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin)
{
    return (DL_GPIO_readPins(GPIOx, GPIO_Pin) != 0U) ? GPIO_PIN_SET
                                                     : GPIO_PIN_RESET;
}

typedef UART_Regs UART_TypeDef;
typedef I2C_Regs I2C_TypeDef;
typedef SPI_Regs SPI_TypeDef;
typedef GPTIMER_Regs TIM_TypeDef;

typedef struct {
    UART_TypeDef *Instance;
    volatile uint32_t gState;
    void *hdmarx;
    uint8_t *rx_buffer;
    uint16_t rx_size;
    volatile uint16_t rx_count;
} UART_HandleTypeDef;

#define HAL_UART_STATE_READY   (0x00000000U)
#define HAL_UART_STATE_BUSY_TX (0x00000001U)
#define HAL_UART_STATE_BUSY_RX (0x00000002U)

typedef struct {
    I2C_TypeDef *Instance;
    uint16_t Devaddress;
} I2C_HandleTypeDef;

typedef struct {
    SPI_TypeDef *Instance;
} SPI_HandleTypeDef;

typedef struct {
    uint32_t Prescaler;
} TIM_Base_InitTypeDef;

typedef struct {
    TIM_TypeDef *Instance;
    TIM_Base_InitTypeDef Init;
    uint32_t Channel;
    uint32_t tclk_hz;
    uint32_t period_ticks;
} TIM_HandleTypeDef;

#define TIM_CHANNEL_1 DL_TIMER_CC_0_INDEX
#define TIM_CHANNEL_2 DL_TIMER_CC_1_INDEX
#define TIM_CHANNEL_3 DL_TIMER_CC_2_INDEX
#define TIM_CHANNEL_4 DL_TIMER_CC_3_INDEX

#define SPI1 SPI_0_INST
#define SPI2 SPI_0_INST
#define I2C1 I2C_0_INST
#define I2C2 I2C_0_INST
#define USART1 UART_0_INST
#define USART2 UART_0_INST
#define UART1 UART_0_INST
#define UART2 UART_0_INST

extern UART_HandleTypeDef huart0;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c0;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern SPI_HandleTypeDef hspi0;
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart,
    uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Transmit_IT(
    UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Transmit_DMA(
    UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(
    UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);

#define DMA_IT_HT (0x00000001U)
#define __HAL_DMA_DISABLE_IT(hdma, it) \
    do {                               \
        (void) (hdma);                 \
        (void) (it);                   \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif
