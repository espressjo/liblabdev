#include "dummydevice.h"
#include <iostream>
#include <random>

dummydevice::dummydevice(std::string devicename = "",
                         prometheus::Registry *registry = nullptr)
    : labdevices(devicename, registry)

{

  addGauge("Dummy", "Dummy Device");
}
void dummydevice::set_timeout(int to_s) { timeout = to_s; }
int dummydevice::read_dummy(double *value, double noise) {

  static std::mt19937 rng(std::random_device{}());
  std::normal_distribution<double> dist(0.0, noise);
  *value = 5 + dist(rng);

  return OK;
}
void dummydevice::update() {

  // 1. Create a random device (seed)

  // std::random_device rd;

  // 2. Initialize a Mersenne Twister engine with the seed
  // std::mt19937 gen(rd());

  // 3. Define the distribution (range [min, max))
  // std::uniform_real_distribution<double> dis(0.0,
  double value = 0.0;
  if (read_dummy(&value, 0.6) == OK) {
    online = true;
    setGauge("Dummy", value);
std::cout<<"[debug] setting dummy gauge to"<<std::to_string(value)<<std::endl;
  } else {
    online = false;
  }
}
