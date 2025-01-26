//
// Created by Karim on 2024-12-16.
//

#ifndef TRAFFICLIGHT_FUNCTIONS_H
#define TRAFFICLIGHT_FUNCTIONS_H

#include "spi.h"


typedef uint32_t street_direction_flags_t;

/**
 * @brief Enumeration for street direction flags.
 */
enum street_direction_flags_t
{
    // Street direction 1
    TL1_Red = 1 << 0, // U1 Q0
    TL1_Yellow = 1 << 1, // U1 Q1
    TL1_Green = 1 << 2, // U1 Q2
    PL1_Red = 1 << 3, // U1 Q3
    PL1_Green = 1 << 4, // U1 Q4
    PL1_Blue = 1 << 5, // U1 Q5

    // Street direction 2
    TL2_Red = 1 << 8, // U2 Q0
    TL2_Yellow = 1 << 9, // U2 Q1
    TL2_Green = 1 << 10, // U2 Q2
    PL2_Red = 1 << 11, // U2 Q3
    PL2_Green = 1 << 12, // U2 Q4
    PL2_Blue = 1 << 13, // U2 Q5

    // Street direction 3
    TL3_Red = 1 << 16, // U3 Q0
    TL3_Yellow = 1 << 17, // U3 Q1
    TL3_Green = 1 << 18, // U3 Q2

    // Street direction 4
    TL4_Red = 1 << 19, // U3 Q3
    TL4_Yellow = 1 << 20, // U3 Q4
    TL4_Green = 1 << 21, // U3 Q5
};

/**
 * @brief Enumeration for grouping street directions and their respective lights.
 */
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
    PL_Red_Group = PL1_Red | PL2_Red,
    PL_Green_Group = PL1_Green | PL2_Green,
    PL_Blue_Group = PL1_Blue | PL2_Blue,

    // Traffic light color groups
    TL_Red_Group = TL1_Red | TL2_Red | TL3_Red | TL4_Red,
    TL_Orange_Group = TL1_Yellow | TL2_Yellow | TL3_Yellow | TL4_Yellow,
    TL_Green_Group = TL1_Green | TL2_Green | TL3_Green | TL4_Green,

    // Vertical and horizontal groups by color
    TL_Vertical_Green = TL1_Green | TL3_Green,
    TL_Vertical_Orange = TL1_Yellow | TL3_Yellow,
    TL_Vertical_Red = TL1_Red | TL3_Red,

    TL_Horizontal_Green = TL2_Green | TL4_Green,
    TL_Horizontal_Orange = TL2_Yellow | TL4_Yellow,
    TL_Horizontal_Red = TL2_Red | TL4_Red,

    // All traffic lights and pedestrian lights combined
    TL_Group = TL1_Group | TL2_Group | TL3_Group | TL4_Group,
    PL_Group = PL1_Group | PL2_Group,

    ALL_Group = TL_Group | PL_Group
};

typedef enum traffic_state_t traffic_state_t;

/**
 * @brief Traffic light states enumeration.
 */
enum traffic_state_t
{
    STATE_IDLE,
    STATE_WAITING,
    STATE_CARS_TO_RED,
    STATE_PEDESTRIAN_GREEN,
    STATE_CARS_TO_GREEN
};

typedef struct traffic_light_context_t traffic_light_context_t;

/**
 * @brief Context structure for managing traffic light states and timing.
 */
struct traffic_light_context_t
{
    /**
     * Bitmask representing street direction flags.
     */
    street_direction_flags_t flags;
    /**
     * Array to store traffic light state data.
     */
    uint8_t data[3];

    /**
     * Indicates if toggling functionality is active.
     */
    uint8_t toggling;
    /**
     * Stores the last timestamp when the toggle action occurred.
     */
    uint32_t last_toggle_time;
    /**
     * Current timestamp in milliseconds.
     */
    uint32_t now;
};

/**
 * @brief Reads the state of the TL1 car switch hit GPIO pin.
 * @return GPIO_PIN_RESET if the switch is hit, otherwise GPIO_PIN_SET.
 */
GPIO_PinState TL1_Car_Hit();
/**
 * @brief Checks if the TL3 car switch has been triggered.
 * @return GPIO_PIN_RESET if the switch is hit, otherwise GPIO_PIN_SET.
 */
GPIO_PinState TL3_Car_Hit();
/**
 * @brief Checks if the TL2 car switch is hit.
 * @return GPIO_PIN_RESET if the switch is hit, otherwise GPIO_PIN_SET.
 */
GPIO_PinState TL2_Car_Hit();
/**
 * @brief Checks if TL4 car switch is hit.
 * @return GPIO_PIN_RESET if the switch is hit, otherwise GPIO_PIN_SET.
 */
GPIO_PinState TL4_Car_Hit();
/**
 * @brief Reads the state of the PL1 button.
 * @return GPIO_PIN_RESET if the button is hit, GPIO_PIN_SET otherwise.
 */
GPIO_PinState PL1_Hit();
/**
 * @brief Checks if the pedestrian button PL2 is hit.
 * @return GPIO_PIN_RESET if the button is hit, GPIO_PIN_SET otherwise.
 */
GPIO_PinState PL2_Hit();
/**
 * @brief Transmits traffic light control flags via SPI.
 * @param ctx Pointer to the traffic light context structure.
 * @param flags 32-bit value representing the traffic light control flags.
 */
void transmit_traffic_light_flags(traffic_light_context_t* ctx, uint32_t flags);
/**
 * @brief Toggles specified lights based on timing and state.
 * @param ctx Pointer to the traffic light context.
 * @param flags Street direction flags
 */
void toggle_indicator_light(traffic_light_context_t* ctx, uint32_t flags);
/**
 * @brief Handles the transition of traffic lights from red to green state.
 * @param ctx Pointer to the traffic light context structure.
 * @param allowed_axis Axis identifier representing the direction allowed to proceed.
 * @param state Pointer to the current traffic light state variable.
 * @param last_direction_switch Pointer to the timestamp of the last direction switch.
 * @param state_start_time Start time of the current state.
 */
void handle_cars_to_green(
    traffic_light_context_t* ctx,
    uint32_t allowed_axis,
    traffic_state_t* state,
    uint32_t* last_direction_switch,
    uint32_t state_start_time);
/**
 * @brief Handles traffic light transition for cars to red and pedestrians to green.
 * @param ctx Pointer to the traffic light context structure.
 * @param allowed_axis Indicates the axis currently allowed for vehicle movement.
 * @param pl_side Specifies the pedestrian light side being handled.
 * @param state Pointer to the traffic state variable to be updated.
 * @param state_start_time Pointer to the state start time variable to be updated.
 */
void handle_cars_to_red(
    traffic_light_context_t* ctx,
    uint32_t allowed_axis,
    uint8_t pl_side,
    traffic_state_t* state,
    uint32_t* state_start_time);

#define DEBOUNCE_TIME 50

// Timings
#define TOGGLE_FREQ      250
#define PEDESTRIAN_DELAY 3000
#define WALKING_DELAY    5000
#define ORANGE_DELAY     3000
#define GREEN_DELAY      4000
#define RED_DELAY_MAX    5000

#endif //TRAFFICLIGHT_FUNCTIONS_H
