#ifndef SMARTCAR_DEF_H
#define SMARTCAR_DEF_H

#include "bsp_gpio.h"
#include "bsp_pwm.h"

/* PWM 鏈€澶у崰绌烘瘮锛屽搴斿畾鏃跺櫒 ARR 鍛ㄦ湡璁℃暟鍊?*/
#define MAX_MOTOR_PWM 1000

/* 鍏煎灞傦細灏?motor 妯″潡鏈熸湜鐨勭被鍨嬫槧灏勫埌褰撳墠 BSP 瀹炵幇 */
typedef GPIOInstance GPIO_Device_t;
typedef PWMInstance  PWM_Device_t;

/* 鍏煎灞傦細鍐呰仈鍖呰鍑芥暟 */
static inline void GPIO_High(GPIO_Device_t *dev) { GPIOSet(dev); }
static inline void GPIO_Low(GPIO_Device_t *dev)  { GPIOReset(dev); }
static inline void PWM_Start(PWM_Device_t *dev)  { PWMStart(dev); }

static inline void PWM_SetCompare(PWM_Device_t *dev, uint32_t compare)
{
    if ((dev != NULL) && (dev->htim != NULL) && (dev->htim->period_ticks > 0U)) {
        float ratio = (float)compare / (float)dev->htim->period_ticks;
        PWMSetDutyRatio(dev, ratio);
    }
}

#endif