//
// Created by Abdik on 2024-12-07.
//

#ifndef TRAFFICLIGHT_H
#define TRAFFICLIGHT_H

#include <stdint.h>

typedef uint32_t street_direction_flags_t;
enum street_direction_flags_t
{
    // Street direction 1
    TL1_Red     =   1 << 0,     // U1 Q0
    TL1_Yellow  =   1 << 1,     // U1 Q1
    TL1_Green   =   1 << 2,     // U1 Q2
    PL1_Red     =   1 << 3,     // U1 Q3
    PL1_Green   =   1 << 4,     // U1 Q4
    PL1_Blue    =   1 << 5,     // U1 Q5

    // Street direction 2
    TL2_Red     =   1 << 8,     // U2 Q0
    TL2_Yellow  =   1 << 9,     // U2 Q1
    TL2_Green   =   1 << 10,    // U2 Q2
    PL2_Red     =   1 << 11,    // U2 Q3
    PL2_Green   =   1 << 12,    // U2 Q4
    PL2_Blue    =   1 << 13,    // U2 Q5

    // Street direction 3
    TL3_Red     =   1 << 16,    // U3 Q0
    TL3_Yellow  =   1 << 17,    // U3 Q1
    TL3_Green   =   1 << 18,    // U3 Q2

    // Street direction 4
    TL4_Red     =   1 << 19,    // U3 Q3
    TL4_Yellow  =   1 << 20,    // U3 Q4
    TL4_Green   =   1 << 21,    // U3 Q5
};

enum street_direction_groups_t
{
    // Individual direction groups
    TL1_Group = TL1_Red | TL1_Yellow | TL1_Green,
    TL2_Group = TL2_Red | TL2_Yellow | TL2_Green,
    TL3_Group = TL3_Red | TL3_Yellow | TL3_Green,
    TL4_Group = TL4_Red | TL4_Yellow | TL4_Green,

    PL1_Group = PL1_Red | PL1_Blue | PL1_Green,
    PL2_Group = PL2_Red | PL2_Blue | PL2_Green,

    // Pedestrian color groups
    PL_Red_Group   = PL1_Red | PL2_Red,
    PL_Green_Group = PL1_Green | PL2_Green,
    PL_Blue_Group  = PL1_Blue | PL2_Blue,

    // Traffic light color groups
    TL_Red_Group    = TL1_Red | TL2_Red | TL3_Red | TL4_Red,
    TL_Orange_Group = TL1_Yellow | TL2_Yellow | TL3_Yellow | TL4_Yellow,
    TL_Green_Group  = TL1_Green | TL2_Green | TL3_Green | TL4_Green,

    // Vertical and horizontal groups by color
    TL_Vertical_Green  = TL1_Green | TL3_Green,
    TL_Vertical_Orange = TL1_Yellow | TL3_Yellow,
    TL_Vertical_Red    = TL1_Red | TL3_Red,

    // Horizontal direction uses TL2 and TL4 as green directions, TL1 and TL3 stay red when horizontal is allowed
    TL_Horizontal_Green  = TL2_Green | TL4_Green,
    TL_Horizontal_Orange = TL2_Yellow | TL4_Yellow,
    TL_Horizontal_Red    = TL2_Red | TL4_Red,

    // When vertical direction is allowed: vertical green + horizontal red
    TL_Vertical_Group   = TL_Vertical_Green | TL_Horizontal_Red,
    // When horizontal direction is allowed: horizontal green + vertical red
    TL_Horizontal_Group = TL_Horizontal_Green | TL_Vertical_Red,

    // All traffic lights and pedestrian lights combined
    TL_Group = TL1_Group | TL2_Group | TL3_Group | TL4_Group,
    PL_Group = PL1_Group | PL2_Group
};

typedef enum traffic_state_t traffic_state_t;
enum traffic_state_t {
    STATE_IDLE,           // Cars green, Ped red (initial state)
    STATE_WAITING,        // Button pressed, toggling indicator until pedestrian green
    STATE_CARS_TO_RED,    // Transition cars from green to red (through orange)
    STATE_PEDESTRIAN_GREEN,// Pedestrian green for walkingDelay
    STATE_CARS_TO_GREEN   // Transition cars from red to green (through orange)
};


typedef struct traffic_light_context_t traffic_light_context_t;
struct traffic_light_context_t
{
    street_direction_flags_t flags;
    uint8_t data[3];
};

void TrafficLight(void);


void set_traffic_light(traffic_light_context_t *ctx, uint32_t flags);

#endif //TRAFFICLIGHT_H
