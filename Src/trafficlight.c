//
// Created by Abdik on 2024-12-07.
//

#include "trafficlight.h"
#include <spi.h>
#include "stm32l4xx_hal_spi.h"

void set_traffic_light(traffic_light_context_t *ctx, uint32_t flags)
{
    //- karim: store flags
    ctx->flags = flags;

    //- karim: bit flag setup
    ctx->data[0] = flags >> 16 & 0xFF;
    ctx->data[1] = flags >> 8 & 0xFF;
    ctx->data[2] = flags & 0xFF;

    //- karim: send data
    HAL_SPI_Transmit(&hspi3, ctx->data, 3, 0);

    //- karim: enable & reset shift register
    HAL_GPIO_WritePin(STCP_595_GPIO_Port, STCP_595_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STCP_595_GPIO_Port, STCP_595_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LATCH_595_GPIO_Port, LATCH_595_Pin, GPIO_PIN_SET);
}
