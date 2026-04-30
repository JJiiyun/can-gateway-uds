#include "adas_input.h"

#include "adc.h"
#include "main.h"

#include <string.h>

extern ADC_HandleTypeDef hadc1;

#ifndef ADAS_FRONT_ADC_CHANNEL
#define ADAS_FRONT_ADC_CHANNEL ADC_CHANNEL_0
#endif

#ifndef ADAS_REAR_ADC_CHANNEL
#define ADAS_REAR_ADC_CHANNEL ADC_CHANNEL_3
#endif

#ifndef ADAS_FRONT_ADC_GPIO_Port
#define ADAS_FRONT_ADC_GPIO_Port GPIOA
#define ADAS_FRONT_ADC_Pin       GPIO_PIN_0
#endif

#ifndef ADAS_REAR_ADC_GPIO_Port
#define ADAS_REAR_ADC_GPIO_Port  GPIOA
#define ADAS_REAR_ADC_Pin        GPIO_PIN_3
#endif

#ifndef ADAS_LANE_GPIO_Port
#define ADAS_LANE_GPIO_Port      GPIOE
#define ADAS_LANE_Pin            GPIO_PIN_6
#endif

#ifndef ADAS_BRAKE_GPIO_Port
#define ADAS_BRAKE_GPIO_Port     GPIOF
#define ADAS_BRAKE_Pin           GPIO_PIN_6
#endif

#ifndef ADAS_SENSOR_FAULT_GPIO_Port
#define ADAS_SENSOR_FAULT_GPIO_Port GPIOF
#define ADAS_SENSOR_FAULT_Pin       GPIO_PIN_7
#endif

#ifndef ADAS_INPUT_ACTIVE_STATE
#define ADAS_INPUT_ACTIVE_STATE GPIO_PIN_RESET
#endif

#ifndef ADAS_ADC_DISTANCE_CLOSE_IS_HIGH
#define ADAS_ADC_DISTANCE_CLOSE_IS_HIGH 1
#endif

#define ADAS_ADC_MAX_VALUE       4095U
#define ADAS_DISTANCE_MIN_CM     5U
#define ADAS_DISTANCE_MAX_CM     250U

static volatile AdasInputState_t s_state;

static uint8_t read_button(GPIO_TypeDef *port, uint16_t pin)
{
    return HAL_GPIO_ReadPin(port, pin) == ADAS_INPUT_ACTIVE_STATE ? 1U : 0U;
}

static uint16_t read_adc_channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef config = {0};

    config.Channel = channel;
    config.Rank = 1U;
    config.SamplingTime = ADC_SAMPLETIME_15CYCLES;

    if (HAL_ADC_ConfigChannel(&hadc1, &config) != HAL_OK) {
        return 0U;
    }

    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return 0U;
    }

    if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK) {
        (void)HAL_ADC_Stop(&hadc1);
        return 0U;
    }

    uint16_t value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    (void)HAL_ADC_Stop(&hadc1);
    return value;
}

static uint8_t adc_to_distance_cm(uint16_t adc)
{
    uint32_t span = ADAS_DISTANCE_MAX_CM - ADAS_DISTANCE_MIN_CM;
    uint32_t scaled;

    if (adc > ADAS_ADC_MAX_VALUE) {
        adc = ADAS_ADC_MAX_VALUE;
    }

#if ADAS_ADC_DISTANCE_CLOSE_IS_HIGH
    scaled = ADAS_DISTANCE_MAX_CM -
             (((uint32_t)adc * span) / ADAS_ADC_MAX_VALUE);
#else
    scaled = ADAS_DISTANCE_MIN_CM +
             (((uint32_t)adc * span) / ADAS_ADC_MAX_VALUE);
#endif

    if (scaled < ADAS_DISTANCE_MIN_CM) {
        scaled = ADAS_DISTANCE_MIN_CM;
    }
    if (scaled > ADAS_DISTANCE_MAX_CM) {
        scaled = ADAS_DISTANCE_MAX_CM;
    }

    return (uint8_t)scaled;
}

void ADAS_Input_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = ADAS_FRONT_ADC_Pin;
    HAL_GPIO_Init(ADAS_FRONT_ADC_GPIO_Port, &gpio);
    gpio.Pin = ADAS_REAR_ADC_Pin;
    HAL_GPIO_Init(ADAS_REAR_ADC_GPIO_Port, &gpio);

    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Pin = ADAS_LANE_Pin;
    HAL_GPIO_Init(ADAS_LANE_GPIO_Port, &gpio);
    gpio.Pin = ADAS_BRAKE_Pin;
    HAL_GPIO_Init(ADAS_BRAKE_GPIO_Port, &gpio);
    gpio.Pin = ADAS_SENSOR_FAULT_Pin;
    HAL_GPIO_Init(ADAS_SENSOR_FAULT_GPIO_Port, &gpio);

    memset((void *)&s_state, 0, sizeof(s_state));
    s_state.front_distance_cm = ADAS_DISTANCE_MAX_CM;
    s_state.rear_distance_cm = ADAS_DISTANCE_MAX_CM;
}

void ADAS_Input_Poll(void)
{
    AdasInputState_t next = s_state;

    next.front_adc_raw = read_adc_channel(ADAS_FRONT_ADC_CHANNEL);
    next.rear_adc_raw = read_adc_channel(ADAS_REAR_ADC_CHANNEL);
    next.front_distance_cm = adc_to_distance_cm(next.front_adc_raw);
    next.rear_distance_cm = adc_to_distance_cm(next.rear_adc_raw);
    next.lane_departure = read_button(ADAS_LANE_GPIO_Port, ADAS_LANE_Pin);
    next.harsh_brake = read_button(ADAS_BRAKE_GPIO_Port, ADAS_BRAKE_Pin);
    next.sensor_fault = read_button(ADAS_SENSOR_FAULT_GPIO_Port,
                                    ADAS_SENSOR_FAULT_Pin);

    s_state = next;
}

void ADAS_Input_GetState(AdasInputState_t *out_state)
{
    if (out_state != NULL) {
        *out_state = s_state;
    }
}
