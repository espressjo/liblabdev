#ifndef LABDEVICES_H
#define LABDEVICES_H

#include <map>
#include <prometheus/family.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>
#include <string>
#include <vector>

class labdevices {
public:
  labdevices(std::string devicename, prometheus::Registry *registry);
  virtual ~labdevices(); // Add virtual destructor for proper cleanup
  virtual void update();

  void setRegistry(prometheus::Registry *registry);
  bool isOnline();
  std::string device_name;

  //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  //:::   Read back the last value pushed by update(). The      :::
  //:::   gauge is already the cache, no extra state is kept.   :::
  //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  bool getGauge(const std::string &gaugeName, double &value) const;
  // Same, but returns NaN when the gauge does not exist.
  double getGauge(const std::string &gaugeName) const;
  std::vector<std::string> getGaugeNames() const;

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
