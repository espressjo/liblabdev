/**
 * MKS Series 390 Micro-Ion ATM Vacuum Gauge — EtherCAT Reader (C++)
 * ------------------------------------------------------------------
 * Manual ref: MKS p/n 20018109-001
 *
 * Direct port of the Python/pysoem script to C++ using SOEM
 * (Simple Open EtherCAT Master: https://github.com/OpenEtherCATsociety/SOEM).
 *
 * Build (Linux):
 *   g++ -O2 -std=c++17 mks390_ethercat.cpp -lsoem -o mks390_ethercat
 *   sudo ./mks390_ethercat
 *
 * SOEM must be installed (headers + lib).  On Ubuntu:
 *   git clone https://github.com/OpenEtherCATsociety/SOEM
 *   cd SOEM && mkdir build && cd build
 *   cmake .. && make && sudo make install
 */

#include <soem/ethercat.h>

#include <chrono>
#include <csignal>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
static const char *ADAPTER = "enx00e04c68046e";
static const double CYCLE_TIME_S = 0.5; // 2 Hz — SDO reads are ~10 ms each
static const bool DUMP_PDO = true;      // set false once layout is confirmed
static const int PDO_DUMP_MAX = 5;      // only dump the first N cycles

// Sensor number -> human-readable name  (0xF640:12, ETG.5003.2080)
static const std::map<int, std::string> SENSOR_NAMES = {
    {0, "Absolute Piezo"},
    {1, "Differential Piezo"},
    {2, "Heat Transfer (Conductron)"},
    {3, "Hot Cathode Ion"},
    {4, "ATM Absolute Piezo"},
};

// ---------------------------------------------------------------------------
// Signal handling (Ctrl-C)
// ---------------------------------------------------------------------------
static volatile bool g_running = true;
static void signal_handler(int) { g_running = false; }

// ---------------------------------------------------------------------------
// SDO helpers
// ---------------------------------------------------------------------------

/**
 * Read a 4-byte IEEE-754 float from 0xF640:11
 * (Combination Gauge Active Value, Torr).
 */
static float read_pressure(int slave_idx) {
  float value = 0.0f;
  int size = sizeof(value);
  if (ec_SDOread(slave_idx, 0xF640, 0x11, FALSE, &size, &value,
                 EC_TIMEOUTRXM) <= 0)
    throw std::runtime_error("SDO read failed: 0xF640:11 (pressure)");
  return value;
}

/**
 * Read a UINT16 from 0xF640:12 — which sensor is currently active.
 */
static uint16_t read_sensor_number(int slave_idx) {
  uint16_t value = 0;
  int size = sizeof(value);
  if (ec_SDOread(slave_idx, 0xF640, 0x12, FALSE, &size, &value,
                 EC_TIMEOUTRXM) <= 0)
    throw std::runtime_error("SDO read failed: 0xF640:12 (sensor number)");
  return value;
}

/**
 * Read BOOL from 0xF640:01 — combination gauge reading valid.
 */
static bool read_reading_valid(int slave_idx) {
  uint8_t value = 0;
  int size = sizeof(value);
  if (ec_SDOread(slave_idx, 0xF640, 0x01, FALSE, &size, &value,
                 EC_TIMEOUTRXM) <= 0)
    throw std::runtime_error("SDO read failed: 0xF640:01 (valid)");
  return value != 0;
}

/**
 * Read UINT8 from 0xF380:00 — active exception status.
 */
static uint8_t read_exception_status(int slave_idx) {
  uint8_t value = 0;
  int size = sizeof(value);
  if (ec_SDOread(slave_idx, 0xF380, 0x00, FALSE, &size, &value,
                 EC_TIMEOUTRXM) <= 0)
    throw std::runtime_error("SDO read failed: 0xF380:00 (exception)");
  return value;
}

