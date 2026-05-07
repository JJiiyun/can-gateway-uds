/**
 * @file    bcm_input.c
 * @brief   BCM turn-signal button input handling.
 */

#include "bcm_input.h"
#include "main.h"
#include <string.h>

/* Inputs use pull-up wiring: active switch/button shorts the pin to GND. */
#ifndef BCM_INPUT_ACTIVE_STATE
#define BCM_INPUT_ACTIVE_STATE GPIO_PIN_RESET
#endif

#if !defined(BCM_TURN_LEFT_GPIO_Port) && (defined(STM32F103xB) || defined(STM32F103RB))
#define BCM_TURN_LEFT_GPIO_Port    GPIOB
#define BCM_TURN_LEFT_Pin          GPIO_PIN_11
#define BCM_TURN_RIGHT_GPIO_Port   GPIOB
#define BCM_TURN_RIGHT_Pin         GPIO_PIN_12
#endif

#ifndef BCM_TURN_LEFT_GPIO_Port
#define BCM_TURN_LEFT_GPIO_Port    GPIOE
#define BCM_TURN_LEFT_Pin          GPIO_PIN_6
#endif
#ifndef BCM_TURN_RIGHT_GPIO_Port
#define BCM_TURN_RIGHT_GPIO_Port   GPIOF
#define BCM_TURN_RIGHT_Pin         GPIO_PIN_6
#endif
#ifndef BCM_HAZARD_GPIO_Port
#define BCM_HAZARD_GPIO_Port       GPIOE
#define BCM_HAZARD_Pin             GPIO_PIN_7
#endif

#define BCM_BUTTON_DEBOUNCE_SAMPLES 2U

typedef struct {
    uint8_t raw_active;
    uint8_t stable_active;
    uint8_t same_count;
} BcmButton_Debounce_t;

static volatile BcmInput_State_t s_state;
static volatile BcmInput_Mode_t s_mode = BCM_INPUT_MODE_GPIO;
static BcmButton_Debounce_t s_left_button;
static BcmButton_Debounce_t s_right_button;
static BcmButton_Debounce_t s_hazard_button;

static uint8_t read_input(GPIO_TypeDef *port, uint16_t pin)
{
    return HAL_GPIO_ReadPin(port, pin) == BCM_INPUT_ACTIVE_STATE ? 1U : 0U;
}

static uint8_t debounce_pressed(BcmButton_Debounce_t *button, uint8_t raw_active)
{
    uint8_t pressed = 0U;

    if (raw_active == button->raw_active) {
        if (button->same_count < BCM_BUTTON_DEBOUNCE_SAMPLES) {
            button->same_count++;
        }
    } else {
        button->raw_active = raw_active;
        button->same_count = 0U;
    }

    if (button->same_count >= BCM_BUTTON_DEBOUNCE_SAMPLES &&
        raw_active != button->stable_active) {
        button->stable_active = raw_active;
        if (button->stable_active) {
            pressed = 1U;
        }
    }

    return pressed;
}

void BCM_Input_Init(void)
{
    GPIO_InitTypeDef gpio;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
#ifdef __HAL_RCC_GPIOE_CLK_ENABLE
    __HAL_RCC_GPIOE_CLK_ENABLE();
#endif
#ifdef __HAL_RCC_GPIOF_CLK_ENABLE
    __HAL_RCC_GPIOF_CLK_ENABLE();
#endif
#ifdef __HAL_RCC_GPIOG_CLK_ENABLE
    __HAL_RCC_GPIOG_CLK_ENABLE();
#endif

    memset(&gpio, 0, sizeof(gpio));
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = BCM_TURN_LEFT_Pin;
    HAL_GPIO_Init(BCM_TURN_LEFT_GPIO_Port, &gpio);
    gpio.Pin = BCM_TURN_RIGHT_Pin;
    HAL_GPIO_Init(BCM_TURN_RIGHT_GPIO_Port, &gpio);
    gpio.Pin = BCM_HAZARD_Pin;
    HAL_GPIO_Init(BCM_HAZARD_GPIO_Port, &gpio);

    memset((void *)&s_state, 0, sizeof(s_state));
    s_mode = BCM_INPUT_MODE_GPIO;
    memset(&s_left_button, 0, sizeof(s_left_button));
    memset(&s_right_button, 0, sizeof(s_right_button));
    memset(&s_hazard_button, 0, sizeof(s_hazard_button));
}

void BCM_Input_Poll(void)
{
    if (s_mode != BCM_INPUT_MODE_GPIO) {
        return;
    }

    BcmInput_State_t state = s_state;

    if (debounce_pressed(&s_left_button,
                         read_input(BCM_TURN_LEFT_GPIO_Port, BCM_TURN_LEFT_Pin))) {
        state.turn_left_enabled = state.turn_left_enabled ? 0U : 1U;
    }

    if (debounce_pressed(&s_right_button,
                         read_input(BCM_TURN_RIGHT_GPIO_Port, BCM_TURN_RIGHT_Pin))) {
        state.turn_right_enabled = state.turn_right_enabled ? 0U : 1U;
    }

    if (debounce_pressed(&s_hazard_button,
                         read_input(BCM_HAZARD_GPIO_Port, BCM_HAZARD_Pin))) {
        state.hazard_enabled = state.hazard_enabled ? 0U : 1U;
    }

    s_state = state;
}

void BCM_Input_GetState(BcmInput_State_t *out_state)
{
    if (out_state != NULL) {
        *out_state = s_state;
    }
}

void BCM_Input_SetState(const BcmInput_State_t *state)
{
    if (state != NULL) {
        s_state = *state;
    }
}

void BCM_Input_SetField(BcmInput_Field_t field, uint8_t active)
{
    BcmInput_State_t state = s_state;
    uint8_t value = active ? 1U : 0U;

    switch (field) {
    case BCM_INPUT_FIELD_TURN_LEFT:
        state.turn_left_enabled = value;
        break;
    case BCM_INPUT_FIELD_TURN_RIGHT:
        state.turn_right_enabled = value;
        break;
    case BCM_INPUT_FIELD_HAZARD:
        state.hazard_enabled = value;
        break;
    default:
        return;
    }

    s_state = state;
}

void BCM_Input_ClearAll(void)
{
    BcmInput_State_t state;

    memset(&state, 0, sizeof(state));
    s_state = state;
}

void BCM_Input_SetMode(BcmInput_Mode_t mode)
{
    if (mode == BCM_INPUT_MODE_GPIO || mode == BCM_INPUT_MODE_UART) {
        s_mode = mode;
    }
}

BcmInput_Mode_t BCM_Input_GetMode(void)
{
    return s_mode;
}

const char *BCM_Input_GetModeString(void)
{
    return s_mode == BCM_INPUT_MODE_UART ? "uart" : "gpio";
}
