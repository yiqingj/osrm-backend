/*

Copyright (c) 2016, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef CONTRACTOR_CONTRACTOR_HPP
#define CONTRACTOR_CONTRACTOR_HPP

#include "contractor/contractor_config.hpp"
#include "contractor/query_edge.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/edge_based_node_segment.hpp"
#include "util/deallocating_vector.hpp"
#include "util/typedefs.hpp"

#include <string>
#include <vector>

#include <cstddef>

namespace osrm
{
namespace contractor
{

/// Base class of osrm-contract
class Contractor
{
  public:
    using EdgeData = QueryEdge::EdgeData;

    explicit Contractor(const ContractorConfig &config_) : config{config_} {}

    Contractor(const Contractor &) = delete;
    Contractor &operator=(const Contractor &) = delete;

    int Run();

  protected:
    void ContractGraph(const unsigned max_edge_id,
                       util::DeallocatingVector<extractor::EdgeBasedEdge> &edge_based_edge_list,
                       util::DeallocatingVector<QueryEdge> &contracted_edge_list,
                       std::vector<EdgeWeight> &&node_weights,
                       std::vector<bool> &is_core_node,
                       std::vector<float> &inout_node_levels) const;

  private:
    ContractorConfig config;
};
}
}

#endif // PROCESSING_CHAIN_HPP
