#ifndef OSRM_EXTRACTOR_GUIDANCE_DRIVEWAY_HANDLER_HPP
#define OSRM_EXTRACTOR_GUIDANCE_DRIVEWAY_HANDLER_HPP

#include "extractor/guidance/intersection_handler.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{

// Intersection handlers deal with all issues related to intersections.
class DrivewayHandler final : public IntersectionHandler
{
  public:
    DrivewayHandler(const IntersectionGenerator &intersection_generator,
                    const util::NodeBasedDynamicGraph &node_based_graph,
                    const std::vector<util::Coordinate> &coordinates,
                    const util::NameTable &name_table,
                    const SuffixTable &street_name_suffix_table);

    ~DrivewayHandler() override final = default;

    // check whether the handler can actually handle the intersection
    bool canProcess(const NodeID nid,
                    const EdgeID via_eid,
                    const Intersection &intersection) const override final;

    // process the intersection
    Intersection operator()(const NodeID nid,
                            const EdgeID via_eid,
                            Intersection intersection) const override final;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_DRIVEWAY_HANDLER_HPP */
