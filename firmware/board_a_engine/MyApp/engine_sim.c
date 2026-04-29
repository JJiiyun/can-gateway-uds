#include "engine_sim.h"

/* ============================================================
 * Period
 * ============================================================ */

#define ENG_PERIOD_ENGINE_DATA_MS       50U
#define ENG_PERIOD_MODEL_MS             50U
#define ENG_PERIOD_COOLANT_MODEL_MS     1000U

/* ============================================================
 * Engine Model Config
 * ============================================================ */

#define ENGINE_RPM_IDLE                 800U
#define ENGINE_RPM_MAX                  6000U
#define ENGINE_SPEED_MAX                130U

/* ============================================================
 * Coolant Model Config
 * ============================================================ */

#define ENGINE_COOLANT_INIT             70U
#define ENGINE_COOLANT_MAX              115U

/* ============================================================
 * ADC / Pedal Config
 * ============================================================ */

#define ADC_MAX_VALUE                   4095U
#define ADC_FILTER_SHIFT                3U      // 1/8 low-pass filter

#define THROTTLE_DEADZONE_PERCENT       3U
#define BRAKE_DEADZONE_PERCENT          8U

#define PEDAL_MAX_CLAMP_PERCENT         97U

/*
 * 실험용 브레이크 민감도.
 * 실제 차처럼 brake가 throttle을 완전히 이기게 하지 않고,
 * brake 입력의 일부만 감속 모델에 반영한다.
 *
 * 35%면 brake=100일 때도 effective_brake=35로 계산됨.
 */
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

/* ============================================================
 * Public Functions
 * ============================================================ */

void EngineSim_Init(void)
{
    CAN_BSP_Init();

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
}

void EngineSim_SetThrottle(uint8_t throttle)
{
    if (throttle > 100U)
        throttle = 100U;

    engine.throttle = throttle;
}

void EngineSim_SetBrake(uint8_t brake)
{
    if (brake > 100U)
        brake = 100U;

    engine.brake = brake;
}

void EngineSim_GetStatus(EngineSimStatus_t *status)
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

const char *EngineSim_GetModeString(EngineMode_t mode)
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
    /*
     * brake를 곡선형으로 변환.
     *
     * brake 10  -> 1
     * brake 30  -> 9
     * brake 50  -> 25
     * brake 80  -> 64
     * brake 100 -> 100
     */
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

    /*
     * 가변저항은 실제 페달처럼 정확히 0/100에 잘 안 붙기 때문에
     * deadzone과 max clamp를 적용한다.
     */
    engine.throttle = EngineSim_MapPedalPercent(throttle_raw, THROTTLE_DEADZONE_PERCENT);
    engine.brake = EngineSim_MapPedalPercent(brake_raw, BRAKE_DEADZONE_PERCENT);
}

