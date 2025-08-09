#include "../src/drivers/can_if.h"
#include <stdio.h>
#include <string.h>

static can_filter_ext_mask_t g_f;

void can_init_100k(void) { memset(&g_f, 0, sizeof(g_f)); }
bool can_set_ext_filter_mask(uint8_t idx, const can_filter_ext_mask_t *f) { (void)idx; g_f = *f; return true; }
bool can_tx_ext(uint32_t eid, const uint8_t *data, uint8_t dlc) {
    (void)data; (void)dlc;
    fprintf(stderr, "[host-stub] TX EID=0x%08X\n", eid);
    return true;
}
bool can_rx_wait(uint32_t *eid, uint8_t *data, uint8_t *dlc, uint32_t timeout_ms) {
    (void)eid; (void)data; (void)dlc; (void)timeout_ms; return false;
}
void can_sleep_ms(uint32_t ms) { (void)ms; }
