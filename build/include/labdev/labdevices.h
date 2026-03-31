#ifndef LABDEVICES_H
#define LABDEVICES_H

#include <map>
#include <prometheus/family.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>
#include <string>

class labdevices {
public:
  labdevices(std::string devicename, prometheus::Registry *registry);
  virtual ~labdevices(); // Add virtual destructor for proper cleanup
  virtual void update();

  void setRegistry(prometheus::Registry *registry);

  std::string device_name;

protected:
  prometheus::Registry *registry;
  bool addGauge(std::string name, std::string description);
  bool setGauge(std::string gaugeName, double value);

  bool online;

private:
  std::map<std::string, prometheus::Gauge *> gaugeSensor;
  std::map<std::string, prometheus::Family<prometheus::Gauge> *> gaugeFamily;
};

#endif
