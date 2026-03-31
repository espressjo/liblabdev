"""
MKS Series 390 Micro-Ion ATM Vacuum Gauge — EtherCAT Reader (fixed)
--------------------------------------------------------------------
Manual ref: MKS p/n 20018109-001

The PDO byte-layout in the default TxPDO 0x1BFE is tricky to parse
correctly from the manual alone (bit-packed fields, alignment padding).
This script uses two approaches:

  1. SDO reads  — always correct, no layout ambiguity, ~2 Hz is fine
                  for a vacuum gauge (the device updates at 1 ms internally)
  2. PDO hex dump — prints the raw bytes so you can verify the layout
                    and switch to PDO-based reading once confirmed

Run with:  sudo python mks390_ethercat.py
"""

import pysoem
import struct
import time

ADAPTER      = "enx00e04c68046e"
CYCLE_TIME_S = 0.5    # 2 Hz for SDO-based reading (SDO is blocking ~10 ms each)
DUMP_PDO     = True   # Set False once you no longer need the hex dump

# Sensor number -> name (0xF640:12).
# The 390 combo gauge uses these values per ETG.5003.2080:
#   0 = Absolute Piezo   1 = Differential Piezo
#   2 = Heat Transfer    3 = Hot Cathode Ion
#   4 = ATM Absolute Piezo (the high-pressure piezo on ATM-capable models)
SENSOR_NAMES = {
    0: "Absolute Piezo",
    1: "Differential Piezo",
    2: "Heat Transfer (Conductron)",
    3: "Hot Cathode Ion",
    4: "ATM Absolute Piezo",
}


# -- SDO helpers ---------------------------------------------------------------

def read_pressure(slave) -> float:
    """0xF640:11 -- Combination Gauge Active Value (IEEE-754 float32, Torr)."""
    raw = slave.sdo_read(0xF640, 0x11)
    return struct.unpack("<f", raw)[0]


def read_sensor_number(slave) -> int:
    """0xF640:12 -- Which sensor is currently active (UINT, 16-bit)."""
    raw = slave.sdo_read(0xF640, 0x12)
    return struct.unpack("<H", raw)[0]


def read_reading_valid(slave) -> bool:
    """0xF640:01 -- Combination gauge reading valid (BOOL)."""
    raw = slave.sdo_read(0xF640, 0x01)
    return bool(raw[0])


def read_exception_status(slave) -> int:
    """0xF380:00 -- Active exception status byte."""
    raw = slave.sdo_read(0xF380, 0x00)
    return raw[0]


def decode_exception(status: int) -> str:
    bits = []
    if status & 0x01: bits.append("DeviceWarning")
    if status & 0x02: bits.append("MfgWarning")
    if status & 0x04: bits.append("DeviceError")
    if status & 0x08: bits.append("MfgError")
    return ", ".join(bits) if bits else "OK"


# -- PDO debug dump ------------------------------------------------------------

def dump_pdo(raw: bytes):
    """
    Print raw PDO bytes and scan every 4-byte offset for a plausible pressure
    float (1e-11 to 1100 Torr covers the full range of the 390).
    Use this output to figure out the correct byte offsets for fast PDO parsing.
    """
    print(f"  PDO raw ({len(raw)} bytes): {raw.hex(' ')}")
    print("  Scanning for float32 pressure value:")
    found = False
    for offset in range(0, len(raw) - 3):
        val = struct.unpack_from("<f", raw, offset)[0]
        if 1e-11 < val < 1100:
            print(f"    -> offset [{offset}:{offset+4}]  value = {val:.4e} Torr  <- likely pressure")
            found = True
    if not found:
        print("    (no plausible pressure float found in PDO bytes)")
    print()


# -- Main ----------------------------------------------------------------------

def run():
    master = pysoem.Master()
    master.open(ADAPTER)

    n = master.config_init()
    if n == 0:
        print("No EtherCAT slaves found.")
        master.close()
        return

    print(f"Found {n} slave(s):")
    gauge_slave = None
    for i, slave in enumerate(master.slaves):
        print(f"  [{i}] '{slave.name}'  "
              f"Vendor=0x{slave.man:08X}  Product=0x{slave.id:08X}")
        if slave.man == 1291 or "390" in slave.name or "MKS" in slave.name.upper():
            gauge_slave = slave
    if gauge_slave is None:
        print("  (Could not ID gauge by vendor ID -- using slave 0)")
        gauge_slave = master.slaves[0]

    master.config_map()
    master.state_check(pysoem.SAFEOP_STATE, timeout=3_000_000)
    master.state = pysoem.OP_STATE
    master.write_state()
    master.state_check(pysoem.OP_STATE, timeout=3_000_000)

    if master.state != pysoem.OP_STATE:
        print(f"Failed to reach OP state (state={master.state}). Exiting.")
        master.close()
        return

    print("\nIn OP state. Starting reads...\n")
    print(f"{'Time':>8}  {'Pressure (Torr)':>18}  {'Sensor':>26}  {'Valid':>5}  {'Exception'}")
    print("-" * 80)

    cycle = 0
    try:
        while True:
            t0 = time.perf_counter()

            # Keep PDO cycle alive (required to stay in OP state)
            master.send_processdata()
            master.receive_processdata(timeout=2000)

            # PDO hex dump for the first 5 cycles to help diagnose layout
            if DUMP_PDO and cycle < 5:
                dump_pdo(bytes(gauge_slave.input))

            # SDO reads -- authoritative, no byte-layout ambiguity
            try:
                pressure    = read_pressure(gauge_slave)
                sensor_n    = read_sensor_number(gauge_slave)
                valid       = read_reading_valid(gauge_slave)
                exc         = read_exception_status(gauge_slave)

                sensor_name = SENSOR_NAMES.get(sensor_n, f"Sensor #{sensor_n}")
                exc_str     = decode_exception(exc)
                ts          = time.strftime("%H:%M:%S")

                print(f"{ts:>8}  {pressure:>18.4e}  {sensor_name:>26}  "
                      f"{str(valid):>5}  {exc_str}")

            except pysoem.SdoError as e:
                print(f"  SDO error: {e}")

            cycle += 1
            elapsed = time.perf_counter() - t0
            sleep = CYCLE_TIME_S - elapsed
            if sleep > 0:
                time.sleep(sleep)

    except KeyboardInterrupt:
        print("\nStopped by user.")

    finally:
        master.state = pysoem.INIT_STATE
        master.write_state()
        master.close()
        print("Master closed.")


if __name__ == "__main__":
    run()
