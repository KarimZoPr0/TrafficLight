/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program integrating Task 1 and Task 2 requirements
  ******************************************************************************
  * @attention
  *
  * This code implements:
  * - Task 1 (R1.1–R1.6): Pedestrian crossing with toggle indicator.
  * - Task 2 (R2.1–R2.8): Road crossing logic (vertical/horizontal lanes), timing.
  *
  * Initialization (R1.1 + R2.8):
  * - Ped red, vertical green, horizontal red.
  *
  * Pedestrian request:
  * - Indicator toggle (R1.2), after pedestrianDelay cars red (R1.3), ped green (R1.4).
  * - After walkingDelay, cars green again (R1.5, R1.6).
  *
  * Idle (no ped):
  * - If no cars, switch allowed direction every greenDelay (R2.4).
  * - If car waits at red, either wait redDelayMax or switch immediately if no cars on allowed side (R2.6,R2.7).
  * - Only one direction green (R2.2), no left turns scenario (R2.1).
  * - Remain green if cars active (R2.5).
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "trafficlight.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
#define DEBOUNCE_TIME 50
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static traffic_light_context_t ctx = {0};

// Timings
static uint32_t toggleFreq = 250; // R1.2
static uint32_t pedestrianDelay = 3000; // R1.3
static uint32_t walkingDelay = 5000; // R1.4
static uint32_t orangeDelay = 3000; // R1.6
static uint32_t greenDelay = 4000; // R2.4
static uint32_t redDelayMax = 5000; // R2.6

// Directions: 0 = vertical, 1 = horizontal
static int allowed_axis = 0;
static uint32_t last_direction_switch = 0;
static uint32_t axis_table[2] = {TL_Vertical_Group, TL_Horizontal_Group};

// Car activity
static int active_vertical_cars = 0;
static int active_horizontal_cars = 0;

// State machine
static traffic_state_t state = STATE_IDLE;
static uint32_t button_press_time = 0;
static uint32_t last_toggle_time = 0;
static uint32_t toggle_state = 0;
static uint32_t toggling = 0;
static uint32_t state_start_time = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

// Toggle pedestrian indicator
static void toggle_ped_indicator(uint32_t now)
{
    if (toggling && now - last_toggle_time >= toggleFreq)
    {
        last_toggle_time = now;
        set_traffic_light(&ctx, ctx.flags ^ PL_Blue_Group);
        toggle_state ^= 1;
    }
}

