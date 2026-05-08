#include "engine_sim.h"

/* ============================================================
 * Period
 * ============================================================ */

#define ENG_PERIOD_RPM_TX_MS            50U
#define ENG_PERIOD_IGN_TX_MS            50U
#define ENG_PERIOD_SPEED_1A0_TX_MS      50U
#define ENG_PERIOD_SPEED_5A0_TX_MS      50U
#define ENG_PERIOD_COOLANT_TX_MS        100U
#define ENG_PERIOD_WARNING_TX_MS        100U

#define ENG_PERIOD_MODEL_MS             50U
#define ENG_PERIOD_COOLANT_MODEL_MS     1000U
#define ENG_WARNING_CHIME_DURATION_MS   1000U

 /* ============================================================
  * Engine Model Config
  * ============================================================ */

#define ENGINE_RPM_IDLE                 800U
#define ENGINE_RPM_MAX                  6000U
#define ENGINE_SPEED_MAX                240U
#define ENGINE_RPM_WARN_THRESHOLD       5000U

  /* ============================================================
   * Coolant Model Config
   * ============================================================ */

#define ENGINE_COOLANT_MIN              60U
#define ENGINE_COOLANT_INIT             60U
#define ENGINE_COOLANT_MAX              130U
#define ENGINE_COOLANT_WARN_THRESHOLD   115U
#define ENGINE_COOLANT_CAN_MIN          60U
#define ENGINE_COOLANT_CAN_MAX          130U

#define ENGINE_COOLANT_RPM_MIN          800U
#define ENGINE_COOLANT_RPM_MAX          6000U

   /* ============================================================
    * ADC / Pedal Config
    * ============================================================ */

#define ADC_MAX_VALUE                   4095U
#define ADC_FILTER_SHIFT                3U

#define THROTTLE_DEADZONE_PERCENT       3U
#define BRAKE_DEADZONE_PERCENT          8U

#define PEDAL_MAX_CLAMP_PERCENT         97U

#define BRAKE_EFFECT_PERCENT            35U

    /* ============================================================
     * Static Variables
     * ============================================================ */

static EngineSimStatus_t engine = {
    .mode = ENGINE_MODE_ADC,
    .throttle = 0,
    .brake = 0,
    .rpm = ENGINE_RPM_IDLE,
    .speed = 0,
    .coolant = ENGINE_COOLANT_INIT,
    .tx_count = 0
};

static uint8_t adc_filter_init = 0;
static uint16_t throttle_adc_filtered = 0;
static uint16_t brake_adc_filtered = 0;
static uint8_t warning_alive = 0;
static uint8_t speed_1a0_alive = 0;
static uint8_t warning_prev_active = 0;
static uint8_t warning_chime_active = 0;
static uint32_t warning_chime_start_tick = 0;

/* ============================================================
 * Live Debug Variables
 * ============================================================ */

volatile uint8_t live_dbg_mode = 0;
volatile uint8_t live_dbg_throttle = 0;
volatile uint8_t live_dbg_brake = 0;
volatile uint16_t live_dbg_rpm = ENGINE_RPM_IDLE;
volatile uint16_t live_dbg_speed = 0;
volatile uint8_t live_dbg_coolant = ENGINE_COOLANT_INIT;
volatile uint32_t live_dbg_tx_count = 0;

/* ============================================================
 * Private Function Prototypes
 * ============================================================ */

static void EngineSim_UpdateLiveDebug(void);

static uint8_t EngineSim_AdcToPercent(uint16_t adc);
static uint16_t EngineSim_LowPassAdc(uint16_t filtered, uint16_t raw);
static uint8_t EngineSim_MapPedalPercent(uint8_t percent, uint8_t deadzone);
static uint8_t EngineSim_BrakeCurve(uint8_t brake);

static void EngineSim_UpdateInput(void);
static void EngineSim_UpdatePhysics(void);
static void EngineSim_UpdateCoolant(void);

static void EngineSim_PutU16LE(uint8_t* data, uint8_t idx, uint16_t value);

static uint16_t EngineSim_EncodeRpmRaw(uint16_t rpm);
static uint8_t EngineSim_EncodeSpeed5A0Value(uint16_t speed);
static uint8_t EngineSim_MapCoolantToClusterValue(uint8_t coolant);
static uint8_t EngineSim_EncodeCoolantRaw(uint8_t coolant);
static uint8_t EngineSim_GetWarningFlags(void);
static void EngineSim_UpdateWarningChime(uint32_t now);

