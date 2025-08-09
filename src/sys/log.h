#pragma once
/*
 * Project: ATSAMC21J15 – Rectifier CAN Interface (Classic CAN, 29-bit EID)
 * Author:  Sripad Madhusudan Upadhyaya
 * License: MIT
 * Year:    2025
 */
#include <stdio.h>
#define LOGI(fmt, ...) do{ printf("[I] " fmt "\n", ##__VA_ARGS__); }while(0)
#define LOGW(fmt, ...) do{ printf("[W] " fmt "\n", ##__VA_ARGS__); }while(0)
#define LOGE(fmt, ...) do{ printf("[E] " fmt "\n", ##__VA_ARGS__); }while(0)
