#!/usr/bin/env python3
# Inject a rectifier ACK on SocketCAN (Linux).
# EID=0x020A2008 (ID=2, V=221.0 => 0x08A2), payload D2 00 (I=21.0 => 0x00D2)
import can
bus = can.interface.Bus(bustype="socketcan", channel="can0", bitrate=100000)
msg = can.Message(arbitration_id=0x020A2008, data=bytes([0xD2,0x00]), is_extended_id=True)
bus.send(msg)
print("ACK sent:", msg)