static void EngineSim_SendClusterRpm(void);
static void EngineSim_SendIgnOnStatus(void);
static void EngineSim_SendClusterSpeed1A0(void);
static void EngineSim_SendClusterSpeed5A0(void);
static void EngineSim_SendClusterCoolant(void);
static void EngineSim_SendWarningStatus(void);



/* ============================================================
 * Public Functions
 * ============================================================ */

void EngineSim_Init(void)
{
    engine.mode = ENGINE_MODE_ADC;
    engine.throttle = 0;
    engine.brake = 0;
    engine.rpm = ENGINE_RPM_IDLE;
    engine.speed = 0;
    engine.coolant = ENGINE_COOLANT_INIT;
    engine.tx_count = 0;

    adc_filter_init = 0;
    throttle_adc_filtered = 0;
    brake_adc_filtered = 0;
    warning_alive = 0;
    speed_1a0_alive = 0;
    warning_prev_active = 0;
    warning_chime_active = 0;
    warning_chime_start_tick = 0;

    EngineSim_UpdateLiveDebug();
}

void EngineSim_Reset(void)
{
    engine.mode = ENGINE_MODE_ADC;
    engine.throttle = 0;
    engine.brake = 0;
    engine.rpm = ENGINE_RPM_IDLE;
    engine.speed = 0;
    engine.coolant = ENGINE_COOLANT_INIT;
    engine.tx_count = 0;

    adc_filter_init = 0;
    throttle_adc_filtered = 0;
    brake_adc_filtered = 0;
    warning_alive = 0;
    speed_1a0_alive = 0;
    warning_prev_active = 0;
    warning_chime_active = 0;
    warning_chime_start_tick = 0;

    EngineSim_UpdateLiveDebug();
}

void EngineSim_SetMode(EngineMode_t mode)
{
    if (mode > ENGINE_MODE_UART)
        return;

    engine.mode = mode;

    if (mode == ENGINE_MODE_ADC)
    {
        adc_filter_init = 0;
    }

    EngineSim_UpdateLiveDebug();
}

void EngineSim_SetThrottle(uint8_t throttle)
{
    if (throttle > 100U)
        throttle = 100U;

    engine.throttle = throttle;

    EngineSim_UpdateLiveDebug();
}

void EngineSim_SetBrake(uint8_t brake)
{
    if (brake > 100U)
        brake = 100U;

    engine.brake = brake;

    EngineSim_UpdateLiveDebug();
}

void EngineSim_GetStatus(EngineSimStatus_t* status)
{
    if (status == NULL)
        return;

    status->mode = engine.mode;
    status->throttle = engine.throttle;
    status->brake = engine.brake;
    status->rpm = engine.rpm;
    status->speed = engine.speed;
    status->coolant = engine.coolant;
    status->tx_count = engine.tx_count;
}

const char* EngineSim_GetModeString(EngineMode_t mode)
{
    switch (mode)
    {
    case ENGINE_MODE_ADC:
        return "adc";

    case ENGINE_MODE_UART:
        return "uart";

    default:
        return "unknown";
    }
}

/* ============================================================
 * Private Functions
 * ============================================================ */

static void EngineSim_UpdateLiveDebug(void)
{
    live_dbg_mode = (uint8_t)engine.mode;
    live_dbg_throttle = engine.throttle;
    live_dbg_brake = engine.brake;
    live_dbg_rpm = engine.rpm;
    live_dbg_speed = engine.speed;
    live_dbg_coolant = engine.coolant;
    live_dbg_tx_count = engine.tx_count;
}

static uint8_t EngineSim_AdcToPercent(uint16_t adc)
{
    if (adc > ADC_MAX_VALUE)
        adc = ADC_MAX_VALUE;

    return (uint8_t)(((uint32_t)adc * 100U) / ADC_MAX_VALUE);
}

static uint16_t EngineSim_LowPassAdc(uint16_t filtered, uint16_t raw)
{
    int32_t diff = (int32_t)raw - (int32_t)filtered;
    int32_t next = (int32_t)filtered + (diff / (1 << ADC_FILTER_SHIFT));

    if (next < 0)
        next = 0;

    if (next > ADC_MAX_VALUE)
        next = ADC_MAX_VALUE;

    return (uint16_t)next;
}

static uint8_t EngineSim_MapPedalPercent(uint8_t percent, uint8_t deadzone)
{
    if (percent <= deadzone)
        return 0U;

    if (percent >= PEDAL_MAX_CLAMP_PERCENT)
        return 100U;

    return (uint8_t)(((uint32_t)(percent - deadzone) * 100U) /
        (PEDAL_MAX_CLAMP_PERCENT - deadzone));
}

static uint8_t EngineSim_BrakeCurve(uint8_t brake)
{
    return (uint8_t)(((uint32_t)brake * brake) / 100U);
}

