#ifndef MKS972B_ETHERCAT_H
#define MKS972B_ETHERCAT_H

#include "labdev_log.h"
#include "labdevices.h"
#include "mks972b-ethercat-utility.h"
#include <thread>

#define NOK -1 /* valeur de retour avec erreur */
#define OK 0

class gauge972_e : public labdevices {

public:
  gauge972_e(std::string configfile, std::string devicename,
             prometheus::Registry *registry);
  ~gauge972_e();
  int read_pressure(double *pressure);
  int read_pirani(double *pressure);
  int read_cold_cathod(double *pressure);
  void update();
  void set_timeout(int to_s);
  double pressure;

private:
  void dprint(std::string msg);
  int timeout;
  std::string p_gauge;
  const double CYCLE_TIME_S = 0.5; // 2 Hz
  const bool DUMP_PDO = true;
  const int PDO_DUMP_MAX = 5;

  const std::map<int, std::string> SENSOR_NAMES = {
      {0, "Absolute Piezo"},
      {1, "Differential Piezo"},
      {2, "Heat Transfer (Conductron)"},
      {3, "Hot Cathode Ion"},
      {4, "ATM Absolute Piezo"},
  };
  bool init_ethercat();
  // ---------------------------------------------------------------------------
  // SOEM v2 context — fully self-contained, just zero-init it
  // ---------------------------------------------------------------------------
  int gauge_idx;
  int cycle;
  ecx_contextt ctx = {};
  uint8_t IOmap[4096];
  std::string ADAPTER;
  void t_main();
  std::thread _thread;
  void dump_pdo(const uint8_t *data, int len);

  // ---------------------------------------------------------------------------
  // Latched faults: t_main() spins at CYCLE_TIME_S, so a persistent problem
  // must not produce one log entry per cycle.
  // ---------------------------------------------------------------------------
  labdev::log_latch sdo_fault;
  labdev::log_latch reading_fault;

  // ---------------------------------------------------------------------------
  // Signal handling
  // ---------------------------------------------------------------------------
  volatile bool g_running = true;
};

#endif
