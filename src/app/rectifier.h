#pragma once
/*
 * Project: ATSAMC21J15 – Rectifier CAN Interface (Classic CAN, 29-bit EID)
 * Author:  Sripad Madhusudan Upadhyaya
 * License: MIT
 * Year:    2025
 */
#include <stdint.h>
#include <stdbool.h>

#define RECT_DATA1_POS  0
#define RECT_TOFROM_POS 8
#define RECT_PRIO_POS   9
#define RECT_DATA0_POS  12
#define RECT_MSG_POS    20
#define RECT_ID_POS     24

static inline uint32_t rect_pack_eid(uint8_t id, uint8_t msg,
                                     uint8_t data0, uint8_t prio,
                                     uint8_t tofrom, uint8_t data1)
{
    return ((uint32_t)(id & 0x1F) << RECT_ID_POS)    |
           ((uint32_t)(msg & 0x0F) << RECT_MSG_POS)  |
           ((uint32_t)(data0)      << RECT_DATA0_POS)|
           ((uint32_t)(prio & 0x07)<< RECT_PRIO_POS) |
           ((uint32_t)(tofrom&0x01)<< RECT_TOFROM_POS)|
           ((uint32_t)(data1)      << RECT_DATA1_POS);
}

static inline void rect_unpack_eid(uint32_t eid,
                                   uint8_t *id, uint8_t *msg,
                                   uint8_t *data0, uint8_t *prio,
                                   uint8_t *tofrom, uint8_t *data1)
{
    *id     = (eid >> RECT_ID_POS)     & 0x1F;
    *msg    = (eid >> RECT_MSG_POS)    & 0x0F;
    *data0  = (eid >> RECT_DATA0_POS)  & 0xFF;
    *prio   = (eid >> RECT_PRIO_POS)   & 0x07;
    *tofrom = (eid >> RECT_TOFROM_POS) & 0x01;
    *data1  = (uint8_t)(eid >> RECT_DATA1_POS);
}

static inline uint16_t fx10_from_float(float f) { int v=(int)(f*10.0f+(f>=0?0.5f:-0.5f)); if(v<0)v=0; if(v>0xFFFF)v=0xFFFF; return (uint16_t)v; }
static inline float    fx10_to_float(uint16_t v) { return v/10.0f; }

void  rect_init(float v_set, float i_set);
bool  rect_send_setpoints_and_wait_ack(uint32_t timeout_ms);
float rect_last_v(void);
float rect_last_i(void);
