#include "adas_input.h"

#include "main.h"

#include <string.h>

#define ADAS_DISTANCE_MIN_CM 5U
#define ADAS_DISTANCE_MAX_CM 250U

#ifndef ADAS_HCSR04_FRONT_TRIG_GPIO_Port
#define ADAS_HCSR04_FRONT_TRIG_GPIO_Port GPIOF
#define ADAS_HCSR04_FRONT_TRIG_Pin       GPIO_PIN_15
#endif

#ifndef ADAS_HCSR04_FRONT_ECHO_GPIO_Port
#define ADAS_HCSR04_FRONT_ECHO_GPIO_Port GPIOF
#define ADAS_HCSR04_FRONT_ECHO_Pin       GPIO_PIN_14
#endif

#ifndef ADAS_HCSR04_REAR_TRIG_GPIO_Port
#define ADAS_HCSR04_REAR_TRIG_GPIO_Port  GPIOE
#define ADAS_HCSR04_REAR_TRIG_Pin        GPIO_PIN_11
#endif

#ifndef ADAS_HCSR04_REAR_ECHO_GPIO_Port
#define ADAS_HCSR04_REAR_ECHO_GPIO_Port  GPIOE
#define ADAS_HCSR04_REAR_ECHO_Pin        GPIO_PIN_9
#endif

#ifndef ADAS_HCSR04_ECHO_START_TIMEOUT_US
#define ADAS_HCSR04_ECHO_START_TIMEOUT_US 3000U
#endif

#ifndef ADAS_HCSR04_ECHO_TIMEOUT_US
#define ADAS_HCSR04_ECHO_TIMEOUT_US       16000U
#endif

typedef struct {
    GPIO_TypeDef *trig_port;
    uint16_t trig_pin;
    GPIO_TypeDef *echo_port;
    uint16_t echo_pin;
} Hcsr04Sensor_t;

static const Hcsr04Sensor_t s_front_sensor = {
    ADAS_HCSR04_FRONT_TRIG_GPIO_Port,
    ADAS_HCSR04_FRONT_TRIG_Pin,
    ADAS_HCSR04_FRONT_ECHO_GPIO_Port,
    ADAS_HCSR04_FRONT_ECHO_Pin,
};

static const Hcsr04Sensor_t s_rear_sensor = {
    ADAS_HCSR04_REAR_TRIG_GPIO_Port,
    ADAS_HCSR04_REAR_TRIG_Pin,
    ADAS_HCSR04_REAR_ECHO_GPIO_Port,
    ADAS_HCSR04_REAR_ECHO_Pin,
};

static volatile AdasInputState_t s_state;

static void enable_gpio_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    } else if (port == GPIOF) {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    } else if (port == GPIOG) {
        __HAL_RCC_GPIOG_CLK_ENABLE();
    } else if (port == GPIOH) {
        __HAL_RCC_GPIOH_CLK_ENABLE();
    }
#ifdef GPIOI
    else if (port == GPIOI) {
        __HAL_RCC_GPIOI_CLK_ENABLE();
    }
#endif
}

static uint32_t cycles_per_us(void)
{
    uint32_t cycles = SystemCoreClock / 1000000U;

    return cycles == 0U ? 1U : cycles;
}

static void dwt_delay_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t wait_cycles = us * cycles_per_us();

    while ((uint32_t)(DWT->CYCCNT - start) < wait_cycles) {
        __NOP();
    }
}

static uint8_t wait_echo_state(GPIO_TypeDef *port,
                               uint16_t pin,
                               GPIO_PinState state,
                               uint32_t timeout_us,
                               uint32_t *elapsed_us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t per_us = cycles_per_us();
    uint32_t timeout_cycles = timeout_us * per_us;

    while (HAL_GPIO_ReadPin(port, pin) != state) {
        if ((uint32_t)(DWT->CYCCNT - start) >= timeout_cycles) {
            return 0U;
        }
    }

    if (elapsed_us != NULL) {
        *elapsed_us = (uint32_t)(DWT->CYCCNT - start) / per_us;
    }
    return 1U;
}

