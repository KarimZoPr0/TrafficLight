//
// Created by Abdikarim on 2024-12-16.
//

#include "trafficlight_functions.h"

/**
 * axis_group_table
 * Array mapping axis indices to traffic light group states.
 */
extern uint32_t axis_group_table[];
/**
 * Allowed green traffic light configuration for specific traffic axes.
 */
extern uint32_t allowed_green_table[];
/**
 * Array representing the allowed traffic light states for the orange phase.
 */
extern uint32_t allowed_orange_table[];
/**
 * Array defining the allowed red light configurations for each axis.
 */
extern uint32_t allowed_red_table[];
/**
 * Traffic light red state lookup table.
 */
extern uint32_t pl_red_table[];
/**
 * External array representing the green traffic light table for pedestrians.
 */
extern uint32_t pl_green_table[];
/** Array representing blue pedestrian light states for different sides. */
extern uint32_t pl_blue_table[];

/**
 * @brief Checks if the TL1 car switch is activated.
 *
 * @return GPIO_PinState GPIO_PIN_SET if the switch is not activated, GPIO_PIN_RESET if activated.
 */
GPIO_PinState TL1_Car_Hit()
{
    return HAL_GPIO_ReadPin(TL1_Car_GPIO_Port, TL1_Car_Pin) == GPIO_PIN_RESET;
}

/**
 * @brief Checks if TL3 car detector is activated.
 * @return GPIO_PIN_RESET if the detector is activated, GPIO_PIN_SET otherwise.
 */
GPIO_PinState TL3_Car_Hit()
{
    return HAL_GPIO_ReadPin(TL3_Car_GPIO_Port, TL3_Car_Pin) == GPIO_PIN_RESET;
}

/**
 * @brief Checks if TL2 car switch is activated.
 * @return GPIO_PIN_SET if the switch is not activated, GPIO_PIN_RESET if activated.
 */
GPIO_PinState TL2_Car_Hit()
{
    return HAL_GPIO_ReadPin(TL2_Car_GPIO_Port, TL2_Car_Pin) == GPIO_PIN_RESET;
}

/**
 * @brief Checks if the TL4_Car GPIO input is active (hit).
 * @return GPIO_PIN_RESET if the pin is active (hit), otherwise GPIO_PIN_SET.
 */
GPIO_PinState TL4_Car_Hit()
{
    return HAL_GPIO_ReadPin(TL4_Car_GPIO_Port, TL4_Car_Pin) == GPIO_PIN_RESET;
}

/**
 * @brief Checks if the PL1 button is pressed.
 * @return GPIO_PIN_RESET if the PL1 button is pressed, otherwise GPIO_PIN_SET
 */
GPIO_PinState PL1_Hit()
{
    return HAL_GPIO_ReadPin(PL1_Switch_GPIO_Port, PL1_Switch_Pin) == GPIO_PIN_RESET;
}

/**
 * @brief Reads the state of the PL2 button.
 * @return GPIO_PIN_RESET if the PL2 button is pressed, otherwise GPIO_PIN_SET.
 */
GPIO_PinState PL2_Hit()
{
    return HAL_GPIO_ReadPin(PL2_Switch_GPIO_Port, PL2_Switch_Pin) == GPIO_PIN_RESET;
}

/**
 * @brief Transmits traffic light control flags via SPI.
 * @param ctx Pointer to the traffic light context structure.
 * @param flags 32-bit value representing the traffic light control flags.
 */
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


/**
 * @brief Toggles the indicator light in a traffic light context based on given flags.
 *
 * @param ctx Pointer to the traffic light context structure.
 * @param flags Bitmask flags indicating which indicator lights to toggle.
 */
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

/**
 * @brief Handles transition of car traffic lights to red and pedestrian lights to green.
 * @param ctx Pointer to the traffic light context.
 * @param allowed_axis Indicates which axis is currently allowed for movement.
 * @param pl_side Active pedestrian light side.
 * @param state Pointer to the current traffic state.
 * @param state_start_time Pointer to the state's start time.
 */
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

/**
 * @brief Handles the transition of cars to the green light state.
 * @param ctx Pointer to the traffic light context.
 * @param allowed_axis Axis allowed to proceed (e.g., 0 or 1).
 * @param state Pointer to the current traffic state.
 * @param last_direction_switch Pointer to the timestamp of the last direction switch.
 * @param state_start_time Start time of the current state.
 */
void handle_cars_to_green(
    traffic_light_context_t* ctx,
    uint32_t allowed_axis,
    traffic_state_t* state,
    uint32_t* last_direction_switch,
    uint32_t state_start_time)
{
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

