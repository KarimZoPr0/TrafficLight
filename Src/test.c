//
// Created by Karim on 2024-12-16.
//

#include "test.h"

#include <assert.h>


void test_program(void)
{
    //test_transmit_traffic_light_flags();
    //test_buttons();
    //test_toggle_indicator_light();
    test_handle_cars_to_green();
}

void test_toggle_indicator_light()
{
    traffic_light_context_t ctx = {0};

    uint32_t pl_blue_table[] = {PL1_Blue, PL2_Blue};
    uint32_t count = sizeof(pl_blue_table) / sizeof(pl_blue_table[0]);
    ctx.toggling = 1;
    for (int idx = 0; idx < count; ++idx)
    {
        for (int j = 0; j < 10; ++j) // 10 iterations -> 5 toggles
        {
            ctx.now += TOGGLE_FREQ;
            toggle_indicator_light(&ctx, pl_blue_table[idx]);
            HAL_Delay(TOGGLE_FREQ);
        }
    }
}

void test_buttons_and_switches(void)
{
    traffic_light_context_t ctx = {0};
    const uint32_t flags_table[] =
    {
        TL1_Red, TL1_Yellow, TL1_Green, PL1_Red, PL1_Green, PL1_Blue,
        TL2_Red, TL2_Yellow, TL2_Green, PL2_Red, PL2_Green, PL2_Blue,
        TL3_Red, TL3_Yellow, TL3_Green,
        TL4_Red, TL4_Yellow, TL4_Green,
    };

    int32_t j;

    /* Checking buttons */
    j = 0;
    transmit_traffic_light_flags(&ctx, flags_table[j]);
    while (j < 18 && j >= 0)
    {
        if (TL1_Car_Hit()) // Wait
        {
            j++; // next led to the right
            transmit_traffic_light_flags(&ctx, flags_table[j]); // Light on
            HAL_Delay(100); // 100 ms
            while (TL1_Car_Hit()); // Wait for button release
            HAL_Delay(100); // 100 ms
        }
        if (TL2_Car_Hit()) // Wait
        {
            j--; // next led to the left
            transmit_traffic_light_flags(&ctx, flags_table[j]); // Light on
            HAL_Delay(100); // 100 ms
            while (TL2_Car_Hit()); // Wait for button release
            HAL_Delay(100); // 100 ms
        }
        if (TL3_Car_Hit()) // Wait
        {
            j++; // next led to the left
            transmit_traffic_light_flags(&ctx, flags_table[j]); // Light on
            HAL_Delay(100); // 100 ms
            while (TL3_Car_Hit()); // Wait for button release
            HAL_Delay(100); // 100 ms
        }
        if (TL4_Car_Hit()) // Wait
        {
            j--; // next led to the left
            transmit_traffic_light_flags(&ctx, flags_table[j]); // Light on
            HAL_Delay(100); // 100 ms
            while (TL4_Car_Hit()); // Wait for button release
            HAL_Delay(100); // 100 ms
        }

        if (PL1_Hit()) // Wait
        {
            j++; // next led to the left
            transmit_traffic_light_flags(&ctx, flags_table[j]); // Light on
            HAL_Delay(100); // 100 ms
            while (PL1_Hit()); // Wait for button release
            HAL_Delay(100); // 100 ms
        }

        if (PL2_Hit()) // Wait
        {
            j--; // next led to the left
            transmit_traffic_light_flags(&ctx, flags_table[j]); // Light on
            HAL_Delay(100); // 100 ms
            while (PL2_Hit()); // Wait for button release
            HAL_Delay(100); // 100 ms
        }
    }
}
void test_transmit_traffic_light_flags(void)
{
    traffic_light_context_t ctx = {0};
    const uint32_t flags_table[] =
    {
        TL1_Red, TL1_Yellow, TL1_Green, PL1_Red, PL1_Green, PL1_Blue,
        TL2_Red, TL2_Yellow, TL2_Green, PL2_Red, PL2_Green, PL2_Blue,
        TL3_Red, TL3_Yellow, TL3_Green,
        TL4_Red, TL4_Yellow, TL4_Green,
    };

    // All lights on
    transmit_traffic_light_flags(&ctx, ALL_Group);

    HAL_Delay(1000);

    // All lights off
    transmit_traffic_light_flags(&ctx, ~ALL_Group);

    HAL_Delay(1000);

    // Individual lights
    uint32_t count = sizeof(flags_table) / sizeof(flags_table[0]);
    for (int idx = 0; idx < count; ++idx)
    {
        transmit_traffic_light_flags(&ctx, flags_table[idx]);
        HAL_Delay(200);
    }
}

void test_handle_cars_to_green(void)
{
    traffic_light_context_t ctx = {0};
    transmit_traffic_light_flags(&ctx, TL_Red_Group | PL_Red_Group);

    for (uint32_t axis = 0; axis < 2; axis++)
    {
        uint32_t state_start_time = ctx.now;
        traffic_state_t state = STATE_CARS_TO_GREEN;
        transmit_traffic_light_flags(&ctx, TL_Red_Group | PL_Red_Group);

        HAL_Delay(1000);

        for (int i = 0; i < 10; i++)
        {
            ctx.now += 500;

            handle_cars_to_green(
                &ctx,
                axis,
                &state,
                 NULL,
                state_start_time
            );

            HAL_Delay(500);
        }
    }
}

void test_handle_cars_to_red(void)
{
    // We’ll test both axes: 0 => vertical is green, 1 => horizontal is green
    for (uint8_t axis = 0; axis < 2; axis++)
    {
        traffic_light_context_t ctx = {0};

        uint32_t start_flags = 0;
        if (axis == 0)
            start_flags = (TL_Vertical_Green | TL_Horizontal_Red | PL_Red_Group);
        else
            start_flags = (TL_Horizontal_Green | TL_Vertical_Red | PL_Red_Group);

        transmit_traffic_light_flags(&ctx, start_flags);

        HAL_Delay(1000);

        ctx.toggling = 1;

        traffic_state_t state = STATE_CARS_TO_RED;
        uint32_t state_start_time = 0;

        uint8_t pl_side = axis;

        while (state == STATE_CARS_TO_RED)
        {
            ctx.now += 500;

            handle_cars_to_red(&ctx, axis, pl_side, &state, &state_start_time);

            HAL_Delay(500);
        }
    }
}



