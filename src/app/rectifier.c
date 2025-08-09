/*
 * Project: ATSAMC21J15 – Rectifier CAN Interface (Classic CAN, 29-bit EID)
 * Author:  Sripad Madhusudan Upadhyaya
 * License: MIT
 * Year:    2025
 */
#include "app/rectifier.h"
#include "drivers/can_if.h"
#include "sys/log.h"

#define RECT_CMD_ID        1u
#define RECT_ACK_ID        2u
#define RECT_MSG_TYPE      1u
#define RECT_PRIO          1u
#define RECT_TOFROM_TX     1u
#define RECT_TOFROM_RX     0u

typedef struct {
    uint16_t v_set_x10;
    uint16_t i_set_x10;
    uint16_t v_ack_x10;
    uint16_t i_ack_x10;
} rect_ctx_t;

static rect_ctx_t g;

static uint32_t make_cmd_eid(uint16_t v_x10)
{
    uint8_t d0 = (uint8_t)(v_x10 & 0xFF);
    uint8_t d1 = (uint8_t)(v_x10 >> 8);
    return rect_pack_eid(RECT_CMD_ID, RECT_MSG_TYPE, d0, RECT_PRIO, RECT_TOFROM_TX, d1);
}

void rect_init(float v_set, float i_set)
{
    g.v_set_x10 = fx10_from_float(v_set);
    g.i_set_x10 = fx10_from_float(i_set);
}

bool rect_send_setpoints_and_wait_ack(uint32_t timeout_ms)
{
    uint32_t eid = make_cmd_eid(g.v_set_x10);
    uint8_t  payload[2] = { (uint8_t)(g.i_set_x10 & 0xFF), (uint8_t)(g.i_set_x10 >> 8) };

    if (!can_tx_ext(eid, payload, 2)) { LOGE("CAN TX failed"); return false; }

    uint32_t rx_eid; uint8_t rx_data[8]; uint8_t rx_len;
    if (!can_rx_wait(&rx_eid, rx_data, &rx_len, timeout_ms)) { LOGE("ACK timeout"); return false; }

    uint8_t id,msg,d0,prio,tofrom,d1;
    rect_unpack_eid(rx_eid, &id,&msg,&d0,&prio,&tofrom,&d1);
    if (id != RECT_ACK_ID || tofrom != RECT_TOFROM_RX) return false;

    g.v_ack_x10 = (uint16_t)((d1 << 8) | d0);
    if (rx_len < 2) return false;
    g.i_ack_x10 = (uint16_t)((rx_data[1] << 8) | rx_data[0]);

    LOGI("ACK V=%.1fV I=%.1fA", fx10_to_float(g.v_ack_x10), fx10_to_float(g.i_ack_x10));
    return true;
}

float rect_last_v(void){ return fx10_to_float(g.v_ack_x10); }
float rect_last_i(void){ return fx10_to_float(g.i_ack_x10); }
