//
// Created by karim on 2024-12-07.

#include "trafficlight.h"

// Flags table
const uint32_t axis_group_table[] = {TL_Vertical_Green | TL_Horizontal_Red, TL_Horizontal_Green | TL_Vertical_Red};
const uint32_t allowed_green_table[] = {TL_Vertical_Green, TL_Horizontal_Green};
const uint32_t allowed_orange_table[] = {TL_Vertical_Orange, TL_Horizontal_Orange};
const uint32_t allowed_red_table[] = {TL_Vertical_Red, TL_Horizontal_Red};
const uint32_t pl_red_table[] = {PL1_Red, PL2_Red};
const uint32_t pl_green_table[] = {PL1_Green, PL2_Green};
const uint32_t pl_blue_table[] = {PL1_Blue, PL2_Blue};

void traffic_light_sys(void)
{
    uint8_t allowed_axis = 0;
    uint32_t last_direction_switch = HAL_GetTick();

    // Car activity
    uint8_t active_vertical_cars = 0;
    uint8_t active_horizontal_cars = 0;

    // State machine
    traffic_state_t state = STATE_IDLE;
    uint32_t button_press_time = 0;
    uint8_t pl_side = 0;
    uint32_t state_start_time = 0;

    traffic_light_context_t ctx = {0};
    transmit_traffic_light_flags(&ctx, TL_Vertical_Green | TL_Horizontal_Red | PL_Red_Group);
    while (1)
    {
        ctx.now = HAL_GetTick();

        uint8_t tl1_active = TL1_Car_Hit();
        uint8_t tl3_active = TL3_Car_Hit();
        uint8_t tl2_active = TL2_Car_Hit();
        uint8_t tl4_active = TL4_Car_Hit();

        active_vertical_cars = tl1_active || tl3_active;
        active_horizontal_cars = tl2_active || tl4_active;

        // State machine for Task 1
        switch (state)
        {
        case STATE_IDLE:
            {
                // Switch direction if needed
                if (!active_vertical_cars && !active_horizontal_cars &&ctx.now - last_direction_switch >= GREEN_DELAY)
                {
                    allowed_axis ^= 1;
                    last_direction_switch =ctx.now;
                    transmit_traffic_light_flags(&ctx, ctx.flags & ~TL_Group | axis_group_table[allowed_axis] | PL_Red_Group);
                }

                // Handle redDelayMax (R2.6,R2.7)
                {
                    static uint32_t red_wait_start = 0;
                    uint8_t active_axis_cars_table[2] = {active_vertical_cars, active_horizontal_cars};
                    uint8_t allowed_cars = active_axis_cars_table[allowed_axis];
                    uint8_t disallowed_cars = active_axis_cars_table[allowed_axis ^ 1];

                    // If no cars in the disallowed direction, no waiting needed
                    if (!disallowed_cars)
                    {
                        red_wait_start = 0;
                    }
                    else
                    {
                        // If disallowed direction has cars but allowed direction doesn't, switch immediately
                        if (!allowed_cars)
                        {
                            allowed_axis ^= 1;
                            last_direction_switch =ctx.now;
                            transmit_traffic_light_flags(
                                &ctx, ctx.flags & ~TL_Group | axis_group_table[allowed_axis] | PL_Red_Group);
                            red_wait_start = 0;
                        }
                        else
                        {
                            // Both allowed and disallowed have cars
                            if (red_wait_start == 0)
                            {
                                red_wait_start =ctx.now;
                            }
                            else
                            {
                                if (ctx.now - red_wait_start >= RED_DELAY_MAX)
                                {
                                    allowed_axis ^= 1;
                                    last_direction_switch =ctx.now;
                                    transmit_traffic_light_flags(
                                        &ctx, (ctx.flags & ~TL_Group) | axis_group_table[allowed_axis] | PL_Red_Group);
                                    red_wait_start = 0;
                                }
                            }
                        }
                    }
                }

                static uint32_t last_press_check = 0;
                if (ctx.now - last_press_check > DEBOUNCE_TIME)
                {
                    last_press_check =ctx.now;
                    if (PL1_Hit() || PL2_Hit())
                    {
                        ctx.toggling = 1;
                        ctx.last_toggle_time = button_press_time =ctx.now;
                        state = STATE_WAITING;
                        pl_side = PL2_Hit();
                    }
                }
            }
            break;

        case STATE_WAITING:
            // Wait pedestrianDelay, toggle indicator until pedestrian green (R1.2)
            if (ctx.now - button_press_time >= PEDESTRIAN_DELAY)
            {
                state_start_time =ctx.now;
                state = STATE_CARS_TO_RED;
            }

            toggle_indicator_light(&ctx,pl_blue_table[pl_side]);
            break;

        case STATE_CARS_TO_RED:
            {
                handle_cars_to_red(&ctx, allowed_axis, pl_side, &state, &state_start_time);
            }
            break;

        case STATE_PEDESTRIAN_GREEN:
            // Ped green for walkingDelay (R1.4)
            if (ctx.now - state_start_time >= WALKING_DELAY)
            {
                // Return ped red, cars red->orange->green (R1.5 & R1.6)
                transmit_traffic_light_flags(&ctx, ctx.flags & ~PL_Green_Group | PL_Red_Group);
                state_start_time =ctx.now;
                state = STATE_CARS_TO_GREEN;
            }
            break;

        case STATE_CARS_TO_GREEN:
            // Cars red->orange->green (R1.6)
            {
                handle_cars_to_green(&ctx, allowed_axis, &state, &last_direction_switch, state_start_time);
            }
            break;
        }
    }
}