static uint8_t clamp_distance_cm(uint32_t cm)
{
    if (cm < ADAS_DISTANCE_MIN_CM) {
        return ADAS_DISTANCE_MIN_CM;
    }
    if (cm > ADAS_DISTANCE_MAX_CM) {
        return ADAS_DISTANCE_MAX_CM;
    }
    return (uint8_t)cm;
}

static void init_hcsr04_gpio(const Hcsr04Sensor_t *sensor)
{
    GPIO_InitTypeDef gpio = {0};

    enable_gpio_clock(sensor->trig_port);
    enable_gpio_clock(sensor->echo_port);

    HAL_GPIO_WritePin(sensor->trig_port, sensor->trig_pin, GPIO_PIN_RESET);

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pin = sensor->trig_pin;
    HAL_GPIO_Init(sensor->trig_port, &gpio);

    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Pin = sensor->echo_pin;
    HAL_GPIO_Init(sensor->echo_port, &gpio);
}

static uint8_t measure_hcsr04_cm(const Hcsr04Sensor_t *sensor,
                                 uint8_t *out_cm,
                                 uint16_t *out_echo_us)
{
    uint32_t pulse_us = 0U;

    if (sensor == NULL || out_cm == NULL || out_echo_us == NULL) {
        return 0U;
    }

    HAL_GPIO_WritePin(sensor->trig_port, sensor->trig_pin, GPIO_PIN_RESET);
    delay_us(2U);
    HAL_GPIO_WritePin(sensor->trig_port, sensor->trig_pin, GPIO_PIN_SET);
    delay_us(10U);
    HAL_GPIO_WritePin(sensor->trig_port, sensor->trig_pin, GPIO_PIN_RESET);

    if (!wait_echo_state(sensor->echo_port,
                         sensor->echo_pin,
                         GPIO_PIN_SET,
                         ADAS_HCSR04_ECHO_START_TIMEOUT_US,
                         NULL)) {
        *out_cm = ADAS_DISTANCE_MAX_CM;
        *out_echo_us = 0U;
        return 0U;
    }

    if (!wait_echo_state(sensor->echo_port,
                         sensor->echo_pin,
                         GPIO_PIN_RESET,
                         ADAS_HCSR04_ECHO_TIMEOUT_US,
                         &pulse_us)) {
        *out_cm = ADAS_DISTANCE_MAX_CM;
        *out_echo_us = 0U;
        return 0U;
    }

    *out_echo_us = pulse_us > 0xFFFFU ? 0xFFFFU : (uint16_t)pulse_us;
    *out_cm = clamp_distance_cm((pulse_us + 29U) / 58U);
    return 1U;
}

void ADAS_Input_Init(void)
{
    dwt_delay_init();
    init_hcsr04_gpio(&s_front_sensor);
    init_hcsr04_gpio(&s_rear_sensor);

    memset((void *)&s_state, 0, sizeof(s_state));
    s_state.front_distance_cm = ADAS_DISTANCE_MAX_CM;
    s_state.rear_distance_cm = ADAS_DISTANCE_MAX_CM;
}

void ADAS_Input_Poll(void)
{
    AdasInputState_t next = s_state;
    uint8_t front_ok;
    uint8_t rear_ok;

    front_ok = measure_hcsr04_cm(&s_front_sensor,
                                 &next.front_distance_cm,
                                 &next.front_echo_us);
    rear_ok = measure_hcsr04_cm(&s_rear_sensor,
                                &next.rear_distance_cm,
                                &next.rear_echo_us);
    next.sensor_fault = (front_ok && rear_ok) ? 0U : 1U;

    s_state = next;
}

void ADAS_Input_GetState(AdasInputState_t *out_state)
{
    if (out_state != NULL) {
        *out_state = s_state;
    }
}
