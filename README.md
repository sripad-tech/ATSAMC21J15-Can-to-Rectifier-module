<img width="2379" height="1184" alt="1000141322" src="https://github.com/user-attachments/assets/8f7cc276-26de-45b9-a11a-5ebca7e03733" />
# ATSAMC21J15 – Rectifier CAN Interface (Classic CAN, 29-bit EID)

**Author:** Sripad Madhusudan Upadhyaya • **License:** MIT

Firmware for **ATSAMC21J15** that sends rectifier setpoints over **Classic CAN @ 100 kbit/s** with a **29-bit EID** carrying **Voltage×10 (2 bytes)** and the **payload** carrying **Current×10 (2 bytes)**. It then waits for an **ACK** and parses it back.

This project includes **two interchangeable drivers**:
- **ASF4 HAL** (default): interrupt RX + ring buffer, TX via HAL.
- **Bare-metal (HRI)**: pure register-level; **TX+RX** implemented (no HAL).

---

## Features
- **Protocol mapping**: 2 bytes in **EID** (Voltage×10 → `Data0 LSB`, `Data1 MSB`), 2 bytes in **payload** (Current×10 → `[LSB, MSB]`).
- **Filtering**: accept only ACKs with **`ID=2` & `ToFrom=0`**.
- **Bit timing**: 100 kbit/s @ 4 MHz (**20 TQ**, ~80% SP, SJW 3).
- **ISR RX ring buffer** (8 frames) with overwrite-oldest behavior.
- **Fixed-point ×10** helpers and endian-safe pack/unpack.
- **Logging** via `LOGI/W/E` (easy to redirect to UART/RTT).
- **Host unit tests** for EID packing and scaling.
- **HIL helper** script to inject an ACK on a PC.

---

## Protocol

**29-bit EID layout**
```
bit: 28..24   23..20    19..12   11..9   8       7..0
fld:   ID     MsgType    Data0    Prio   ToFrom  Data1
```

**MCU → Rectifier (request)**
- `ID=1, MsgType=1, Prio=1, ToFrom=1`
- `Data0` = Voltage×10 **LSB**; `Data1` = Voltage×10 **MSB**
- Payload[0..1] = Current×10 `[LSB, MSB]`

**Rectifier → MCU (ACK)**
- Must match `ID=2` and `ToFrom=0` (others ignored by filter).
- Same field mapping (Voltage in EID, Current in payload).

**Golden vectors**
- TX: V=220.5 V → 2205 (`0x089D`) → Data0=0x9D, Data1=0x08; I=20.0 A → 200 (`0x00C8`) → payload `C8 00`. EID example: `0x0119D308`.
- ACK: V=221.0 V → 2210 (`0x08A2`) → Data0=0xA2, Data1=0x08; I=21.0 A → 210 (`0x00D2`) → payload `D2 00`. EID example: `0x020A2008`.
- Filter: `match=0x02000000`, `mask=0x1F000100`.

---

## Backend selection
Edit `src/config/build_config.h`:
```c
#define CAN_BACKEND_ASF4 1   // ASF4 HAL (default)
#define CAN_BACKEND_MCAN 0   // bare-metal (HRI) TX+RX
```

### ASF4 HAL backend
- Registers a RX callback → pushes frames into a SW ring buffer.
- `can_rx_wait()` blocks on ISR signal; `can_tx_ext()` uses `can_async_write()`.
- Forces NBTP at init to 100 k @ 4 MHz (safe if START wasn’t set).

### Bare-metal (HRI) backend
- Programs **NBTP** and enables **RX FIFO0** IRQ.
- **TX**: picks buffer via **TXFQS.TFQPI**, fills **T0/T1+DATA** in MRAM, kicks **TXBAR**, optionally checks **TXBTO**.
- **RX**: computes element address from **RXF0C.F0SA** + **RXESC.F0DS**, reads **ID/DLC/data**, acks **RXF0A**, clears **IR.RF0N**.

---

## Build & flash
1. Open this START project in **Atmel Studio 7** or **MPLAB X**.
2. Confirm **MCAN TX/RX pins** and transceiver enable pin.
3. Ensure **CAN clock = 4 MHz** (or leave it and let the runtime NBTP override handle it).
4. Build & flash to **ATSAMC21J15**.

**Quick HIL test (PC @ 100 kbit/s):**
```bash
cansend can0 020A2008#D200    # ACK example
```
Expected log:
```
[I] ACK V=221.0V I=21.0A
```

---

## Testing

### Host unit tests (no hardware)
```
cd tests/host
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```
Covers EID packing/mask and ×10 scaling.

### HIL helper
```
python3 tools/python/ack_inject.py   # sends the ACK example on can0
```

---

## File map
- `src/app/rectifier.h/.c` – protocol pack/unpack, request/ACK flow, scaling.
- `src/drivers/can_if.h` – portable CAN API used by app.
- `src/drivers/can_if_asf4.c` – ASF4 backend (ISR RX + HAL TX).
- `src/drivers/can_if_mcan.c` – bare-metal backend (**TX+RX**, no HAL).
- `src/config/build_config.h` – backend toggle + NBTP macro.
- `src/sys/log.h` – printf-based logging.
- `src/main.c` – init → filter → send→wait→parse.
- `tests/host` – CMake tests for host-side validation.
- `tools/python/ack_inject.py` – ACK injector.

---

## License
MIT © 2025 **Sripad Madhusudan Upadhyaya** (project sources). Microchip vendor files remain under their original licenses.
