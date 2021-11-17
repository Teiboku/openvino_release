// Copyright (C) 2018-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "ngraph/op/op.hpp"
#include "openvino/op/roi_align.hpp"

namespace ngraph {
namespace op {
namespace v3 {
using ov::op::v3::ROIAlign;
}  // namespace v3
using v3::ROIAlign;
}  // namespace op
}  // namespace ngraph
