#include "adc_driver.h"

extern ADC_HandleTypeDef hadc1;

/* ===== 내부 공통 함수 ===== */
static uint16_t ADC_ReadChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;

    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        return 0;
    }

    uint16_t value = (uint16_t)HAL_ADC_GetValue(&hadc1);

    HAL_ADC_Stop(&hadc1);

    return value;
}

/* ===== Throttle (PA0 / ADC1_IN0) ===== */
uint16_t ADC_ReadThrottle(void)
{
    uint16_t value = ADC_ReadChannel(ADC_CHANNEL_0);

    /* 초록 LED (LD1) */
    if (value > 500)
        HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);

    return value;
}

/* ===== Brake (PA1 / ADC1_IN3) ===== */
uint16_t ADC_ReadBrake(void)
{
    uint16_t value = ADC_ReadChannel(ADC_CHANNEL_3);

    /* 빨간 LED (LD3) */
    if (value > 500)
        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

    return value;
}