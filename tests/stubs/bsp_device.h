#ifndef TEST_STUB_BSP_DEVICE_H
#define TEST_STUB_BSP_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DEVICE_OK = 0,
    DEVICE_ERROR,
    DEVICE_BUSY,
    DEVICE_TIMEOUT,
} Device_Status_e;

typedef struct {
    uint32_t load;
    uint32_t compare[4];
    uint32_t start_count;
    uint32_t stop_count;
    bool running;
} GPTIMER_Regs;

typedef uint32_t DL_TIMER_CC_INDEX;

typedef struct {
    uint32_t input;
    uint32_t output;
} GPIO_TypeDef;

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET,
} GPIO_PinState;

typedef uint32_t DL_GPIO_IIDX;
#define DL_GPIO_IIDX_DIO0  ((DL_GPIO_IIDX) 1U)
#define DL_GPIO_IIDX_DIO12 ((DL_GPIO_IIDX) 13U)
#define DL_GPIO_IIDX_DIO31 ((DL_GPIO_IIDX) 32U)

typedef struct {
    uint32_t status;
    uint32_t target_address;
    uint16_t transfer_size;
    uint32_t direction;
    uint32_t stop_mode;
    uint8_t rx_value;
    uint32_t reset_count;
} I2C_Regs;

typedef struct {
    I2C_Regs *Instance;
} I2C_HandleTypeDef;

#define DL_I2C_CONTROLLER_STATUS_BUSY  (1UL << 0U)
#define DL_I2C_CONTROLLER_STATUS_ERROR (1UL << 1U)
#define DL_I2C_CONTROLLER_STATUS_IDLE  (1UL << 2U)
#define DL_I2C_CONTROLLER_STATUS_ARBITRATION_LOST (1UL << 3U)
#define DL_I2C_CONTROLLER_DIRECTION_TX 0U
#define DL_I2C_CONTROLLER_DIRECTION_RX 1U
#define DL_I2C_CONTROLLER_START_ENABLE 1U
#define DL_I2C_CONTROLLER_STOP_DISABLE 0U
#define DL_I2C_CONTROLLER_STOP_ENABLE  1U
#define DL_I2C_CONTROLLER_ACK_DISABLE  0U

typedef struct {
    GPTIMER_Regs *Instance;
    uint32_t Channel;
    uint32_t tclk_hz;
    uint32_t period_ticks;
    bool count_up;
} TIM_HandleTypeDef;

void DL_Timer_startCounter(GPTIMER_Regs *timer);
void DL_Timer_stopCounter(GPTIMER_Regs *timer);
void DL_Timer_setLoadValue(GPTIMER_Regs *timer, uint32_t load);
void DL_Timer_setCaptureCompareValue(
    GPTIMER_Regs *timer, uint32_t compare, DL_TIMER_CC_INDEX channel);
void DL_GPIO_togglePins(GPIO_TypeDef *gpio, uint32_t pins);
void DL_GPIO_setPins(GPIO_TypeDef *gpio, uint32_t pins);
void DL_GPIO_clearPins(GPIO_TypeDef *gpio, uint32_t pins);
uint32_t DL_GPIO_readPins(GPIO_TypeDef *gpio, uint32_t pins);
uint32_t DL_I2C_getControllerStatus(I2C_Regs *i2c);
void DL_I2C_fillControllerTXFIFO(
    I2C_Regs *i2c, const uint8_t *data, uint16_t size);
void DL_I2C_startControllerTransferAdvanced(I2C_Regs *i2c,
    uint32_t target_address, uint32_t direction, uint16_t size,
    uint32_t start, uint32_t stop, uint32_t ack);
bool DL_I2C_isControllerTXFIFOFull(I2C_Regs *i2c);
void DL_I2C_transmitControllerData(I2C_Regs *i2c, uint8_t data);
bool DL_I2C_isControllerRXFIFOEmpty(I2C_Regs *i2c);
uint8_t DL_I2C_receiveControllerData(I2C_Regs *i2c);
void DL_I2C_resetControllerTransfer(I2C_Regs *i2c);

static inline void delay_cycles(uint32_t cycles)
{
    (void) cycles;
}

static inline uint32_t __get_PRIMASK(void)
{
    return 0U;
}

static inline void __disable_irq(void)
{
}

static inline void __enable_irq(void)
{
}

#endif
