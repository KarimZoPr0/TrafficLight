//
// Created by Abdikarim on 2024-12-16.
//

#include "trafficlight_functions.h"

// Flags table
extern uint32_t axis_group_table[];
extern uint32_t allowed_green_table[];
extern uint32_t allowed_orange_table[];
extern uint32_t allowed_red_table[];
extern uint32_t pl_red_table[];
extern uint32_t pl_green_table[];
extern uint32_t pl_blue_table[];

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


void toggle_indicator_light(traffic_light_context_t* ctx, uint32_t flags)
{
    const uint32_t valid_flags = PL1_Blue | PL2_Blue;
    if (flags & ~valid_flags) return;

    if (ctx->toggling && ctx->now - ctx->last_toggle_time >= TOGGLE_FREQ)
    {
        ctx->last_toggle_time = ctx->now;
        transmit_traffic_light_flags(ctx, ctx->flags ^ flags);
    }
}

void handle_cars_to_red(
    traffic_light_context_t* ctx,
    uint32_t allowed_axis,
    uint8_t  pl_side,
    traffic_state_t* state,
    uint32_t* state_start_time)
{
    toggle_indicator_light(ctx, pl_blue_table[pl_side]);

    uint32_t elapsed = ctx->now - (*state_start_time);
    if (elapsed < ORANGE_DELAY)
    {
        // Only allowed direction should show orange
        transmit_traffic_light_flags(
            ctx,
            ctx->flags
            & ~allowed_green_table[allowed_axis]
            |  allowed_orange_table[allowed_axis]
            |  PL_Red_Group
        );
    }
    else
    {
        // Remove all greens/oranges => set cars fully red
        transmit_traffic_light_flags(
            ctx,
            ctx->flags
            & ~(TL_Green_Group | TL_Orange_Group)
            |  TL_Red_Group
        );

        // Then pedestrians go green for this side
        transmit_traffic_light_flags(
            ctx,
            ctx->flags
            & ~pl_red_table[pl_side]
            |  pl_green_table[pl_side]
        );

        // Disable toggling
        ctx->toggling = 0;

        // Update state/time exactly as original
        *state_start_time = ctx->now;
        *state = STATE_PEDESTRIAN_GREEN;
    }
}

void handle_cars_to_green(
    traffic_light_context_t* ctx,
    uint32_t allowed_axis,
    traffic_state_t* state,
    uint32_t* last_direction_switch,
    uint32_t state_start_time)
{
    // -----------------------------------------------
    // EXACT code from your original STATE_CARS_TO_GREEN
    // -----------------------------------------------
    uint32_t elapsed = ctx->now - state_start_time;
    if (elapsed < ORANGE_DELAY)
    {
        transmit_traffic_light_flags(
            ctx,
            ctx->flags
            & ~allowed_red_table[allowed_axis]
            |  allowed_orange_table[allowed_axis]
            |  PL_Red_Group
        );
    }
    else
    {
        transmit_traffic_light_flags(
            ctx,
            ctx->flags
            & ~TL_Group
            |  axis_group_table[allowed_axis]
            |  PL_Red_Group
        );
        *last_direction_switch = ctx->now;
        *state = STATE_IDLE;
    }
}