// Handle redDelayMax (R2.6,R2.7)
static void handle_red_delay_max(uint32_t now)
{
    static uint32_t red_wait_start = 0;
    int allowed_cars = (allowed_axis == 0) ? active_vertical_cars : active_horizontal_cars;
    int disallowed_cars = (allowed_axis == 0) ? active_horizontal_cars : active_vertical_cars;

    // If no cars in the disallowed direction, no waiting needed
    if (!disallowed_cars)
    {
        red_wait_start = 0;
        return;
    }

    // If disallowed direction has cars, but allowed direction doesn't,
    // switch immediately without waiting.
    if (!allowed_cars)
    {
        allowed_axis ^= 1;
        last_direction_switch = now;
        set_traffic_light(&ctx, ctx.flags & ~TL_Group | axis_table[allowed_axis] | PL_Red_Group);
        red_wait_start = 0;
        return;
    }

    // Both allowed and disallowed directions have cars.
    // Start or continue waiting until redDelayMax is reached.
    if (red_wait_start == 0)
    {
        red_wait_start = now;
    }
    else if (now - red_wait_start >= redDelayMax)
    {
        allowed_axis ^= 1;
        last_direction_switch = now;
        set_traffic_light(&ctx, ctx.flags & ~TL_Group | axis_table[allowed_axis] | PL_Red_Group);
        red_wait_start = 0;
    }
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_SPI3_Init();

    /* USER CODE BEGIN 2 */
    set_traffic_light(&ctx, TL_Vertical_Group | PL_Red_Group); // vertical green, ped red
    allowed_axis = 0;
    last_direction_switch = HAL_GetTick();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        uint32_t now = HAL_GetTick();

        // Read car inputs (active-low)
        uint8_t tl1_active = HAL_GPIO_ReadPin(TL1_Car_GPIO_Port, TL1_Car_Pin) == GPIO_PIN_RESET;
        uint8_t tl3_active = HAL_GPIO_ReadPin(TL3_Car_GPIO_Port, TL3_Car_Pin) == GPIO_PIN_RESET;
        uint8_t tl2_active = HAL_GPIO_ReadPin(TL2_Car_GPIO_Port, TL2_Car_Pin) == GPIO_PIN_RESET;
        uint8_t tl4_active = HAL_GPIO_ReadPin(TL4_Car_GPIO_Port, TL4_Car_Pin) == GPIO_PIN_RESET;

        active_vertical_cars = tl1_active || tl3_active;
        active_horizontal_cars = tl2_active || tl4_active;

        // State machine for Task 1
        switch (state)
        {
        case STATE_IDLE:
            {
                // Switch direction if_needed
                if (!active_vertical_cars && !active_horizontal_cars && now - last_direction_switch >= greenDelay)
                {
                    allowed_axis ^= 1;
                    last_direction_switch = now;
                    set_traffic_light(&ctx, ctx.flags & ~TL_Group | axis_table[allowed_axis] | PL_Red_Group);
                }

                handle_red_delay_max(now);

                static uint32_t last_press_check = 0;
                if (now - last_press_check > DEBOUNCE_TIME)
                {
                    last_press_check = now;
                    // Check pedestrian button (R1.2)
                    if (HAL_GPIO_ReadPin(PL1_Switch_GPIO_Port, PL1_Switch_Pin) == GPIO_PIN_RESET ||
                        HAL_GPIO_ReadPin(PL2_Switch_GPIO_Port, PL2_Switch_Pin) == GPIO_PIN_RESET)
                    {
                        toggling = 1;
                        toggle_state = 0;
                        last_toggle_time = button_press_time = now;
                        state = STATE_WAITING;
                    }
                }
            }
            break;

        case STATE_WAITING:
            // Wait pedestrianDelay, toggle indicator until pedestrian green (R1.2)
            if (now - button_press_time >= pedestrianDelay)
            {
                state_start_time = now;
                state = STATE_CARS_TO_RED;
            }
            toggle_ped_indicator(now);
            break;

        case STATE_CARS_TO_RED:
            {
                // Cars green->orange->red (R1.3 & R1.6), toggle indicator
                toggle_ped_indicator(now);
                uint32_t elapsed = now - state_start_time;
                if (elapsed < orangeDelay)
                {
                    set_traffic_light(&ctx, ctx.flags & ~TL_Green_Group | TL_Orange_Group | PL_Red_Group);
                }
                else
                {
                    set_traffic_light(&ctx, ctx.flags & ~(TL_Green_Group | TL_Orange_Group) | TL_Red_Group);

                    // Cars now red, pedestrian green (R1.4)
                    set_traffic_light(&ctx, (ctx.flags & ~PL_Red_Group) | PL_Green_Group);
                    toggling = 0;
                    state_start_time = now;
                    state = STATE_PEDESTRIAN_GREEN;
                }
            }
            break;

        case STATE_PEDESTRIAN_GREEN:
            // Ped green for walkingDelay (R1.4)
            if (now - state_start_time >= walkingDelay)
            {
                // Return ped red, cars red->orange->green (R1.5 & R1.6)
                set_traffic_light(&ctx, (ctx.flags & ~PL_Green_Group) | PL_Red_Group);
                state_start_time = now;
                state = STATE_CARS_TO_GREEN;
            }
            break;

        case STATE_CARS_TO_GREEN:
            // Cars red->orange->green (R1.6)
            {
                uint32_t elapsed = now - state_start_time;
                if (elapsed < orangeDelay)
                {
                    set_traffic_light(&ctx, ctx.flags & ~TL_Red_Group | TL_Orange_Group | PL_Red_Group);
                }
                else
                {
                    set_traffic_light(
                        &ctx, ctx.flags & ~(TL_Red_Group | TL_Orange_Group) | TL_Green_Group | PL_Red_Group);
                    set_traffic_light(&ctx, ctx.flags & ~TL_Group | axis_table[allowed_axis] | PL_Red_Group);
                    state = STATE_IDLE;
                }
            }
            break;
        }

        /* USER CODE END WHILE */
        /* USER CODE BEGIN 3 */
        // Additional user code if needed
        /* USER CODE END 3 */
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    /* This is auto-generated code from CubeMX and is omitted for brevity. */
    /* Ensure your clock is configured as needed. */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 10;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
