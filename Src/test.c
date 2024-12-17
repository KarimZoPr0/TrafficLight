//
// Created by Abdik on 2024-12-16.
//

#include "test.h"

void test_program(void)
{
    test_transmit_traffic_light_flags();
}

void Test_buttons(void)
{

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