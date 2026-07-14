/**
 * MKS Series 390 Micro-Ion ATM Vacuum Gauge — EtherCAT Reader (C++, SOEM v2.x)
 * -----------------------------------------------------------------------------
 * Manual ref: MKS p/n 20018109-001
 *
 * Requires SOEM v2.0+  (header: soem/soem.h, lib: libsoem.a)
 *
 * Build:
 *   g++ -O2 -std=c++17 mks390_ethercat.cpp \
 *       -I/usr/local/include/soem \
 *       -L/usr/local/lib \
 *       -lsoem -pthread -o mks390_ethercat
 *   sudo ./mks390_ethercat
 */

#include <soem/soem.h>

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <csignal>
#include <ctime>
#include <vector>

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
static const char*  ADAPTER      = "enp9s0";
static const double CYCLE_TIME_S = 0.5;   // 2 Hz
static const bool   DUMP_PDO     = true;
static const int    PDO_DUMP_MAX = 5;

static const std::map<int, std::string> SENSOR_NAMES = {
    {0, "Absolute Piezo"},
    {1, "Differential Piezo"},
    {2, "Heat Transfer (Conductron)"},
    {3, "Hot Cathode Ion"},
    {4, "ATM Absolute Piezo"},
};

// ---------------------------------------------------------------------------
// SOEM v2 context — fully self-contained, just zero-init it
// ---------------------------------------------------------------------------
static ecx_contextt ctx = {};
static uint8_t      IOmap[4096];

// ---------------------------------------------------------------------------
// Signal handling
// ---------------------------------------------------------------------------
static volatile bool g_running = true;
static void signal_handler(int) { g_running = false; }

// ---------------------------------------------------------------------------
// SDO helpers
// ---------------------------------------------------------------------------
static float read_pressure(int slave_idx)
{
    float val  = 0.0f;
    int   size = sizeof(val);
    if (ecx_SDOread(&ctx, slave_idx, 0xF640, 0x11, FALSE, &size, &val, EC_TIMEOUTRXM) <= 0)
        throw std::runtime_error("SDO read failed: 0xF640:11 (pressure)");
    return val;
}

static uint16_t read_sensor_number(int slave_idx)
{
    uint16_t val  = 0;
    int      size = sizeof(val);
    if (ecx_SDOread(&ctx, slave_idx, 0xF640, 0x12, FALSE, &size, &val, EC_TIMEOUTRXM) <= 0)
        throw std::runtime_error("SDO read failed: 0xF640:12 (sensor number)");
    return val;
}

static bool read_reading_valid(int slave_idx)
{
    uint8_t val  = 0;
    int     size = sizeof(val);
    if (ecx_SDOread(&ctx, slave_idx, 0xF640, 0x01, FALSE, &size, &val, EC_TIMEOUTRXM) <= 0)
        throw std::runtime_error("SDO read failed: 0xF640:01 (valid)");
    return val != 0;
}

static uint8_t read_exception_status(int slave_idx)
{
    uint8_t val  = 0;
    int     size = sizeof(val);
    if (ecx_SDOread(&ctx, slave_idx, 0xF380, 0x00, FALSE, &size, &val, EC_TIMEOUTRXM) <= 0)
        throw std::runtime_error("SDO read failed: 0xF380:00 (exception)");
    return val;
}

static std::string decode_exception(uint8_t status)
{
    std::vector<std::string> bits;
    if (status & 0x01) bits.push_back("DeviceWarning");
    if (status & 0x02) bits.push_back("MfgWarning");
    if (status & 0x04) bits.push_back("DeviceError");
    if (status & 0x08) bits.push_back("MfgError");
    if (bits.empty()) return "OK";
    std::string out;
    for (size_t i = 0; i < bits.size(); ++i) { if (i) out += ", "; out += bits[i]; }
    return out;
}