static void EngineSim_UpdateInput(void)
{
    if (engine.mode != ENGINE_MODE_ADC)
    {
        return;
    }

    uint16_t throttle_adc = ADC_ReadThrottle();
    uint16_t brake_adc = ADC_ReadBrake();

    if (adc_filter_init == 0U)
    {
        throttle_adc_filtered = throttle_adc;
        brake_adc_filtered = brake_adc;
        adc_filter_init = 1U;
    }
    else
    {
        throttle_adc_filtered = EngineSim_LowPassAdc(throttle_adc_filtered, throttle_adc);
        brake_adc_filtered = EngineSim_LowPassAdc(brake_adc_filtered, brake_adc);
    }

    uint8_t throttle_raw = EngineSim_AdcToPercent(throttle_adc_filtered);
    uint8_t brake_raw = EngineSim_AdcToPercent(brake_adc_filtered);

    engine.throttle = EngineSim_MapPedalPercent(throttle_raw, THROTTLE_DEADZONE_PERCENT);
    engine.brake = EngineSim_MapPedalPercent(brake_raw, BRAKE_DEADZONE_PERCENT);
}

static void EngineSim_UpdatePhysics(void)
{
    int16_t speed = (int16_t)engine.speed;

    uint8_t brake_effect = EngineSim_BrakeCurve(engine.brake);

    int16_t effective_throttle =
        ((int16_t)engine.throttle * (100 - brake_effect)) / 100;

    if (effective_throttle < 0)
        effective_throttle = 0;

    if (effective_throttle > 100)
        effective_throttle = 100;

    int16_t target_speed =
        (effective_throttle * (int16_t)ENGINE_SPEED_MAX) / 100;

    if (speed < target_speed)
    {
        speed += 1 + (engine.throttle / 30U);
    }
    else if (speed > target_speed)
    {
        speed -= 1 + (brake_effect / 18U);
    }

    if (engine.throttle == 0U && engine.brake == 0U)
    {
        if (speed > 0)
            speed -= 1;
    }

    if (speed < 0)
        speed = 0;

    if (speed > ENGINE_SPEED_MAX)
        speed = ENGINE_SPEED_MAX;

    engine.speed = (uint16_t)speed;

    engine.rpm =
        ENGINE_RPM_IDLE +
        ((uint32_t)engine.speed * (ENGINE_RPM_MAX - ENGINE_RPM_IDLE)) /
        ENGINE_SPEED_MAX;

    if (engine.speed >= ENGINE_SPEED_MAX)
        engine.rpm = ENGINE_RPM_MAX;

    if (engine.rpm > ENGINE_RPM_MAX)
        engine.rpm = ENGINE_RPM_MAX;
}

static void EngineSim_UpdateCoolant(void)
{
    uint32_t rpm;
    uint32_t target_coolant;

    rpm = engine.rpm;

    if (rpm <= ENGINE_COOLANT_RPM_MIN)
    {
        target_coolant = ENGINE_COOLANT_MIN;
    }
    else if (rpm >= ENGINE_COOLANT_RPM_MAX)
    {
        target_coolant = ENGINE_COOLANT_MAX;
    }
    else
    {
        target_coolant =
            ENGINE_COOLANT_MIN +
            (((rpm - ENGINE_COOLANT_RPM_MIN) *
              (ENGINE_COOLANT_MAX - ENGINE_COOLANT_MIN)) /
             (ENGINE_COOLANT_RPM_MAX - ENGINE_COOLANT_RPM_MIN));
    }

    if (engine.coolant < target_coolant)
    {
        engine.coolant++;
    }
    else if (engine.coolant > target_coolant)
    {
        engine.coolant--;
    }
}


/* ============================================================
 * CAN Encoding Helpers
 * ============================================================ */

static void EngineSim_PutU16LE(uint8_t* data, uint8_t idx, uint16_t value)
{
    data[idx] = (uint8_t)(value & 0xFFU);
    data[idx + 1U] = (uint8_t)((value >> 8) & 0xFFU);
}

static uint16_t EngineSim_EncodeRpmRaw(uint16_t rpm)
{
    /*
     * 0x280
     * byte[2]~byte[3] : RPM raw
     *
     * 일반적으로 VW 계열에서 RPM raw = rpm * 4 형태가 자주 쓰임.
     * 예: 800rpm  -> 3200  -> 0x0C80
     *     6000rpm -> 24000 -> 0x5DC0
     */
    return (uint16_t)(rpm * 4U);
}

