//
// Created by Abdikarim on 2024-12-16.
//

#include "trafficlight_functions.h"


GPIO_PinState TL1_Car_Hit()
{
    return HAL_GPIO_ReadPin(TL1_Car_GPIO_Port, TL1_Car_Pin) == GPIO_PIN_RESET;
}

GPIO_PinState TL3_Car_Hit()
{
    return HAL_GPIO_ReadPin(TL3_Car_GPIO_Port, TL3_Car_Pin) == GPIO_PIN_RESET;
}

GPIO_PinState TL2_Car_Hit()
{
    return HAL_GPIO_ReadPin(TL2_Car_GPIO_Port, TL2_Car_Pin) == GPIO_PIN_RESET;
}

GPIO_PinState TL4_Car_Hit()
{
    return HAL_GPIO_ReadPin(TL4_Car_GPIO_Port, TL4_Car_Pin) == GPIO_PIN_RESET;
}

GPIO_PinState PL1_Hit()
{
    return HAL_GPIO_ReadPin(PL1_Switch_GPIO_Port, PL1_Switch_Pin) == GPIO_PIN_RESET;
}

GPIO_PinState PL2_Hit()
{
    return HAL_GPIO_ReadPin(PL2_Switch_GPIO_Port, PL2_Switch_Pin) == GPIO_PIN_RESET;
}

// Toggle pedestrian indicator
void toggle_ped_indicator(traffic_light_context_t* ctx, uint32_t flag, uint32_t now)
{
    if (ctx->toggling && now - ctx->last_toggle_time >= TOGGLE_FREQ)
    {
        ctx->last_toggle_time = now;
        transmit_traffic_light_flags(ctx, ctx->flags ^ flag);
    }
}

void transition_cars_to_green(traffic_light_context_t* ctx, uint32_t tl_index)
{
    const uint32_t tl_red_table[] = {TL1_Red, TL2_Red, TL3_Red, TL4_Red};
    const uint32_t tl_orange_table[] = {TL1_Yellow, TL2_Yellow, TL3_Yellow, TL4_Yellow};
    const uint32_t tl_green_table[] = {TL1_Green, TL2_Green, TL3_Green, TL4_Green};

    // Transition from red to orange
    transmit_traffic_light_flags(ctx, ctx->flags & ~tl_red_table[tl_index] | tl_orange_table[tl_index]);
    HAL_Delay(ORANGE_DELAY);

    // Transition from orange to green
    transmit_traffic_light_flags(ctx, ctx->flags & ~tl_orange_table[tl_index] | tl_green_table[tl_index]);
    HAL_Delay(GREEN_DELAY);
}

void transition_cars_to_red(traffic_light_context_t* ctx, uint32_t tl_index)
{
    const uint32_t tl_red_table[] = {TL1_Red, TL2_Red, TL3_Red, TL4_Red};
    const uint32_t tl_orange_table[] = {TL1_Yellow, TL2_Yellow, TL3_Yellow, TL4_Yellow};
    const uint32_t tl_green_table[] = {TL1_Green, TL2_Green, TL3_Green, TL4_Green};

    // Transition from red to orange
    transmit_traffic_light_flags(ctx, ctx->flags & ~tl_green_table[tl_index] | tl_orange_table[tl_index]);
    HAL_Delay(ORANGE_DELAY);

    // Transition from orange to green
    transmit_traffic_light_flags(ctx, ctx->flags & ~tl_orange_table[tl_index] | tl_red_table[tl_index]);
    HAL_Delay(RED_DELAY_MAX);
}

void transmit_traffic_light_flags(traffic_light_context_t* ctx, uint32_t flags)
{
    // Store flags
    ctx->flags = flags;

    // Load flags
    ctx->data[0] = flags >> 16 & 0xFF;
    ctx->data[1] = flags >> 8 & 0xFF;
    ctx->data[2] = flags & 0xFF;

    // Send data
    HAL_SPI_Transmit(&hspi3, ctx->data, 3, 0);

    // Enable & reset shift register
    HAL_GPIO_WritePin(STCP_595_GPIO_Port, STCP_595_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STCP_595_GPIO_Port, STCP_595_Pin, GPIO_PIN_SET);
}
