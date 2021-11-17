// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once
#include "node_context.hpp"

namespace ov {
namespace frontend {
namespace pdpd {
namespace op {
ngraph::op::PadType get_auto_pad(const NodeContext& node);
std::pair<CoordinateDiff, CoordinateDiff> get_pads(const NodeContext& node);
std::shared_ptr<Node> get_reshaped_filter(const Output<Node>& filters, int32_t groups);

template <typename T1, typename T2>
NamedOutputs conv2d_base(const NodeContext& node) {
    auto data = node.get_ng_input("Input");
    auto filters = node.get_ng_input("Filter");

    const auto strides = node.get_attribute<std::vector<int32_t>>("strides");
    const auto dilations = node.get_attribute<std::vector<int32_t>>("dilations");
    const auto auto_pad_type = get_auto_pad(node);
    const auto paddings = get_pads(node);
    const auto pads_begin = paddings.first;
    const auto pads_end = paddings.second;
    const auto groups = node.get_attribute<int32_t>("groups");
    const auto data_format = node.get_attribute<std::string>("data_format");
    // TODO Support Other data layout #55423
    PDPD_ASSERT(data_format == "NCHW", "conv2d only supports NCHW now");

    if (groups > 1) {
        const auto reshaped_filters = get_reshaped_filter(filters, groups);

        return node.default_single_output_mapping(
            {std::make_shared<T1>(data,
                                  reshaped_filters,
                                  ngraph::Strides(strides.begin(), strides.end()),
                                  pads_begin,
                                  pads_end,
                                  ngraph::Strides(dilations.begin(), dilations.end()),
                                  auto_pad_type)},
            {"Output"});
    } else {
        return node.default_single_output_mapping(
            {std::make_shared<T2>(data,
                                  filters,
                                  ngraph::Strides(strides.begin(), strides.end()),
                                  pads_begin,
                                  pads_end,
                                  ngraph::Strides(dilations.begin(), dilations.end()),
                                  auto_pad_type)},
            {"Output"});
    }
}

}  // namespace op
}  // namespace pdpd
}  // namespace frontend
}  // namespace ov
