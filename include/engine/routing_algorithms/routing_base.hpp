#ifndef OSRM_ENGINE_ROUTING_BASE_HPP
#define OSRM_ENGINE_ROUTING_BASE_HPP

#include "extractor/guidance/turn_instruction.hpp"

#include "engine/algorithm.hpp"
#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/search_engine_data.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{

namespace routing_algorithms
{
static constexpr bool FORWARD_DIRECTION = true;
static constexpr bool REVERSE_DIRECTION = false;
static constexpr bool DO_NOT_FORCE_LOOPS = false;

bool needsLoopForward(const PhantomNode &source_phantom, const PhantomNode &target_phantom);

bool needsLoopBackwards(const PhantomNode &source_phantom, const PhantomNode &target_phantom);

template <typename Heap>
void insertNodesInHeaps(Heap &forward_heap, Heap &reverse_heap, const PhantomNodes &nodes)
{
    const auto &source = nodes.source_phantom;
    if (source.IsValidForwardSource())
    {
        forward_heap.Insert(source.forward_segment_id.id,
                            -source.GetForwardWeightPlusOffset(),
                            source.forward_segment_id.id);
    }

    if (source.IsValidReverseSource())
    {
        forward_heap.Insert(source.reverse_segment_id.id,
                            -source.GetReverseWeightPlusOffset(),
                            source.reverse_segment_id.id);
    }

    const auto &target = nodes.target_phantom;
    if (target.IsValidForwardTarget())
    {
        reverse_heap.Insert(target.forward_segment_id.id,
                            target.GetForwardWeightPlusOffset(),
                            target.forward_segment_id.id);
    }

    if (target.IsValidReverseTarget())
    {
        reverse_heap.Insert(target.reverse_segment_id.id,
                            target.GetReverseWeightPlusOffset(),
                            target.reverse_segment_id.id);
    }
}

template <typename ManyToManyQueryHeap>
void insertSourceInHeap(ManyToManyQueryHeap &heap, const PhantomNode &phantom_node)
{
    if (phantom_node.IsValidForwardSource())
    {
        heap.Insert(phantom_node.forward_segment_id.id,
                    -phantom_node.GetForwardWeightPlusOffset(),
                    {phantom_node.forward_segment_id.id, -phantom_node.GetForwardDuration()});
    }
    if (phantom_node.IsValidReverseSource())
    {
        heap.Insert(phantom_node.reverse_segment_id.id,
                    -phantom_node.GetReverseWeightPlusOffset(),
                    {phantom_node.reverse_segment_id.id, -phantom_node.GetReverseDuration()});
    }
}

template <typename ManyToManyQueryHeap>
void insertTargetInHeap(ManyToManyQueryHeap &heap, const PhantomNode &phantom_node)
{
    if (phantom_node.IsValidForwardTarget())
    {
        heap.Insert(phantom_node.forward_segment_id.id,
                    phantom_node.GetForwardWeightPlusOffset(),
                    {phantom_node.forward_segment_id.id, phantom_node.GetForwardDuration()});
    }
    if (phantom_node.IsValidReverseTarget())
    {
        heap.Insert(phantom_node.reverse_segment_id.id,
                    phantom_node.GetReverseWeightPlusOffset(),
                    {phantom_node.reverse_segment_id.id, phantom_node.GetReverseDuration()});
    }
}

template <typename FacadeT>
void annotatePath(const FacadeT &facade,
                  const PhantomNodes &phantom_node_pair,
                  const std::vector<NodeID> &unpacked_nodes,
                  const std::vector<EdgeID> &unpacked_edges,
                  std::vector<PathData> &unpacked_path)
{
    BOOST_ASSERT(!unpacked_nodes.empty());
    BOOST_ASSERT(unpacked_nodes.size() == unpacked_edges.size() + 1);

    const auto source_node_id = unpacked_nodes.front();
    const auto target_node_id = unpacked_nodes.back();
    const bool start_traversed_in_reverse =
        phantom_node_pair.source_phantom.forward_segment_id.id != source_node_id;
    const bool target_traversed_in_reverse =
        phantom_node_pair.target_phantom.forward_segment_id.id != target_node_id;

    BOOST_ASSERT(phantom_node_pair.source_phantom.forward_segment_id.id == source_node_id ||
                 phantom_node_pair.source_phantom.reverse_segment_id.id == source_node_id);
    BOOST_ASSERT(phantom_node_pair.target_phantom.forward_segment_id.id == target_node_id ||
                 phantom_node_pair.target_phantom.reverse_segment_id.id == target_node_id);

    auto node_from = unpacked_nodes.begin(), node_last = std::prev(unpacked_nodes.end());
    for (auto edge = unpacked_edges.begin(); node_from != node_last; ++node_from, ++edge)
    {
        const auto &edge_data = facade.GetEdgeData(*edge);
        const auto turn_id = edge_data.turn_id; // edge-based graph edge index
        const auto node_id = *node_from;        // edge-based graph node index
        const auto name_index = facade.GetNameIndex(node_id);
        const auto turn_instruction = facade.GetTurnInstructionForEdgeID(turn_id);
        const extractor::TravelMode travel_mode = facade.GetTravelMode(node_id);

        const auto geometry_index = facade.GetGeometryIndex(node_id);
        std::vector<NodeID> id_vector;

        std::vector<EdgeWeight> weight_vector;
        std::vector<EdgeWeight> duration_vector;
        std::vector<DatasourceID> datasource_vector;
        if (geometry_index.forward)
        {
            id_vector = facade.GetUncompressedForwardGeometry(geometry_index.id);
            weight_vector = facade.GetUncompressedForwardWeights(geometry_index.id);
            duration_vector = facade.GetUncompressedForwardDurations(geometry_index.id);
            datasource_vector = facade.GetUncompressedForwardDatasources(geometry_index.id);
        }
        else
        {
            id_vector = facade.GetUncompressedReverseGeometry(geometry_index.id);
            weight_vector = facade.GetUncompressedReverseWeights(geometry_index.id);
            duration_vector = facade.GetUncompressedReverseDurations(geometry_index.id);
            datasource_vector = facade.GetUncompressedReverseDatasources(geometry_index.id);
        }
        BOOST_ASSERT(id_vector.size() > 0);
        BOOST_ASSERT(datasource_vector.size() > 0);
        BOOST_ASSERT(weight_vector.size() == id_vector.size() - 1);
        BOOST_ASSERT(duration_vector.size() == id_vector.size() - 1);
        const bool is_first_segment = unpacked_path.empty();

        const std::size_t start_index =
            (is_first_segment ? ((start_traversed_in_reverse)
                                     ? weight_vector.size() -
                                           phantom_node_pair.source_phantom.fwd_segment_position - 1
                                     : phantom_node_pair.source_phantom.fwd_segment_position)
                              : 0);
        const std::size_t end_index = weight_vector.size();

        BOOST_ASSERT(start_index >= 0);
        BOOST_ASSERT(start_index < end_index);
        for (std::size_t segment_idx = start_index; segment_idx < end_index; ++segment_idx)
        {
            unpacked_path.push_back(PathData{id_vector[segment_idx + 1],
                                             name_index,
                                             weight_vector[segment_idx],
                                             duration_vector[segment_idx],
                                             extractor::guidance::TurnInstruction::NO_TURN(),
                                             {{0, INVALID_LANEID}, INVALID_LANE_DESCRIPTIONID},
                                             travel_mode,
                                             EMPTY_ENTRY_CLASS,
                                             datasource_vector[segment_idx],
                                             util::guidance::TurnBearing(0),
                                             util::guidance::TurnBearing(0)});
        }
        BOOST_ASSERT(unpacked_path.size() > 0);
        if (facade.HasLaneData(turn_id))
            unpacked_path.back().lane_data = facade.GetLaneData(turn_id);

        unpacked_path.back().entry_class = facade.GetEntryClass(turn_id);
        unpacked_path.back().turn_instruction = turn_instruction;
        unpacked_path.back().duration_until_turn += facade.GetDurationPenaltyForEdgeID(turn_id);
        unpacked_path.back().weight_until_turn += facade.GetWeightPenaltyForEdgeID(turn_id);
        unpacked_path.back().pre_turn_bearing = facade.PreTurnBearing(turn_id);
        unpacked_path.back().post_turn_bearing = facade.PostTurnBearing(turn_id);
    }

    std::size_t start_index = 0, end_index = 0;
    std::vector<unsigned> id_vector;
    std::vector<EdgeWeight> weight_vector;
    std::vector<EdgeWeight> duration_vector;
    std::vector<DatasourceID> datasource_vector;
    const auto source_geometry_id = facade.GetGeometryIndex(source_node_id).id;
    const auto target_geometry_id = facade.GetGeometryIndex(target_node_id).id;
    const auto is_local_path = source_geometry_id == target_geometry_id && unpacked_path.empty();

    if (target_traversed_in_reverse)
    {
        id_vector = facade.GetUncompressedReverseGeometry(target_geometry_id);
        weight_vector = facade.GetUncompressedReverseWeights(target_geometry_id);
        duration_vector = facade.GetUncompressedReverseDurations(target_geometry_id);
        datasource_vector = facade.GetUncompressedReverseDatasources(target_geometry_id);

        if (is_local_path)
        {
            start_index =
                weight_vector.size() - phantom_node_pair.source_phantom.fwd_segment_position - 1;
        }
        end_index =
            weight_vector.size() - phantom_node_pair.target_phantom.fwd_segment_position - 1;
    }
    else
    {
        if (is_local_path)
        {
            start_index = phantom_node_pair.source_phantom.fwd_segment_position;
        }
        end_index = phantom_node_pair.target_phantom.fwd_segment_position;

        id_vector = facade.GetUncompressedForwardGeometry(target_geometry_id);
        weight_vector = facade.GetUncompressedForwardWeights(target_geometry_id);
        duration_vector = facade.GetUncompressedForwardDurations(target_geometry_id);
        datasource_vector = facade.GetUncompressedForwardDatasources(target_geometry_id);
    }

    // Given the following compressed geometry:
    // U---v---w---x---y---Z
    //    s           t
    // s: fwd_segment 0
    // t: fwd_segment 3
    // -> (U, v), (v, w), (w, x)
    // note that (x, t) is _not_ included but needs to be added later.
    for (std::size_t segment_idx = start_index; segment_idx != end_index;
         (start_index < end_index ? ++segment_idx : --segment_idx))
    {
        BOOST_ASSERT(segment_idx < id_vector.size() - 1);
        BOOST_ASSERT(facade.GetTravelMode(target_node_id) > 0);
        unpacked_path.push_back(
            PathData{id_vector[start_index < end_index ? segment_idx + 1 : segment_idx - 1],
                     facade.GetNameIndex(target_node_id),
                     weight_vector[segment_idx],
                     duration_vector[segment_idx],
                     extractor::guidance::TurnInstruction::NO_TURN(),
                     {{0, INVALID_LANEID}, INVALID_LANE_DESCRIPTIONID},
                     facade.GetTravelMode(target_node_id),
                     EMPTY_ENTRY_CLASS,
                     datasource_vector[segment_idx],
                     util::guidance::TurnBearing(0),
                     util::guidance::TurnBearing(0)});
    }

    if (unpacked_path.size() > 0)
    {
        const auto source_weight = start_traversed_in_reverse
                                       ? phantom_node_pair.source_phantom.reverse_weight
                                       : phantom_node_pair.source_phantom.forward_weight;
        const auto source_duration = start_traversed_in_reverse
                                         ? phantom_node_pair.source_phantom.reverse_duration
                                         : phantom_node_pair.source_phantom.forward_duration;
        // The above code will create segments for (v, w), (w,x), (x, y) and (y, Z).
        // However the first segment duration needs to be adjusted to the fact that the source
        // phantom is in the middle of the segment. We do this by subtracting v--s from the
        // duration.

        // Since it's possible duration_until_turn can be less than source_weight here if
        // a negative enough turn penalty is used to modify this edge weight during
        // osrm-contract, we clamp to 0 here so as not to return a negative duration
        // for this segment.

        // TODO this creates a scenario where it's possible the duration from a phantom
        // node to the first turn would be the same as from end to end of a segment,
        // which is obviously incorrect and not ideal...
        unpacked_path.front().weight_until_turn =
            std::max(unpacked_path.front().weight_until_turn - source_weight, 0);
        unpacked_path.front().duration_until_turn =
            std::max(unpacked_path.front().duration_until_turn - source_duration, 0);
    }

    // there is no equivalent to a node-based node in an edge-expanded graph.
    // two equivalent routes may start (or end) at different node-based edges
    // as they are added with the offset how much "weight" on the edge
    // has already been traversed. Depending on offset one needs to remove
    // the last node.
    if (unpacked_path.size() > 1)
    {
        const std::size_t last_index = unpacked_path.size() - 1;
        const std::size_t second_to_last_index = last_index - 1;

        if (unpacked_path[last_index].turn_via_node ==
            unpacked_path[second_to_last_index].turn_via_node)
        {
            unpacked_path.pop_back();
        }
        BOOST_ASSERT(!unpacked_path.empty());
    }
}

template <typename Algorithm>
double getPathDistance(const datafacade::ContiguousInternalMemoryDataFacade<Algorithm> &facade,
                       const std::vector<PathData> unpacked_path,
                       const PhantomNode &source_phantom,
                       const PhantomNode &target_phantom)
{
    using util::coordinate_calculation::detail::DEGREE_TO_RAD;
    using util::coordinate_calculation::detail::EARTH_RADIUS;

    double distance = 0;
    double prev_lat =
        static_cast<double>(util::toFloating(source_phantom.location.lat)) * DEGREE_TO_RAD;
    double prev_lon =
        static_cast<double>(util::toFloating(source_phantom.location.lon)) * DEGREE_TO_RAD;
    double prev_cos = std::cos(prev_lat);
    for (const auto &p : unpacked_path)
    {
        const auto current_coordinate = facade.GetCoordinateOfNode(p.turn_via_node);

        const double current_lat =
            static_cast<double>(util::toFloating(current_coordinate.lat)) * DEGREE_TO_RAD;
        const double current_lon =
            static_cast<double>(util::toFloating(current_coordinate.lon)) * DEGREE_TO_RAD;
        const double current_cos = std::cos(current_lat);

        const double sin_dlon = std::sin((prev_lon - current_lon) / 2.0);
        const double sin_dlat = std::sin((prev_lat - current_lat) / 2.0);

        const double aharv = sin_dlat * sin_dlat + prev_cos * current_cos * sin_dlon * sin_dlon;
        const double charv = 2. * std::atan2(std::sqrt(aharv), std::sqrt(1.0 - aharv));
        distance += EARTH_RADIUS * charv;

        prev_lat = current_lat;
        prev_lon = current_lon;
        prev_cos = current_cos;
    }

    const double current_lat =
        static_cast<double>(util::toFloating(target_phantom.location.lat)) * DEGREE_TO_RAD;
    const double current_lon =
        static_cast<double>(util::toFloating(target_phantom.location.lon)) * DEGREE_TO_RAD;
    const double current_cos = std::cos(current_lat);

    const double sin_dlon = std::sin((prev_lon - current_lon) / 2.0);
    const double sin_dlat = std::sin((prev_lat - current_lat) / 2.0);

    const double aharv = sin_dlat * sin_dlat + prev_cos * current_cos * sin_dlon * sin_dlon;
    const double charv = 2. * std::atan2(std::sqrt(aharv), std::sqrt(1.0 - aharv));
    distance += EARTH_RADIUS * charv;

    return distance;
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif // OSRM_ENGINE_ROUTING_BASE_HPP