static void EngineSim_UpdatePhysics(void)
{
    int16_t speed = (int16_t)engine.speed;

    /*
     * brake는 입력값 그대로 쓰지 않고 곡선형으로 변환한다.
     *
     * 이유:
     * - 살짝 들어온 brake는 노이즈/약한 제동으로 취급
     * - 강하게 들어온 brake는 throttle을 확실히 이김
     * - brake 100이면 throttle 100이어도 target_speed = 0
     */
    uint8_t brake_effect = EngineSim_BrakeCurve(engine.brake);

    /*
     * brake가 throttle을 점진적으로 깎는 구조.
     *
     * brake_effect = 0   -> throttle 그대로
     * brake_effect = 50  -> throttle 절반만 유효
     * brake_effect = 100 -> throttle 완전 무효
     */
    int16_t effective_throttle =
        ((int16_t)engine.throttle * (100 - brake_effect)) / 100;

    if (effective_throttle < 0)
        effective_throttle = 0;

    if (effective_throttle > 100)
        effective_throttle = 100;

    int16_t target_speed =
        (effective_throttle * (int16_t)ENGINE_SPEED_MAX) / 100;

    /*
     * 가속:
     * throttle이 클수록 목표 속도까지 더 빠르게 접근.
     */
    if (speed < target_speed)
    {
        speed += 1 + (engine.throttle / 30U);
    }
    /*
     * 감속:
     * brake_effect가 클수록 강하게 감속.
     */
    else if (speed > target_speed)
    {
        speed -= 1 + (brake_effect / 18U);
    }

    /*
     * 페달 둘 다 놓았을 때 자연 감속.
     */
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

    /*
     * speed 0   -> 800 rpm
     * speed 130 -> 6000 rpm
     *
     * throttle 100%, brake 0이면 최종적으로 speed 130에 도달하고
     * rpm은 정확히 6000까지 올라간다.
     */
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
    uint8_t target_coolant;

    /*
     * 냉각수 온도 모델
     *
     * 실제 냉각수는 RPM처럼 즉시 오르내리지 않음.
     * 그래서 목표 온도만 rpm/throttle 기준으로 잡고,
     * 실제 coolant 값은 1초마다 1도씩 천천히 따라가게 만든다.
     *
     * 대략적인 의미:
     * - 초기/저부하: 70~85도
     * - 일반 주행: 90도 근처
     * - 고RPM/고부하: 100~110도 근처
     */
    if (engine.rpm < 1500U)
    {
        target_coolant = 85U;
    }
    else if (engine.rpm < 3500U)
    {
        target_coolant = 90U;
    }
    else if (engine.rpm < 5000U)
    {
        target_coolant = 98U;
    }
    else
    {
        target_coolant = 105U;
    }

    if (engine.throttle > 80U)
    {
        target_coolant += 5U;
    }

    if (target_coolant > ENGINE_COOLANT_MAX)
    {
        target_coolant = ENGINE_COOLANT_MAX;
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

static void EngineSim_SendEngineData(void)
{
    static uint8_t alive = 0;
    uint8_t data[CAN_ENGINE_DATA_DLC] = {0};

    CAN_PutU16LE(data, CAN_ENGINE_DATA_RPM_IDX, engine.rpm);
    CAN_PutU16LE(data, CAN_ENGINE_DATA_SPEED_IDX, engine.speed);

    data[CAN_ENGINE_DATA_COOLANT_IDX] = engine.coolant;

    /*
     * byte5 Status
     *
     * bit0   : IGN ON
     * bit1~7 : Alive Counter
     */
    data[CAN_ENGINE_DATA_STATUS_IDX] =
        ((alive << CAN_ENGINE_STATUS_ALIVE_SHIFT) & CAN_ENGINE_STATUS_ALIVE_MASK)
        | CAN_ENGINE_STATUS_IGN_MASK;

    data[CAN_ENGINE_DATA_RESERVED0_IDX] = 0U;
    data[CAN_ENGINE_DATA_RESERVED1_IDX] = 0U;

    alive++;

    HAL_StatusTypeDef ret = CAN_BSP_Send(CAN_ID_ENGINE_DATA,
                                         data,
                                         CAN_ENGINE_DATA_DLC);

    if (ret == HAL_OK)
    {
        engine.tx_count++;
    }
}

/* ============================================================
 * FreeRTOS Task
 * ============================================================ */

void EngineSim_Task(void *argument)
{
    (void)argument;

    uint32_t now = osKernelGetTickCount();

    uint32_t t_engine = now - ENG_PERIOD_ENGINE_DATA_MS;
    uint32_t t_model = now - ENG_PERIOD_MODEL_MS;
    uint32_t t_coolant = now - ENG_PERIOD_COOLANT_MODEL_MS;

    for (;;)
    {
        now = osKernelGetTickCount();

        EngineSim_UpdateInput();

        if ((now - t_model) >= ENG_PERIOD_MODEL_MS)
        {
            EngineSim_UpdatePhysics();
            t_model = now;
        }

        if ((now - t_coolant) >= ENG_PERIOD_COOLANT_MODEL_MS)
        {
            EngineSim_UpdateCoolant();
            t_coolant = now;
        }

        if ((now - t_engine) >= ENG_PERIOD_ENGINE_DATA_MS)
        {
            EngineSim_SendEngineData();
            t_engine = now;
        }

        osDelay(5);
    }
}