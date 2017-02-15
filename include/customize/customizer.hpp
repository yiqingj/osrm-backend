#ifndef OSRM_CUSTOMIZE_CUSTOMIZER_HPP
#define OSRM_CUSTOMIZE_CUSTOMIZER_HPP

#include "customize/customizer_config.hpp"

namespace osrm
{
namespace customize
{

// tool access to the recursive partitioner
class Customizer
{
  public:
    int Run(const CustomizationConfig &config);
};

} // namespace partition
} // namespace osrm

#endif // OSRM_CUSTOMIZE_CUSTOMIZER_HPP
