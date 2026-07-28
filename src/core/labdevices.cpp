#include "labdevices.h"
#include <cmath>
#include <iostream>
labdevices::labdevices(std::string devicename, prometheus::Registry *registry)
    : registry(nullptr) {
  device_name = devicename;
  setRegistry(registry);
  online = false;
}

labdevices::~labdevices() {
  // Clean up: Families are owned by the registry, so we don't delete them
  // Just clear our maps
  gaugeSensor.clear();
  gaugeFamily.clear();
}

bool labdevices::setGauge(std::string gaugeName, double value) {
  if (!registry) {
    return false;
  }

  // Use find() instead of looping through all entries
  auto it = gaugeSensor.find(gaugeName);
  if (it != gaugeSensor.end()) {
    it->second->Set(value);
    return true; // Success!
  }

  return false; // Gauge not found
}
bool labdevices::getGauge(const std::string &gaugeName, double &value) const {
  // prometheus::Gauge holds an std::atomic<double>, so reading the last value
  // pushed by update() from another thread is safe without extra locking.
  auto it = gaugeSensor.find(gaugeName);
  if (it == gaugeSensor.end()) {
    return false;
  }

  value = it->second->Value();
  return true;
}

double labdevices::getGauge(const std::string &gaugeName) const {
  double value = std::nan("");
  getGauge(gaugeName, value);
  return value;
}

std::vector<std::string> labdevices::getGaugeNames() const {
  std::vector<std::string> names;
  names.reserve(gaugeSensor.size());
  for (const auto &entry : gaugeSensor) {
    names.push_back(entry.first);
  }
  return names;
}

bool labdevices::isOnline() { return online; }
bool labdevices::addGauge(std::string name, std::string description) {
  if (!registry) {
    return false;
  }

  // Check if gauge already exists
  if (gaugeSensor.find(name) != gaugeSensor.end()) {
    return false; // Already exists, don't create duplicate
  }
  // Create family and gauge
  prometheus::Family<prometheus::Gauge> *family = &prometheus::BuildGauge()
                                                       .Name(name)
                                                       .Help(description)
                                                       .Register(*registry);

  gaugeFamily[name] = family;

  prometheus::Gauge *gauge = &family->Add({{"Component", device_name}});
  gaugeSensor[name] = gauge;

  return true;
}

void labdevices::setRegistry(prometheus::Registry *registry) {
  this->registry = registry;
}

void labdevices::update() {
  // Do nothing, this is the function that should be implemented per device
}
