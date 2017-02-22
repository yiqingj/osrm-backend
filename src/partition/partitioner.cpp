#include "partition/partitioner.hpp"
#include "partition/annotated_partition.hpp"
#include "partition/bisection_graph.hpp"
#include "partition/compressed_node_based_graph_reader.hpp"
#include "partition/edge_based_graph_reader.hpp"
#include "partition/node_based_graph_to_edge_based_graph_mapping_reader.hpp"
#include "partition/recursive_bisection.hpp"

#include "customizer/cell_customizer.hpp"
#include "util/cell_storage.hpp"
#include "util/cell_storage.hpp"
#include "util/coordinate.hpp"
#include "util/geojson_debug_logger.hpp"
#include "util/geojson_debug_policies.hpp"
#include "util/integer_range.hpp"
#include "util/json_container.hpp"
#include "util/log.hpp"
#include "util/multi_level_partition.hpp"

#include <algorithm>
#include <iterator>
#include <unordered_set>
#include <vector>

#include <boost/assert.hpp>
#include <boost/container/small_vector.hpp>

#include "util/geojson_debug_logger.hpp"
#include "util/geojson_debug_policies.hpp"
#include "util/json_container.hpp"
#include "util/timing_util.hpp"

namespace osrm
{
namespace partition
{

void LogStatistics(const std::string &filename, std::vector<std::uint32_t> bisection_ids)
{
    auto compressed_node_based_graph = LoadCompressedNodeBasedGraph(filename);

    util::Log() << "Loaded compressed node based graph: "
                << compressed_node_based_graph.edges.size() << " edges, "
                << compressed_node_based_graph.coordinates.size() << " nodes";

    groupEdgesBySource(begin(compressed_node_based_graph.edges),
                       end(compressed_node_based_graph.edges));

    auto graph =
        makeBisectionGraph(compressed_node_based_graph.coordinates,
                           adaptToBisectionEdge(std::move(compressed_node_based_graph.edges)));

    TIMER_START(annotation);
    AnnotatedPartition partition(graph, bisection_ids);
    TIMER_STOP(annotation);
    std::cout << "Annotation took " << TIMER_SEC(annotation) << " seconds" << std::endl;
}

void LogGeojson(const std::string &filename, const std::vector<std::uint32_t> &bisection_ids)
{
    // reload graph, since we destroyed the old one
    auto compressed_node_based_graph = LoadCompressedNodeBasedGraph(filename);

    util::Log() << "Loaded compressed node based graph: "
                << compressed_node_based_graph.edges.size() << " edges, "
                << compressed_node_based_graph.coordinates.size() << " nodes";

    groupEdgesBySource(begin(compressed_node_based_graph.edges),
                       end(compressed_node_based_graph.edges));

    auto graph =
        makeBisectionGraph(compressed_node_based_graph.coordinates,
                           adaptToBisectionEdge(std::move(compressed_node_based_graph.edges)));

    const auto get_level = [](const std::uint32_t lhs, const std::uint32_t rhs) {
        auto xored = lhs ^ rhs;
        std::uint32_t level = log(xored) / log(2.0);
        return level;
    };

    std::vector<std::vector<util::Coordinate>> border_vertices(33);

    for (NodeID nid = 0; nid < graph.NumberOfNodes(); ++nid)
    {
        const auto source_id = bisection_ids[nid];
        for (const auto &edge : graph.Edges(nid))
        {
            const auto target_id = bisection_ids[edge.target];
            if (source_id != target_id)
            {
                auto level = get_level(source_id, target_id);
                border_vertices[level].push_back(graph.Node(nid).coordinate);
                border_vertices[level].push_back(graph.Node(edge.target).coordinate);
            }
        }
    }

    util::ScopedGeojsonLoggerGuard<util::CoordinateVectorToMultiPoint> guard(
        "border_vertices.geojson");
    std::size_t level = 0;
    for (auto &bv : border_vertices)
    {
        if (!bv.empty())
        {
            std::sort(bv.begin(), bv.end(), [](const auto lhs, const auto rhs) {
                return std::tie(lhs.lon, lhs.lat) < std::tie(rhs.lon, rhs.lat);
            });
            bv.erase(std::unique(bv.begin(), bv.end()), bv.end());

            util::json::Object jslevel;
            jslevel.values["level"] = util::json::Number(level++);
            guard.Write(bv, jslevel);
        }
    }
}

int Partitioner::Run(const PartitionConfig &config)
{
    auto compressed_node_based_graph =
        LoadCompressedNodeBasedGraph(config.compressed_node_based_graph_path.string());

    util::Log() << "Loaded compressed node based graph: "
                << compressed_node_based_graph.edges.size() << " edges, "
                << compressed_node_based_graph.coordinates.size() << " nodes";

    groupEdgesBySource(begin(compressed_node_based_graph.edges),
                       end(compressed_node_based_graph.edges));

    auto graph =
        makeBisectionGraph(compressed_node_based_graph.coordinates,
                           adaptToBisectionEdge(std::move(compressed_node_based_graph.edges)));

    util::Log() << " running partition: " << config.maximum_cell_size << " " << config.balance
                << " " << config.boundary_factor << " " << config.num_optimizing_cuts << " "
                << config.small_component_size
                << " # max_cell_size balance boundary cuts small_component_size";
    RecursiveBisection recursive_bisection(graph,
                                           config.maximum_cell_size,
                                           config.balance,
                                           config.boundary_factor,
                                           config.num_optimizing_cuts,
                                           config.small_component_size);

    LogStatistics(config.compressed_node_based_graph_path.string(),
                  recursive_bisection.BisectionIDs());

    // Up until now we worked on the compressed node based graph.
    // But what we actually need is a partition for the edge based graph to work on.
    // The following loads a mapping from node based graph to edge based graph.
    // Then loads the edge based graph tanslates the partition and modifies it.
    // For details see #3205

    auto mapping = LoadNodeBasedGraphToEdgeBasedGraphMapping(config.nbg_ebg_mapping_path.string());
    util::Log() << "Loaded node based graph to edge based graph mapping";

    auto edge_based_graph = LoadEdgeBasedGraph(config.edge_based_graph_path.string());
    util::Log() << "Loaded edge based graph for mapping partition ids: "
                << edge_based_graph->GetNumberOfEdges() << " edges, "
                << edge_based_graph->GetNumberOfNodes() << " nodes";

    // TODO: node based graph to edge based graph partition id mapping should be done split off.

    // Partition ids, keyed by node based graph nodes
    const auto &node_based_partition_ids = recursive_bisection.BisectionIDs();

    // Partition ids, keyed by edge based graph nodes
    std::vector<NodeID> edge_based_partition_ids(edge_based_graph->GetNumberOfNodes());

    // Extract edge based border nodes, based on node based partition and mapping.
    for (const auto node : util::irange(0u, edge_based_graph->GetNumberOfNodes()))
    {
        const auto node_based_nodes = mapping.Lookup(node);

        const auto u = node_based_nodes.u;
        const auto v = node_based_nodes.v;

        if (node_based_partition_ids[u] == node_based_partition_ids[v])
        {
            // Can use partition_ids[u/v] as partition for edge based graph `node_id`
            edge_based_partition_ids[node] = node_based_partition_ids[u];
        }
        else
        {
            // Border nodes u,v - need to be resolved.
            // FIXME: just pick one side for now. See #3205.
            bool use_v = true;
            for (auto edge : edge_based_graph->GetAdjacentEdgeRange(node))
            {
                auto other = edge_based_graph->GetTarget(edge);
                auto other_node_based_nodes = mapping.Lookup(other);
                auto is_uturn = u == other_node_based_nodes.v && v == other_node_based_nodes.u;

                if (!is_uturn)
                { // use partition id of node v if not u-turn and all other nodes in the partition
                    use_v &= node_based_partition_ids[v] ==
                             node_based_partition_ids[other_node_based_nodes.u];
                    use_v &= node_based_partition_ids[v] ==
                             node_based_partition_ids[other_node_based_nodes.v];
                }
            }

            edge_based_partition_ids[node] = node_based_partition_ids[use_v ? v : u];
        }
    }

    if (config.output_customization_data)
    {

        BOOST_ASSERT(edge_based_partition_ids.size() == edge_based_graph->GetNumberOfNodes());

        // find bit size of bisection ids
        int first_nonzero_position = sizeof(BisectionID) * CHAR_BIT;
        for (auto id : edge_based_partition_ids)
        {
            first_nonzero_position = id == 0 ? first_nonzero_position
                                             : std::min(first_nonzero_position, __builtin_ctz(id));
        }
        BOOST_ASSERT(first_nonzero_position != sizeof(BisectionID) * CHAR_BIT);

        // split bisection id bits into groups starting from SCC and stop at level 1
        BOOST_ASSERT(recursive_bisection.SCCDepth() != 0);
        int mask_from = sizeof(BisectionID) * CHAR_BIT - recursive_bisection.SCCDepth();
        boost::container::small_vector<BisectionID, 8> level_masks;
        for (int mask_to = sizeof(BisectionID) * CHAR_BIT; mask_to > first_nonzero_position;
             mask_to = mask_from, mask_from -= 3) // TODO: find better groupping
        {
            auto bit = std::max(first_nonzero_position, mask_from);
            level_masks.push_back(((1u << (sizeof(BisectionID) * CHAR_BIT - bit)) - 1) << bit);
        }

        util::Log() << "Bisection IDs split for SCC depth " << recursive_bisection.SCCDepth()
                    << " and first non-zero bit position " << first_nonzero_position
                    << " number of levels is " << level_masks.size();
        // for (auto x : level_masks) std::cout << std::setw(8) << std::hex << x << std::dec <<
        // "\n";

        // collect cell ids as masked bisection ids
        std::vector<std::vector<osrm::util::CellID>> partitions(
            level_masks.size(), std::vector<osrm::util::CellID>(edge_based_partition_ids.size()));
        std::vector<std::unordered_set<osrm::util::CellID>> partition_sets(level_masks.size());
        for (std::size_t index = 0; index < edge_based_partition_ids.size(); ++index)
        {
            auto bisection_id = edge_based_partition_ids[index];
            for (std::size_t level = 0; level < level_masks.size(); ++level)
            {
                osrm::util::CellID cell_id =
                    bisection_id & level_masks[level_masks.size() - 1 - level];
                partitions[level][index] = cell_id;
                partition_sets[level].insert(cell_id);
            }
        }

        std::vector<std::size_t> level_to_num_cells;
        std::transform(partition_sets.begin(),
                       partition_sets.end(),
                       std::back_inserter(level_to_num_cells),
                       [](const std::unordered_set<osrm::util::CellID> &partition_set) {
                           return partition_set.size();
                       });
        // std::cout << "# of cell on levels\n";
        // for (std::size_t level=0; level<partition_sets.size(); ++level) {
        //     std::cout << level_to_num_cells[level] << ": ";
        //     for (auto x : partition_sets[level]) std:: cout << " " << x;
        //     std::cout << "\n";
        // }

        TIMER_START(packed_mlp);
        osrm::util::PackedMultiLevelPartition mlp{partitions, level_to_num_cells};
        TIMER_STOP(packed_mlp);
        util::Log() << "PackedMultiLevelPartition constructed in " << TIMER_SEC(packed_mlp)
                    << " seconds";

        TIMER_START(cell_storage);
        osrm::util::CellStorage storage(mlp, *edge_based_graph);
        TIMER_STOP(cell_storage);
        util::Log() << "CellStorage constructed in " << TIMER_SEC(cell_storage) << " seconds";

        TIMER_START(cell_customize);
        osrm::customizer::CellCustomizer customizer(mlp);
        customizer.Customize(*edge_based_graph, storage);
        TIMER_STOP(cell_customize);
        util::Log() << "Cells customization took " << TIMER_SEC(cell_customize) << " seconds";
    }

    return 0;
}

} // namespace partition
} // namespace osrm
