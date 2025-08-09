#pragma once
/*
 * Project: ATSAMC21J15 – Rectifier CAN Interface (Classic CAN, 29-bit EID)
 * Author:  Sripad Madhusudan Upadhyaya
 * License: MIT
 * Year:    2025
 */
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t match;
    uint32_t mask;
} can_filter_ext_mask_t;

void can_init_100k(void);
bool can_set_ext_filter_mask(uint8_t idx, const can_filter_ext_mask_t *f);
bool can_tx_ext(uint32_t eid, const uint8_t *data, uint8_t dlc);
bool can_rx_wait(uint32_t *eid, uint8_t *data, uint8_t *dlc, uint32_t timeout_ms);
void can_sleep_ms(uint32_t ms);
