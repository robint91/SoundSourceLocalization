#ifndef __ADCDMA_H
#define __ADCDMA_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
void adcdma_Init(int16_t* bufferR, int16_t* bufferL, uint16_t size);


#endif 