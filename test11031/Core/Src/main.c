
#include "main.h"

/* Private variables */
TIM_HandleTypeDef htim3;

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
void Error_Handler(void);

/* Smart trash bin variables */
uint8_t trash_level = 0;

uint8_t trigger1_last = GPIO_PIN_SET;
uint8_t trigger2_last = GPIO_PIN_SET;

uint32_t trigger1_debounce = 0;
uint32_t trigger2_debounce = 0;

uint32_t lid_start_time = 0;
uint8_t lid_active = 0;

uint32_t red_blink_time = 0;
uint8_t red_state = 0;

/* Update trash level LEDs */
void Update_Trash_LED(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);

    if (trash_level <= 40)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
    }
    else if (trash_level <= 70)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    }
    else
    {
        red_state = 1;
        red_blink_time = HAL_GetTick();
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_TIM3_Init();

    /* Start TIM3 PWM */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

    /* Initialize LED states */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
    Update_Trash_LED();

    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* ---------- Trigger 1 ---------- */
        GPIO_PinState button1 =
            HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6);

        if (button1 == GPIO_PIN_RESET &&
            trigger1_last == GPIO_PIN_SET &&
            (now - trigger1_debounce >= 50))
        {
            trigger1_debounce = now;

            lid_active = 1;
            lid_start_time = now;

            HAL_GPIO_WritePin(
                GPIOA,
                GPIO_PIN_12,
                GPIO_PIN_SET
            );
        }

        trigger1_last = button1;

        /* Turn off lid LED after 5 seconds */
        if (lid_active &&
            (now - lid_start_time >= 5000))
        {
            lid_active = 0;

            HAL_GPIO_WritePin(
                GPIOA,
                GPIO_PIN_12,
                GPIO_PIN_RESET
            );
        }

        /* ---------- Trigger 2 ---------- */
        GPIO_PinState button2 =
            HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);

        if (button2 == GPIO_PIN_RESET &&
            trigger2_last == GPIO_PIN_SET &&
            (now - trigger2_debounce >= 50))
        {
            trigger2_debounce = now;

            trash_level += 10;

            if (trash_level > 100)
            {
                trash_level = 0;
            }

            Update_Trash_LED();
        }

        trigger2_last = button2;

        /* ---------- Red LED blinking ---------- */
        if (trash_level > 70)
        {
            if (now - red_blink_time >= 1000)
            {
                red_blink_time = now;
                red_state = !red_state;

                HAL_GPIO_WritePin(
                    GPIOB,
                    GPIO_PIN_11,
                    red_state ? GPIO_PIN_SET : GPIO_PIN_RESET
                );
            }
        }
    }
}

/* TIM3 initialization */
static void MX_TIM3_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 169;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 999;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(
            &htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(
            &htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }
}

/* GPIO initialization */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* LEDs off initially */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_11 | GPIO_PIN_12,
        GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_11 | GPIO_PIN_12,
        GPIO_PIN_RESET
    );

    /* Trigger 1 input */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Trigger 2 input */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PA11 and PA12 outputs */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PB11 and PB12 outputs */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(
        PWR_REGULATOR_VOLTAGE_SCALE1_BOOST
    );

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_HSI;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