// ---------------------------------------------------------------------------
// PDO debug dump
// ---------------------------------------------------------------------------
static void dump_pdo(const uint8_t* data, int len)
{
    std::cout << "  PDO raw (" << len << " bytes): ";
    for (int i = 0; i < len; ++i) {
        if (i) std::cout << ' ';
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    std::cout << std::dec << '\n';
    std::cout << "  Scanning for float32 pressure value:\n";
    bool found = false;
    for (int off = 0; off <= len - 4; ++off) {
        float v; std::memcpy(&v, data + off, 4);
        if (v > 1e-11f && v < 1100.0f) {
            std::cout << "    -> offset [" << off << ':' << off+4 << "]  "
                      << std::scientific << std::setprecision(4)
                      << v << " Torr  <- likely pressure\n";
            found = true;
        }
    }
    if (!found) std::cout << "    (no plausible pressure float found)\n";
    std::cout << '\n';
}

// ---------------------------------------------------------------------------
// Timestamp
// ---------------------------------------------------------------------------
static std::string timestamp()
{
    std::time_t t = std::time(nullptr);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    return buf;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main()
{
    std::signal(SIGINT, signal_handler);

    if (!ecx_init(&ctx, ADAPTER)) {
        std::cerr << "No socket connection on " << ADAPTER
                  << "\nCheck interface name and run as root.\n";
        return 1;
    }

    // v2: ecx_config_init takes only the context (no extra arg)
    int n_slaves = ecx_config_init(&ctx);
    if (n_slaves == 0) {
        std::cerr << "No EtherCAT slaves found.\n";
        ecx_close(&ctx);
        return 1;
    }

    std::cout << "Found " << n_slaves << " slave(s):\n";
    int gauge_idx = 1;
    for (int i = 1; i <= ctx.slavecount; ++i) {
        std::cout << "  [" << i << "] '" << ctx.slavelist[i].name << "'"
                  << "  Vendor=0x"  << std::hex << std::setw(8) << std::setfill('0')
                  << ctx.slavelist[i].eep_man
                  << "  Product=0x" << std::setw(8) << std::setfill('0')
                  << ctx.slavelist[i].eep_id << std::dec << '\n';

        bool name_match =
            std::string(ctx.slavelist[i].name).find("390") != std::string::npos ||
            std::string(ctx.slavelist[i].name).find("MKS") != std::string::npos;
        if (ctx.slavelist[i].eep_man == 1291 || name_match)
            gauge_idx = i;
    }
    std::cout << "  Using slave index " << gauge_idx
              << " ('" << ctx.slavelist[gauge_idx].name << "')\n";

    ecx_config_map_group(&ctx, IOmap, 0);
    ecx_configdc(&ctx);

    ecx_statecheck(&ctx, 0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

    ctx.slavelist[0].state = EC_STATE_OPERATIONAL;
    ecx_send_processdata(&ctx);
    ecx_receive_processdata(&ctx, EC_TIMEOUTRET);
    ecx_writestate(&ctx, 0);
    ecx_statecheck(&ctx, 0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE);

    if (ctx.slavelist[0].state != EC_STATE_OPERATIONAL) {
        std::cerr << "Failed to reach OP state (state="
                  << ctx.slavelist[0].state << "). Exiting.\n";
        ecx_close(&ctx);
        return 1;
    }

    std::cout << "\nIn OP state. Starting reads...\n\n";
    std::cout << std::setw(8)  << "Time"
              << "  " << std::setw(18) << "Pressure (Torr)"
              << "  " << std::setw(26) << "Sensor"
              << "  " << std::setw(5)  << "Valid"
              << "  Exception\n";
    std::cout << std::string(80, '-') << '\n';

    int cycle = 0;
    while (g_running) {
        auto t0 = std::chrono::steady_clock::now();

        ecx_send_processdata(&ctx);
        ecx_receive_processdata(&ctx, EC_TIMEOUTRET);

        if (DUMP_PDO && cycle < PDO_DUMP_MAX) {
            const uint8_t* pdo = ctx.slavelist[gauge_idx].inputs;
            int            len = (int)ctx.slavelist[gauge_idx].Ibytes;
            if (pdo && len > 0) dump_pdo(pdo, len);
            else std::cout << "  PDO input data not available yet.\n\n";
        }

        try {
            float    pressure = read_pressure(gauge_idx);
            uint16_t sensor_n = read_sensor_number(gauge_idx);
            bool     valid    = read_reading_valid(gauge_idx);
            uint8_t  exc      = read_exception_status(gauge_idx);

            auto it = SENSOR_NAMES.find(sensor_n);
            std::string sname = (it != SENSOR_NAMES.end())
                ? it->second : ("Sensor #" + std::to_string(sensor_n));

            std::cout << std::setw(8) << timestamp()
                      << "  " << std::scientific << std::setprecision(4)
                      << std::setw(18) << pressure
                      << "  " << std::defaultfloat
                      << std::setw(26) << sname
                      << "  " << std::setw(5) << (valid ? "True" : "False")
                      << "  " << decode_exception(exc) << '\n';
        }
        catch (const std::exception& e) {
            std::cerr << "  SDO error: " << e.what() << '\n';
        }

        ++cycle;

        auto elapsed  = std::chrono::steady_clock::now() - t0;
        auto sleep_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::duration<double>(CYCLE_TIME_S) - elapsed);
        if (sleep_ns.count() > 0)
            std::this_thread::sleep_for(sleep_ns);
    }

    std::cout << "\nStopped by user.\n";
    ctx.slavelist[0].state = EC_STATE_INIT;
    ecx_writestate(&ctx, 0);
    ecx_close(&ctx);
    std::cout << "Master closed.\n";
    return 0;
}
