#include "bsp_init.h"
#include "hardware_test_config.h"
#include "robot.h"
#include "robot_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
#include "ball_balance.h"
#include "chassis.h"
#include "competition.h"
#include "imu.h"
#include "line_follow.h"
#include "oled.h"
#include "vofa.h"
#endif

#define ROBOT_FAST_PERIOD_SECONDS 0.001f
#define ROBOT_CONTROL_PERIOD_SECONDS 0.005f
#define ROBOT_CONTROL_DIVIDER 5U

volatile bool robot_oled_initialized;
volatile uint32_t robot_oled_refresh_count;
volatile uint32_t robot_oled_error_count;

#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
typedef struct {
    LineFollow_Output_t line_follow;
    BallBalance_Status_t ball_balance;
    Chassis_Status_t chassis;
    Competition_Output_t competition;
    uint32_t uptime_ms;
    uint8_t control_divider;
    bool app_ready;
} Robot_App_Context_t;

static Robot_App_Context_t g_robot_app;

static void RobotTelemetryTask(void)
{
    IMU_Data_t imu_data;
    if (IMU_ReadAll(&imu_data) != DEVICE_OK) {
        return;
    }

    float vofa_buf[9] = {
        imu_data.ax, imu_data.ay, imu_data.az,
        imu_data.gx, imu_data.gy, imu_data.gz,
        imu_data.roll, imu_data.pitch, imu_data.yaw,
    };
    (void) VOFA_JustFloatOutputDMA(vofa_buf, 9U);
}

static void RobotControlTask(void)
{
    LineFollowTask(g_robot_app.competition.line_follow_enabled,
        ROBOT_CONTROL_PERIOD_SECONDS, &g_robot_app.line_follow);
    CompetitionTask(g_robot_app.uptime_ms, g_robot_app.app_ready,
        &g_robot_app.line_follow, &g_robot_app.ball_balance,
        &g_robot_app.competition);
    ChassisTask(&g_robot_app.competition.chassis,
        ROBOT_CONTROL_PERIOD_SECONDS, &g_robot_app.chassis);
}
#endif

void RobotInit(void)
{
    __disable_irq();
    BSPInit();

#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
    (void) VOFA_Init();
    (void) IMU_Init();

    const bool chassis_ready = ChassisInit();
    const bool line_ready = LineFollowInit();
    const bool balance_ready = BallBalanceInit();
    const bool competition_ready = CompetitionInit();
    g_robot_app = (Robot_App_Context_t) {0};
    g_robot_app.app_ready = chassis_ready && line_ready &&
        balance_ready && competition_ready;
#endif

    OSTaskInit();
    __enable_irq();
}

void RobotTask(void)
{
#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
    g_robot_app.uptime_ms++;
    BallBalanceTask(&g_robot_app.competition.ball_balance,
        g_robot_app.uptime_ms, ROBOT_FAST_PERIOD_SECONDS,
        &g_robot_app.ball_balance);

    g_robot_app.control_divider++;
    if (g_robot_app.control_divider >= ROBOT_CONTROL_DIVIDER) {
        g_robot_app.control_divider = 0U;
        RobotControlTask();
    }
#endif
}

void RobotOLEDTask(void)
{
#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
    if (!robot_oled_initialized) {
        if (OLED_init_ex() != DEVICE_OK) {
            robot_oled_error_count++;
            return;
        }
        robot_oled_initialized = true;
    }

    RobotTelemetryTask();
    OLED_operate_gram(PEN_CLEAR);
    OLED_printf(0U, 0U, "S:%u T:%lus",
        (unsigned) g_robot_app.competition.status.state,
        (unsigned long) (g_robot_app.competition.status.elapsed_ms / 1000U));
    OLED_printf(1U, 0U, "L:%s V:%s B:%s",
        g_robot_app.competition.status.line_valid ? "OK" : "NO",
        g_robot_app.competition.status.vision_valid ? "OK" : "NO",
        g_robot_app.ball_balance.enabled ? "ON" : "OFF");
    OLED_printf(2U, 0U, "LT:%6d LM:%6d",
        (int) g_robot_app.chassis.left_target_counts_s,
        (int) g_robot_app.chassis.left_measured_counts_s);
    OLED_printf(3U, 0U, "RT:%6d RM:%6d",
        (int) g_robot_app.chassis.right_target_counts_s,
        (int) g_robot_app.chassis.right_measured_counts_s);
    OLED_printf(4U, 0U, "BALL:%4d ST:%5ld",
        (int) g_robot_app.ball_balance.measured_position,
        (long) g_robot_app.ball_balance.step_position);
    OLED_refresh_gram();
    robot_oled_refresh_count++;
#endif
}
