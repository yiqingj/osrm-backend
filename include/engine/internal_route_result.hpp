#ifndef RAW_ROUTE_DATA_H
#define RAW_ROUTE_DATA_H

#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"
#include "engine/phantom_node.hpp"
#include "osrm/coordinate.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/typedefs.hpp"

#include <vector>

namespace osrm
{
namespace engine
{

struct PathData
{
    // id of via node of the turn
    NodeID turn_via_node;
    // name of the street that leads to the turn
    unsigned name_id;
    // weight that is traveled on the segment until the turn is reached
    EdgeWeight weight_until_turn;
    // duration that is traveled on the segment until the turn is reached
    EdgeWeight duration_until_turn;
    // instruction to execute at the turn
    extractor::guidance::TurnInstruction turn_instruction;
    // turn lane data
    util::guidance::LaneTupleIdPair lane_data;
    // travel mode of the street that leads to the turn
    extractor::TravelMode travel_mode : 4;
    // entry class of the turn, indicating possibility of turns
    util::guidance::EntryClass entry_class;

    // Source of the speed value on this road segment
    DatasourceID datasource_id;

    // bearing (as seen from the intersection) pre-turn
    util::guidance::TurnBearing pre_turn_bearing;
    // bearing (as seen from the intersection) post-turn
    util::guidance::TurnBearing post_turn_bearing;
};

struct InternalRouteResult
{
    std::vector<std::vector<PathData>> unpacked_path_segments;
    std::vector<PhantomNodes> segment_end_coordinates;
    std::vector<bool> source_traversed_in_reverse;
    std::vector<bool> target_traversed_in_reverse;
    EdgeWeight shortest_path_weight = INVALID_EDGE_WEIGHT;

    bool is_valid() const { return INVALID_EDGE_WEIGHT != shortest_path_weight; }

    bool is_via_leg(const std::size_t leg) const
    {
        return (leg != unpacked_path_segments.size() - 1);
    }
};

struct InternalManyRoutesResult
{
    InternalManyRoutesResult() = default;
    InternalManyRoutesResult(InternalRouteResult route) : routes{std::move(route)} {}
    InternalManyRoutesResult(std::vector<InternalRouteResult> routes_) : routes{std::move(routes_)}
    {
    }

    std::vector<InternalRouteResult> routes;
};
}
}

#endif // RAW_ROUTE_DATA_H
