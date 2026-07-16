#include "mks972b-ethercat-utility.h"

float e_read_pressure(ecx_contextt *ctx, int slave_idx) {
  float val = 0.0f;
  int size = sizeof(val);
  if (ecx_SDOread(ctx, slave_idx, 0xF640, 0x11, FALSE, &size, &val,
                  EC_TIMEOUTRXM) <= 0)
    throw std::runtime_error("SDO read failed: 0xF640:11 (pressure)");
  return val;
}

uint16_t read_sensor_number(ecx_contextt *ctx, int slave_idx) {
  uint16_t val = 0;
  int size = sizeof(val);
  if (ecx_SDOread(ctx, slave_idx, 0xF640, 0x12, FALSE, &size, &val,
                  EC_TIMEOUTRXM) <= 0)
    throw std::runtime_error("SDO read failed: 0xF640:12 (sensor number)");
  return val;
}

bool read_reading_valid(ecx_contextt *ctx, int slave_idx) {
  uint8_t val = 0;
  int size = sizeof(val);
  if (ecx_SDOread(ctx, slave_idx, 0xF640, 0x01, FALSE, &size, &val,
                  EC_TIMEOUTRXM) <= 0)
    throw std::runtime_error("SDO read failed: 0xF640:01 (valid)");
  return val != 0;
}

uint8_t read_exception_status(ecx_contextt *ctx, int slave_idx) {
  uint8_t val = 0;
  int size = sizeof(val);
  if (ecx_SDOread(ctx, slave_idx, 0xF380, 0x00, FALSE, &size, &val,
                  EC_TIMEOUTRXM) <= 0)
    throw std::runtime_error("SDO read failed: 0xF380:00 (exception)");
  return val;
}

std::string decode_exception(uint8_t status) {
  std::vector<std::string> bits;
  if (status & 0x01)
    bits.push_back("DeviceWarning");
  if (status & 0x02)
    bits.push_back("MfgWarning");
  if (status & 0x04)
    bits.push_back("DeviceError");
  if (status & 0x08)
    bits.push_back("MfgError");
  if (bits.empty())
    return "OK";
  std::string out;
  for (size_t i = 0; i < bits.size(); ++i) {
    if (i)
      out += ", ";
    out += bits[i];
  }
  return out;
}
