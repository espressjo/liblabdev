#include "labdevices.h"
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
