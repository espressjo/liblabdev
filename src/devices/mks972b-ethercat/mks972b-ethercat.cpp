#include "mks972b-ethercat.h"
#include <uics/uics_config.h>
gauge972_e::gauge972_e(std::string configfile, std::string devicename,
                       prometheus::Registry *registry)
    : labdevices(devicename, registry)

{
  timeout = -1;
  cycle = 0;
  pressure = 0.0;
  online = false;
  p_gauge = devicename + "_Pressure";
  addGauge(p_gauge, "Pressure of the MKS972b");
  ADAPTER = cfg_get_string(configfile, "NIC", "");
  if (ADAPTER.empty()) {
    online = false;

    std::cout << "[debug] EtherCat ADAPTER empty" << std::endl;
  } else {
    online = init_ethercat();
    if (online) {
      _thread = std::thread(&gauge972_e::t_main, this);
      std::cout << "[debug] EtherCat thread started" << std::endl;
    } else {

      std::cout << "[debug] EtherCat thread failed to start" << std::endl;
    }
  }
  // Establish the Ethercat connection
}

bool gauge972_e::init_ethercat() {

  if (!ecx_init(&ctx, ADAPTER.c_str())) {
    std::cerr << "No socket connection on " << ADAPTER
              << "\nCheck interface name and run as root.\n";
    return false;
  }
  // v2: ecx_config_init takes only the context (no extra arg)
  int n_slaves = ecx_config_init(&ctx);
  if (n_slaves == 0) {
    std::cerr << "No EtherCAT slaves found.\n";
    ecx_close(&ctx);
    return false;
  }
  std::cout << "Found " << n_slaves << " slave(s):\n";
  gauge_idx = 1;
  for (int i = 1; i <= ctx.slavecount; ++i) {
    std::cout << "  [" << i << "] '" << ctx.slavelist[i].name << "'"
              << "  Vendor=0x" << std::hex << std::setw(8) << std::setfill('0')
              << ctx.slavelist[i].eep_man << "  Product=0x" << std::setw(8)
              << std::setfill('0') << ctx.slavelist[i].eep_id << std::dec
              << '\n';

    bool name_match =
        std::string(ctx.slavelist[i].name).find("390") != std::string::npos ||
        std::string(ctx.slavelist[i].name).find("MKS") != std::string::npos;
    if (ctx.slavelist[i].eep_man == 1291 || name_match)
      gauge_idx = i;
  }
  std::cout << "  Using slave index " << gauge_idx << " ('"
            << ctx.slavelist[gauge_idx].name << "')\n";
  ecx_config_map_group(&ctx, IOmap, 0);
  ecx_configdc(&ctx);

  ecx_statecheck(&ctx, 0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

  ctx.slavelist[0].state = EC_STATE_OPERATIONAL;
  ecx_send_processdata(&ctx);
  ecx_receive_processdata(&ctx, EC_TIMEOUTRET);
  ecx_writestate(&ctx, 0);
  ecx_statecheck(&ctx, 0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE);

  if (ctx.slavelist[0].state != EC_STATE_OPERATIONAL) {
    std::cerr << "Failed to reach OP state (state=" << ctx.slavelist[0].state
              << "). Exiting.\n";
    ecx_close(&ctx);
    return false;
  }

  std::cout << "\nIn OP state. Starting reads...\n\n";
  std::cout << std::setw(8) << "Time"
            << "  " << std::setw(18) << "Pressure (Torr)"
            << "  " << std::setw(26) << "Sensor"
            << "  " << std::setw(5) << "Valid"
            << "  Exception\n";
  std::cout << std::string(80, '-') << '\n';

  return true;
}

void gauge972_e::t_main() {

  while (g_running) {
    auto t0 = std::chrono::steady_clock::now();

    ecx_send_processdata(&ctx);
    ecx_receive_processdata(&ctx, EC_TIMEOUTRET);

    if (DUMP_PDO && cycle < PDO_DUMP_MAX) {
      const uint8_t *pdo = ctx.slavelist[gauge_idx].inputs;
      int len = (int)ctx.slavelist[gauge_idx].Ibytes;
      if (pdo && len > 0)
        dump_pdo(pdo, len);
      else
        std::cout << "  PDO input data not available yet.\n\n";
    }

    try {
      pressure = static_cast<double>(e_read_pressure(&ctx, gauge_idx));
      uint16_t sensor_n = read_sensor_number(&ctx, gauge_idx);
      bool valid = read_reading_valid(&ctx, gauge_idx);
      uint8_t exc = read_exception_status(&ctx, gauge_idx);

      auto it = SENSOR_NAMES.find(sensor_n);
      std::string sname = (it != SENSOR_NAMES.end())
                              ? it->second
                              : ("Sensor #" + std::to_string(sensor_n));
      // setGauge(p_gauge, pressure);
    } catch (const std::exception &e) {
      std::cerr << "  SDO error: " << e.what() << '\n';
      online = false;
    }

    ++cycle;

    auto elapsed = std::chrono::steady_clock::now() - t0;
    auto sleep_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>(CYCLE_TIME_S) - elapsed);
    if (sleep_ns.count() > 0)
      std::this_thread::sleep_for(sleep_ns);
  }
}

void gauge972_e::dump_pdo(const uint8_t *data, int len) {
  std::cout << "  PDO raw (" << len << " bytes): ";
  for (int i = 0; i < len; ++i) {
    if (i)
      std::cout << ' ';
    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
  }
  std::cout << std::dec << '\n';
  std::cout << "  Scanning for float32 pressure value:\n";
  bool found = false;
  for (int off = 0; off <= len - 4; ++off) {
    float v;
    std::memcpy(&v, data + off, 4);
    if (v > 1e-11f && v < 1100.0f) {
      std::cout << "    -> offset [" << off << ':' << off + 4 << "]  "
                << std::scientific << std::setprecision(4) << v
                << " Torr  <- likely pressure\n";
      found = true;
    }
  }
  if (!found)
    std::cout << "    (no plausible pressure float found)\n";
  std::cout << '\n';
}

void gauge972_e::update() {
  std::cout << "ok ethercat gauge" << std::endl;
  if (online) {
    std::cout << "[debug] EtherCat Gauge: " << std::to_string(pressure)
              << std::endl;
    setGauge(p_gauge, pressure);
  }

  std::cout << "[debug] EtherCat Gauge Not Online: " << std::endl;
}
gauge972_e::~gauge972_e() {

  g_running = false;
  if (_thread.joinable()) {
    _thread.join();
  }
}