static std::string decode_exception(uint8_t status) {
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

// ---------------------------------------------------------------------------
// PDO debug dump
// ---------------------------------------------------------------------------

/**
 * Print raw PDO bytes and scan every 4-byte offset for a plausible pressure
 * float (1e-11 to 1100 Torr — full range of the 390).
 */
static void dump_pdo(const uint8_t *data, int len) {
  std::cout << "  PDO raw (" << len << " bytes): ";
  for (int i = 0; i < len; ++i) {
    if (i)
      std::cout << ' ';
    std::cout << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(data[i]);
  }
  std::cout << std::dec << '\n';

  std::cout << "  Scanning for float32 pressure value:\n";
  bool found = false;
  for (int offset = 0; offset <= len - 4; ++offset) {
    float val;
    std::memcpy(&val, data + offset, 4);
    if (val > 1e-11f && val < 1100.0f) {
      std::cout << "    -> offset [" << offset << ':' << offset + 4
                << "]  value = " << std::scientific << std::setprecision(4)
                << val << " Torr  <- likely pressure\n";
      found = true;
    }
  }
  if (!found)
    std::cout << "    (no plausible pressure float found in PDO bytes)\n";
  std::cout << '\n';
}

// ---------------------------------------------------------------------------
// Timestamp helper
// ---------------------------------------------------------------------------
static std::string timestamp() {
  std::time_t t = std::time(nullptr);
  char buf[16];
  std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
  return buf;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
  std::signal(SIGINT, signal_handler);

  // Initialise SOEM master on the chosen adapter
  if (!ec_init(ADAPTER)) {
    std::cerr << "No socket connection on " << ADAPTER
              << "\nCheck interface name and run as root.\n";
    return 1;
  }

  int n_slaves = ec_config_init(FALSE);
  if (n_slaves == 0) {
    std::cerr << "No EtherCAT slaves found.\n";
    ec_close();
    return 1;
  }

  std::cout << "Found " << n_slaves << " slave(s):\n";
  int gauge_idx = 1; // SOEM slaves are 1-indexed; default to first
  for (int i = 1; i <= ec_slavecount; ++i) {
    std::cout << "  [" << i << "] '" << ec_slave[i].name << "'"
              << "  Vendor=0x" << std::hex << std::setw(8) << std::setfill('0')
              << ec_slave[i].eep_man << "  Product=0x" << std::setw(8)
              << std::setfill('0') << ec_slave[i].eep_id << std::dec << '\n';

    // Identify MKS 390: vendor ID 1291 (0x0000050B) or name match
    bool name_match =
        std::string(ec_slave[i].name).find("390") != std::string::npos ||
        std::string(ec_slave[i].name).find("MKS") != std::string::npos;
    if (ec_slave[i].eep_man == 1291 || name_match)
      gauge_idx = i;
  }
  std::cout << "  Using slave index " << gauge_idx << " ('"
            << ec_slave[gauge_idx].name << "')\n";

  ec_config_map(nullptr /*&IOmap — pass a buffer if needed for PDO mapping*/);
  ec_configdc();

  // Transition to SAFE-OP
  ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

  // Transition to OP
  ec_slave[0].state = EC_STATE_OPERATIONAL;
  ec_send_processdata();
  ec_receive_processdata(EC_TIMEOUTRET);
  ec_writestate(0);
  ec_statecheck(0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE);

  if (ec_slave[0].state != EC_STATE_OPERATIONAL) {
    std::cerr << "Failed to reach OP state (state=" << ec_slave[0].state
              << "). Exiting.\n";
    ec_close();
    return 1;
  }

  std::cout << "\nIn OP state. Starting reads...\n\n";
  std::cout << std::setw(8) << "Time"
            << "  " << std::setw(18) << "Pressure (Torr)"
            << "  " << std::setw(26) << "Sensor"
            << "  " << std::setw(5) << "Valid"
            << "  Exception\n";
  std::cout << std::string(80, '-') << '\n';

  int cycle = 0;

  // Pre-allocate a local IO map buffer (up to 4096 bytes)
  static char io_map[4096];
  (void)io_map; // used by ec_config_map if called with a buffer

  while (g_running) {
    auto t0 = std::chrono::steady_clock::now();

    // Keep PDO cycle alive (required to stay in OP state)
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);

    // PDO hex dump for the first PDO_DUMP_MAX cycles
    if (DUMP_PDO && cycle < PDO_DUMP_MAX) {
      const uint8_t *pdo_data = ec_slave[gauge_idx].inputs;
      int pdo_len = ec_slave[gauge_idx].Ibytes;
      if (pdo_data && pdo_len > 0)
        dump_pdo(pdo_data, pdo_len);
      else
        std::cout << "  PDO input data not available yet.\n\n";
    }

    // SDO reads — authoritative, no byte-layout ambiguity
    try {
      float pressure = read_pressure(gauge_idx);
      uint16_t sensor_n = read_sensor_number(gauge_idx);
      bool valid = read_reading_valid(gauge_idx);
      uint8_t exc = read_exception_status(gauge_idx);

      auto it = SENSOR_NAMES.find(sensor_n);
      std::string sensor_name = (it != SENSOR_NAMES.end())
                                    ? it->second
                                    : ("Sensor #" + std::to_string(sensor_n));
      std::string exc_str = decode_exception(exc);

      std::cout << std::setw(8) << timestamp() << "  " << std::scientific
                << std::setprecision(4) << std::setw(18) << pressure << "  "
                << std::defaultfloat << std::setw(26) << sensor_name << "  "
                << std::setw(5) << (valid ? "True" : "False") << "  " << exc_str
                << '\n';
    } catch (const std::exception &e) {
      std::cerr << "  SDO error: " << e.what() << '\n';
    }

    ++cycle;

    // Sleep for the remainder of the cycle period
    auto elapsed = std::chrono::steady_clock::now() - t0;
    auto sleep_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>(CYCLE_TIME_S) - elapsed);
    if (sleep_ns.count() > 0)
      std::this_thread::sleep_for(sleep_ns);
  }

  std::cout << "\nStopped by user.\n";

  // Return to INIT state and close
  ec_slave[0].state = EC_STATE_INIT;
  ec_writestate(0);
  ec_close();
  std::cout << "Master closed.\n";
  return 0;
}
