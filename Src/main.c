/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * This code implements the requirements specified:
  * R1.1 - R1.6 using a state machine and timing logic.
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
#include "trafficlight.h" // Must define set_traffic_light and light groups
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEBOUNCE_TIME    50

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
traffic_light_context_t ctx = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

// Transition cars from Green->Orange->Red or Red->Orange->Green
// Returns 1 if transition done, 0 if still waiting.
int handle_car_transition(uint32_t current_time, uint32_t start_time,uint32_t to_red, uint32_t orangeDelay)
{
    // R1.6: A car signal transitions via orange for orangeDelay ms.
    // If to_red = 1, we go Green->Orange->Red
    // If to_red = 0, we go Red->Orange->Green

    // Calculate how long we've been in this transition
    uint32_t elapsed = current_time - start_time;

    if (elapsed < orangeDelay)
    {
        // During orange phase
        if (to_red)
        {
            // from Green->Orange
            set_traffic_light(&ctx, ctx.flags & ~TL_Green_Group | TL_Orange_Group | PL_Red_Group);
        }
        else
        {
            // from Red->Orange
            set_traffic_light(&ctx, ctx.flags & ~TL_Red_Group | TL_Orange_Group | PL_Red_Group);
        }
        return 0; // not finished yet
    }

    // After orangeDelay, finalize transition
    if (to_red)
    {
        // Cars red, pedestrian could turn green after this
        set_traffic_light(&ctx, ctx.flags & ~(TL_Green_Group | TL_Orange_Group) | TL_Red_Group);
    }
    else
    {
        // Cars green, pedestrian must be red here
        set_traffic_light(&ctx, ctx.flags & ~(TL_Red_Group | TL_Orange_Group) | TL_Green_Group | PL_Red_Group);
    }
    return 1; // transition done
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int main(void)
{
    /* MCU Configuration--------------------------------------------------------*/
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_SPI3_Init();

    /* USER CODE BEGIN 2 */

    // Set lights for initial condition (R1.1)
    set_traffic_light(&ctx, TL_Green_Group | PL_Red_Group);

    // Configurable timings
    uint32_t toggleFreq = 250; // R1.2: toggling frequency of the indicator
    uint32_t pedestrianDelay = 3000; // R1.3: time after button press to have cars red
    uint32_t walkingDelay = 5000; // R1.4: how long pedestrian signal stays green
    uint32_t orangeDelay = 3000; // R1.6: how long orange lasts in transitions

    // State machine variables
    traffic_state_t state = STATE_IDLE;
    uint32_t button_press_time = 0; // When the button was pressed
    uint32_t last_toggle_time = 0; // For indicator toggling
    uint32_t toggle_state = 0; // 0: off, 1: on
    uint32_t toggling = 0; // Are we currently toggling indicator?
    uint32_t state_start_time = 0; // Generic timer to track state transitions

    /* USER CODE END 2 */
    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        uint32_t current_time = HAL_GetTick();

        // Handle state machine
        switch (state)
        {
        case STATE_IDLE:
            // Cars green, Ped red
            // Waiting for button press handled above
            // No toggling in this state
            // Check button press (debounced)
            static uint32_t last_press_check = 0;
            if (current_time - last_press_check > DEBOUNCE_TIME)
            {
                last_press_check = current_time;

                if (HAL_GPIO_ReadPin(PL1_Switch_GPIO_Port, PL1_Switch_Pin) == GPIO_PIN_RESET ||
                    HAL_GPIO_ReadPin(PL2_Switch_GPIO_Port, PL2_Switch_Pin) == GPIO_PIN_RESET)
                {
                    // Start toggling indicator until pedestrian green
                    toggling = 1;
                    toggle_state = 0;
                    last_toggle_time = button_press_time = current_time;
                    state = STATE_WAITING;
                }
            }
            break;

        case STATE_WAITING:
            // After button pressed, toggle indicator until pedestrian green (R1.2)
            // Check if we passed pedestrianDelay to start making cars red
            if (current_time - button_press_time >= pedestrianDelay)
            {
                // Move to CARS_TO_RED state to transition cars from green->orange->red
                state_start_time = current_time;
                state = STATE_CARS_TO_RED;
            }

            // Toggling indicator
            if (toggling && current_time - last_toggle_time >= toggleFreq)
            {
                last_toggle_time = current_time;
                set_traffic_light(&ctx, ctx.flags ^ PL_Blue_Group);
                toggle_state ^= 1;
            }
            break;

        case STATE_CARS_TO_RED:
            // Transition cars from green->orange->red (R1.3 & R1.6)
            // Also continue toggling indicator since pedestrian isn't green yet
            if (toggling && current_time - last_toggle_time >= toggleFreq)
            {
                last_toggle_time = current_time;
                set_traffic_light(&ctx, ctx.flags ^ PL_Blue_Group);
                toggle_state ^= 1;
            }

            if (handle_car_transition(current_time, state_start_time, 1, orangeDelay))
            {
                // Cars are now red
                // Now we can turn pedestrian green (R1.4)
                set_traffic_light(&ctx, ctx.flags & ~PL_Red_Group | PL_Green_Group);
                // Pedestrian is green, stop toggling (R1.2 fulfilled)
                toggling = 0;
                // Start timer for walkingDelay
                state_start_time = current_time;
                state = STATE_PEDESTRIAN_GREEN;
            }
            break;

        case STATE_PEDESTRIAN_GREEN:
            // Ped green for walkingDelay ms (R1.4)
            if (current_time - state_start_time >= walkingDelay)
            {
                // Time to go back to cars green, which means cars: red->orange->green
                // Ped must turn red again as cars move away from red (R1.5)
                set_traffic_light(&ctx, ctx.flags & ~PL_Green_Group | PL_Red_Group);
                state_start_time = current_time;
                state = STATE_CARS_TO_GREEN;
            }
            break;

        case STATE_CARS_TO_GREEN:
            // Transition cars from red->orange->green (R1.6)
            if (handle_car_transition(current_time, state_start_time, 0, orangeDelay))
            {
                // Cars now green, ped red as required
                // Return to idle state
                state = STATE_IDLE;
            }
            break;
        }

        /* USER CODE END WHILE */
        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
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
