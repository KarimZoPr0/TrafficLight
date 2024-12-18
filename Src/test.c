//
// Created by Abdik on 2024-12-16.
//

#include "test.h"

static traffic_light_context_t ctx = {0};

void test_program(void)
{
    transmit_traffic_light_flags(&ctx, ~ALL_Group);
    // test_transmit_traffic_light_flags();
    test_buttons();
    // test_transition_cars();
}


void test_buttons(void)
{
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


void test_transition_cars(void)
{
    transmit_traffic_light_flags(&ctx, TL_Red_Group | PL_Red_Group);
    HAL_Delay(1000);

    for (uint32_t i = 0; i < 4; ++i)
    {
        // Test transition from red to green
        transition_cars_to_green(&ctx, i);

        // Test transition from green to red
        transition_cars_to_red(&ctx, i);
    }
}


void test_transmit_traffic_light_flags(void)
{
    const uint32_t flags_table[] =
    {
        TL1_Red, TL1_Yellow, TL1_Green, PL1_Red, PL1_Green, PL1_Blue,
        TL2_Red, TL2_Yellow, TL2_Green, PL2_Red, PL2_Green, PL2_Blue,
        TL3_Red, TL3_Yellow, TL3_Green,
        TL4_Red, TL4_Yellow, TL4_Green,
    };

    const uint32_t count = sizeof(flags_table) / sizeof(flags_table[0]);
    const uint32_t total_permutations = 1 << count;
    for (uint32_t mask = 0; mask < total_permutations; ++mask)
    {
        uint32_t flags = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            if (mask & 1 << i)
            {
                flags |= flags_table[i];
            }
        }
        transmit_traffic_light_flags(&ctx, flags);
        HAL_Delay(50);
    }
}