static uint16_t EngineSim_EncodeSpeed1A0Raw(uint16_t speed)
{
    /*
     * 0x1A0
     * 예시 프레임:
     * 08 00 20 4E 00 00 00 00
     *
     * byte[2]~byte[3] = speed * 80
     *
     * Gateway/ADAS/Qt monitor all decode this frame with raw / 80.
     */
    return (uint16_t)(speed * 80U);
    return (uint16_t)(speed * 80U);
}

static uint8_t EngineSim_EncodeSpeed5A0Value(uint16_t speed)
{
    /*
     * 0x5A0
     * byte[2] : 속도계 바늘 값
     *
     * 현재 계기판은 이 값을 2km/h 단위처럼 표시하므로
     * 시뮬레이션 speed의 절반 값을 넣음.
     */
    speed /= 2U;

    return (uint8_t)speed;
}

static uint8_t EngineSim_MapCoolantToClusterValue(uint8_t coolant)
{
    if (coolant <= ENGINE_COOLANT_CAN_MIN)
    {
        return ENGINE_COOLANT_CAN_MIN;
    }

    if (coolant >= ENGINE_COOLANT_CAN_MAX)
    {
        return ENGINE_COOLANT_CAN_MAX;
    }

    return coolant;
}

static uint8_t EngineSim_EncodeCoolantRaw(uint8_t coolant)
{
    uint8_t cluster_coolant = EngineSim_MapCoolantToClusterValue(coolant);
    uint32_t raw = (((uint32_t)cluster_coolant + 48U) * 4U) / 3U;

    if (raw > 0xFFU)
    {
        raw = 0xFFU;
    }

    return (uint8_t)raw;
}

static uint8_t EngineSim_GetWarningFlags(void)
{
    uint8_t flags = 0U;

    if (engine.rpm >= ENGINE_RPM_WARN_THRESHOLD)
    {
        flags |= CAN_ENGINE_WARNING_RPM_MASK;
    }

    if (engine.coolant >= ENGINE_COOLANT_WARN_THRESHOLD)
    {
        flags |= CAN_ENGINE_WARNING_COOLANT_MASK;
    }

    if (flags != 0U)
    {
        flags |= CAN_ENGINE_WARNING_GENERAL_MASK;
    }

    return flags;
}

static void EngineSim_UpdateWarningChime(uint32_t now)
{
    uint8_t warning_active = (EngineSim_GetWarningFlags() != 0U) ? 1U : 0U;

    if ((warning_active != 0U) && (warning_prev_active == 0U))
    {
        warning_chime_active = 1U;
        warning_chime_start_tick = now;
    }

    warning_prev_active = warning_active;

    if ((warning_chime_active != 0U) &&
        ((now - warning_chime_start_tick) >= ENG_WARNING_CHIME_DURATION_MS))
    {
        warning_chime_active = 0U;
    }
}

/* ============================================================
 * CAN TX Functions
 * ============================================================ */

static void EngineSim_SendClusterRpm(void)
{
    uint8_t data[CAN_CLUSTER_DLC] = {
        0x00, 0x00, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00
    };

    uint16_t rpm_raw = EngineSim_EncodeRpmRaw(engine.rpm);

    EngineSim_PutU16LE(data, CAN_RPM_RAW_L_IDX, rpm_raw);

    HAL_StatusTypeDef ret = CAN_BSP_Send(CAN_ID_CLUSTER_RPM,
        data,
        CAN_CLUSTER_DLC);

    if (ret == HAL_OK)
    {
        engine.tx_count++;
    }
}

static void EngineSim_SendIgnOnStatus(void)
{
    uint8_t data[CAN_CLUSTER_DLC] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    data[CAN_IGN_STATUS_IDX] = CAN_IGN_STATUS_IGN_ON_MASK;

    HAL_StatusTypeDef ret = CAN_BSP_Send(CAN_ID_CLUSTER_IGN_STATUS,
        data,
        CAN_CLUSTER_DLC);

    if (ret == HAL_OK)
    {
        engine.tx_count++;
    }
}

static void EngineSim_SendClusterSpeed1A0(void)
{
    uint8_t data[CAN_CLUSTER_DLC] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    HAL_StatusTypeDef ret = CAN_BSP_Send(CAN_ID_CLUSTER_SPEED_1A0,
        data,
        CAN_CLUSTER_DLC);

    if (ret == HAL_OK)
    {
        engine.tx_count++;
    }
}

