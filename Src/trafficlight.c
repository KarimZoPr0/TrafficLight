//
// Created by karim on 2024-12-07.

#include "trafficlight.h"

void TrafficLight(void)
{
    // Directions: 0 = vertical, 1 = horizontal
    uint8_t allowed_axis = 0;
    uint32_t last_direction_switch = HAL_GetTick();

    // Color Tables
    const uint32_t axis_group_table[2] = {TL_Vertical_Group, TL_Horizontal_Group};

    const uint32_t allowed_green_table[2] = {TL_Vertical_Green, TL_Horizontal_Green};
    const uint32_t allowed_orange_table[2] = {TL_Vertical_Orange, TL_Horizontal_Orange};
    const uint32_t allowed_red_table[2] = {TL_Vertical_Red, TL_Horizontal_Red};

    const uint32_t pl_red_table[2] = {PL1_Red, PL1_Red};
    const uint32_t pl_green_table[2] = {PL1_Green, PL1_Green};
    const uint32_t pl_blue_table[2] = {PL1_Blue, PL2_Blue};

    // Car activity
    uint8_t active_vertical_cars = 0;
    uint8_t active_horizontal_cars = 0;

    // State machine
    traffic_state_t state = STATE_IDLE;
    uint32_t button_press_time = 0;
    uint8_t pl_side = 0;
    uint32_t state_start_time = 0;

    traffic_light_context_t ctx = {0};
    transmit_traffic_light_flags(&ctx, TL_Vertical_Group | PL_Red_Group);
    while (1)
    {
        uint32_t now = HAL_GetTick();

        // Read car inputs (active-low)
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
                // Switch direction if_needed
                if (!active_vertical_cars && !active_horizontal_cars && now - last_direction_switch >= greenDelay)
                {
                    allowed_axis ^= 1;
                    last_direction_switch = now;
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
                            last_direction_switch = now;
                            transmit_traffic_light_flags(&ctx, ctx.flags & ~TL_Group | axis_group_table[allowed_axis] | PL_Red_Group);
                            red_wait_start = 0;
                        }
                        else
                        {
                            // Both allowed and disallowed have cars
                            if (red_wait_start == 0)
                            {
                                red_wait_start = now;
                            }
                            else
                            {
                                if (now - red_wait_start >= redDelayMax)
                                {
                                    allowed_axis ^= 1;
                                    last_direction_switch = now;
                                    transmit_traffic_light_flags(&ctx, (ctx.flags & ~TL_Group) | axis_group_table[allowed_axis] | PL_Red_Group);
                                    red_wait_start = 0;
                                }
                            }
                        }
                    }
                }

                static uint32_t last_press_check = 0;
                if (now - last_press_check > DEBOUNCE_TIME)
                {
                    last_press_check = now;
                    if (PL1_Hit() || PL2_Hit())
                    {
                        ctx.toggling = 1;
                        ctx.last_toggle_time = button_press_time = now;
                        state = STATE_WAITING;
                        pl_side = PL2_Hit(); // 1 if true, else 0
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

            toggle_ped_indicator(&ctx, pl_blue_table[pl_side], now);
            break;

        case STATE_CARS_TO_RED:
            {
                // Cars green->orange->red (R1.3 & R1.6), toggle indicator
                toggle_ped_indicator(&ctx, pl_blue_table[pl_side], now);
                uint32_t elapsed = now - state_start_time;
                if (elapsed < orangeDelay)
                {
                    // Only allowed direction should show orange
                    transmit_traffic_light_flags(&ctx, ctx.flags & ~allowed_green_table[allowed_axis] | allowed_orange_table[allowed_axis] | PL_Red_Group);
                }
                else
                {
                    transmit_traffic_light_flags(&ctx, ctx.flags & ~(TL_Green_Group | TL_Orange_Group) | TL_Red_Group);

                    // Cars now red, pedestrian green (R1.4)
                    transmit_traffic_light_flags(&ctx, ctx.flags & ~pl_red_table[pl_side] | pl_green_table[pl_side]);
                    ctx.toggling = 0;
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
                transmit_traffic_light_flags(&ctx, ctx.flags & ~PL_Green_Group | PL_Red_Group);
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
                    transmit_traffic_light_flags(&ctx, ctx.flags & ~allowed_red_table[allowed_axis] | allowed_orange_table[allowed_axis] | PL_Red_Group);
                }
                else
                {

                    transmit_traffic_light_flags(&ctx, ctx.flags & ~TL_Group | axis_group_table[allowed_axis] | PL_Red_Group);
                    last_direction_switch = now;
                    state = STATE_IDLE;
                }
            }
            break;
        }

    }
}