static void EngineSim_SendClusterSpeed5A0(void)
{
    uint8_t data[CAN_CLUSTER_DLC] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    data[CAN_SPEED_5A0_VALUE_IDX] = EngineSim_EncodeSpeed5A0Value(engine.speed);
    if (warning_chime_active != 0U)
    {
        data[CAN_SPEED_5A0_WARNING_CHIME_IDX] |= CAN_SPEED_5A0_WARNING_CHIME_MASK;
    }

    HAL_StatusTypeDef ret = CAN_BSP_Send(CAN_ID_CLUSTER_SPEED_5A0,
        data,
        CAN_CLUSTER_DLC);

    if (ret == HAL_OK)
    {
        engine.tx_count++;
    }
}

static void EngineSim_SendClusterCoolant(void)
{
    uint8_t data[CAN_CLUSTER_DLC] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    data[CAN_COOLANT_VALUE_IDX] = EngineSim_EncodeCoolantRaw(engine.coolant);

    HAL_StatusTypeDef ret = CAN_BSP_Send(CAN_ID_CLUSTER_COOLANT,
        data,
        CAN_CLUSTER_DLC);

    if (ret == HAL_OK)
    {
        engine.tx_count++;
    }
}

static void EngineSim_SendWarningStatus(void)
{
    uint8_t data[CAN_ENGINE_WARNING_DLC] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    data[CAN_ENGINE_WARNING_FLAGS_IDX] = EngineSim_GetWarningFlags();
    data[CAN_ENGINE_WARNING_COOLANT_IDX] = engine.coolant;
    data[CAN_ENGINE_WARNING_ALIVE_IDX] = warning_alive++;

    EngineSim_PutU16LE(data, CAN_ENGINE_WARNING_RPM_L_IDX, engine.rpm);

    HAL_StatusTypeDef ret = CAN_BSP_Send(CAN_ID_ENGINE_WARNING_STATUS,
        data,
        CAN_ENGINE_WARNING_DLC);

    if (ret == HAL_OK)
    {
        engine.tx_count++;
    }
}

/* ============================================================
 * FreeRTOS Task
 * ============================================================ */

void EngineSim_Task(void* argument)
{
    (void)argument;

    uint32_t now = osKernelGetTickCount();

    uint32_t t_rpm_tx = now - ENG_PERIOD_RPM_TX_MS;
    uint32_t t_ign_tx = now - (ENG_PERIOD_IGN_TX_MS - 20U);
    uint32_t t_speed_1a0_tx = now - (ENG_PERIOD_SPEED_1A0_TX_MS - 5U);
    uint32_t t_speed_5a0_tx = now - (ENG_PERIOD_SPEED_5A0_TX_MS - 10U);
    uint32_t t_coolant_tx = now - (ENG_PERIOD_COOLANT_TX_MS - 15U);
    uint32_t t_warning_tx = now - (ENG_PERIOD_WARNING_TX_MS - 25U);

    uint32_t t_model = now - ENG_PERIOD_MODEL_MS;
    uint32_t t_coolant_model = now - ENG_PERIOD_COOLANT_MODEL_MS;

    for (;;)
    {
        now = osKernelGetTickCount();

        EngineSim_UpdateInput();

        if ((now - t_model) >= ENG_PERIOD_MODEL_MS)
        {
            EngineSim_UpdatePhysics();
            t_model = now;
        }

        if ((now - t_coolant_model) >= ENG_PERIOD_COOLANT_MODEL_MS)
        {
            EngineSim_UpdateCoolant();
            t_coolant_model = now;
        }

        EngineSim_UpdateWarningChime(now);

        if ((now - t_rpm_tx) >= ENG_PERIOD_RPM_TX_MS)
        {
            EngineSim_SendClusterRpm();
            t_rpm_tx = now;
        }

        if ((now - t_ign_tx) >= ENG_PERIOD_IGN_TX_MS)
        {
            EngineSim_SendIgnOnStatus();
            t_ign_tx = now;
        }

        if ((now - t_speed_1a0_tx) >= ENG_PERIOD_SPEED_1A0_TX_MS)
        {
            EngineSim_SendClusterSpeed1A0();
            t_speed_1a0_tx = now;
        }

        if ((now - t_speed_5a0_tx) >= ENG_PERIOD_SPEED_5A0_TX_MS)
        {
            EngineSim_SendClusterSpeed5A0();
            t_speed_5a0_tx = now;
        }

        if ((now - t_coolant_tx) >= ENG_PERIOD_COOLANT_TX_MS)
        {
            EngineSim_SendClusterCoolant();
            t_coolant_tx = now;
        }

        if ((now - t_warning_tx) >= ENG_PERIOD_WARNING_TX_MS)
        {
            EngineSim_SendWarningStatus();
            t_warning_tx = now;
        }

        EngineSim_UpdateLiveDebug();

        osDelay(5);
    }
}
