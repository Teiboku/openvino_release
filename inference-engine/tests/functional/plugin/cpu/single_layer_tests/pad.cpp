// Copyright (C) 2018-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <shared_test_classes/single_layer/pad.hpp>
#include "test_utils/cpu_test_utils.hpp"
#include "shared_test_classes/base/ov_subgraph.hpp"

using namespace InferenceEngine;
using namespace CPUTestUtils;
using namespace ov;
using namespace test;

namespace CPULayerTestsDefinitions {

using PadLayerCPUTestParamSet = std::tuple<
        InputShape,                                     // Input shape
        ElementType,                                    // Input element type
        std::vector<int64_t>,                           // padsBegin
        std::vector<int64_t>,                           // padsEnd
        float,                                          // argPadValue
        ngraph::helpers::PadMode,                       // padMode
        CPUSpecificParams
>;

class PadLayerCPUTest : public testing::WithParamInterface<PadLayerCPUTestParamSet>,
                        virtual public SubgraphBaseTest, public CPUTestsBase {
public:
    static std::string getTestCaseName(testing::TestParamInfo<PadLayerCPUTestParamSet> obj) {
        InputShape shapes;
        ElementType elementType;
        std::vector<int64_t> padsBegin, padsEnd;
        ngraph::helpers::PadMode padMode;
        float argPadValue;
        CPUSpecificParams cpuParams;
        std::tie(shapes, elementType, padsBegin, padsEnd, argPadValue, padMode, cpuParams) = obj.param;

        std::ostringstream results;
        results << "IS=" << CommonTestUtils::partialShape2str({shapes.first}) << "_";
        results << "TS=";
        for (const auto& item : shapes.second) {
            results << CommonTestUtils::vec2str(item) << "_";
        }
        results << "Prc=" << elementType << "_";
        results << "padsBegin=" << CommonTestUtils::vec2str(padsBegin) << "_";
        results << "padsEnd=" << CommonTestUtils::vec2str(padsEnd) << "_";
        if (padMode == ngraph::helpers::PadMode::CONSTANT) {
            results << "Value=" << argPadValue << "_";
        }
        results << "PadMode=" << padMode << "_";
        results << CPUTestsBase::getTestCaseName(cpuParams);

        return results.str();
    }

protected:
    void SetUp() override {
        InputShape shapes;
        ElementType elementType;
        std::vector<int64_t> padsBegin, padsEnd;
        ngraph::helpers::PadMode padMode;
        float argPadValue;
        CPUSpecificParams cpuParams;
        std::tie(shapes, elementType, padsBegin, padsEnd, argPadValue, padMode, cpuParams) = this->GetParam();

        std::tie(inFmts, outFmts, priority, selectedType) = cpuParams;
        if (selectedType.empty()) {
            selectedType = getPrimitiveType();
        }
        selectedType = selectedType + "_" + InferenceEngine::details::convertPrecision(inType).name();
        targetDevice = CommonTestUtils::DEVICE_CPU;
        init_input_shapes({shapes});

        auto params = ngraph::builder::makeDynamicParams(elementType, inputDynamicShapes);
        auto pad = ngraph::builder::makePad(params[0], padsBegin, padsEnd, argPadValue, padMode);
        pad->get_rt_info() = getCPUInfo();
        ngraph::ResultVector results{std::make_shared<ngraph::opset3::Result>(pad)};
        function = std::make_shared<ngraph::Function>(results, params, "pad");
    }
};

TEST_P(PadLayerCPUTest, CompareWithRefs) {
    SKIP_IF_CURRENT_TEST_IS_DISABLED()

    run();

    // TODO: need to uncomment when this method will be updated
    //CheckPluginRelatedResults(executableNetwork, "Pad");
}

namespace {


const auto cpuParams_nChw16c = CPUSpecificParams {{nChw16c}, {nChw16c}, {}, {}};
const auto cpuParams_nCdhw16c = CPUSpecificParams {{nCdhw16c}, {nCdhw16c}, {}, {}};

const auto cpuParams_nChw8c = CPUSpecificParams {{nChw8c}, {nChw8c}, {}, {}};
const auto cpuParams_nCdhw8c = CPUSpecificParams {{nCdhw8c}, {nCdhw8c}, {}, {}};

const auto cpuParams_nhwc = CPUSpecificParams {{nhwc}, {nhwc}, {}, {}};
const auto cpuParams_ndhwc = CPUSpecificParams {{ndhwc}, {ndhwc}, {}, {}};

const auto cpuParams_nchw = CPUSpecificParams {{nchw}, {nchw}, {}, {}};
const auto cpuParams_ncdhw = CPUSpecificParams {{ncdhw}, {ncdhw}, {}, {}};

const std::vector<ElementType> inputPrecisions = {
        ElementType::f32,
        ElementType::bf16,
        ElementType::i8
};

const std::vector<float> argPadValue = {0.f, 1.f, 2.5f, -1.f};

const std::vector<ngraph::helpers::PadMode> padMode = {
        ngraph::helpers::PadMode::EDGE,
        ngraph::helpers::PadMode::REFLECT,
        ngraph::helpers::PadMode::SYMMETRIC
};

/* *======================* Static Shapes Tests 4D *======================* */

const std::vector<std::vector<int64_t>> padsBegin4DConstBlocked = {{0, 0, 0, 0}, {0, 0, 1, 3}, {2, 16, 1, 0}, {0, 0, 2, 0}};
const std::vector<std::vector<int64_t>> padsEnd4DConstBlocked   = {{0, 0, 0, 0}, {0, 0, 2, 1}, {2, 0, 0, 1}, {1, 32, 2, 0}};

const std::vector<std::vector<int64_t>> padsBegin4DBlocked = {{0, 0, 0, 0}, {0, 0, 1, 3}, {2, 0, 1, 0}, {0, 0, 2, 0}};
const std::vector<std::vector<int64_t>> padsEnd4DBlocked   = {{0, 0, 0, 0}, {0, 0, 2, 1}, {2, 0, 0, 1}, {1, 0, 2, 0}};

const std::vector<std::vector<int64_t>> padsBegin4D = {{0, 0, 0, 0}, {0, 1, 1, 1}, {0, 2, 1, 0}, {0, 0, 0, 1}};
const std::vector<std::vector<int64_t>> padsEnd4D   = {{0, 0, 0, 0}, {0, 2, 1, 1}, {0, 0, 2, 0}, {1, 1, 0, 0}};

const std::vector<CPUSpecificParams> CPUParams4DBlocked = {
        cpuParams_nChw16c,
        cpuParams_nChw8c,
};

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPad4DConstBlocked,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(static_shapes_to_test_representation({{3, 16, 5, 5}})),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin4DConstBlocked),
                ::testing::ValuesIn(padsEnd4DConstBlocked),
                ::testing::ValuesIn(argPadValue),
                ::testing::Values(ngraph::helpers::PadMode::CONSTANT),
                ::testing::ValuesIn(CPUParams4DBlocked)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPad4DConst,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(static_shapes_to_test_representation({{3, 16, 5, 5}})),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin4D),
                ::testing::ValuesIn(padsEnd4D),
                ::testing::ValuesIn(argPadValue),
                ::testing::Values(ngraph::helpers::PadMode::CONSTANT),
                ::testing::Values(cpuParams_nhwc)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPad4DBlocked,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(static_shapes_to_test_representation({{3, 16, 10, 5}})),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin4DBlocked),
                ::testing::ValuesIn(padsEnd4DBlocked),
                ::testing::Values(0),
                ::testing::ValuesIn(padMode),
                ::testing::ValuesIn(CPUParams4DBlocked)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPad4D,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(static_shapes_to_test_representation({{3, 16, 10, 5}})),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin4DBlocked),
                ::testing::ValuesIn(padsEnd4DBlocked),
                ::testing::Values(0),
                ::testing::ValuesIn(padMode),
                ::testing::Values(cpuParams_nhwc)),
        PadLayerCPUTest::getTestCaseName
);

/* *======================* *=====================* *======================* */

/* *======================* Dynamic Shapes Tests 4D *======================* */

const std::vector<InputShape> inputShapesDynamic4D = {
        {{-1, -1, -1, -1},                                    // dynamic
         {{5, 36, 5, 5}, {3, 16, 10, 5}, {3, 24, 10, 10}}},   // target

        {{-1, 32, -1, -1},                                    // dynamic
         {{5, 32, 5, 5}, {5, 32, 5, 8}, {3, 32, 8, 8}}},      // target

        {{{1, 5}, {16, 32}, {1, 16}, {1, 16}},                // dynamic
         {{3, 16, 5, 5}, {5, 24, 5, 8}, {3, 32, 8, 8}}},      // target
};

const std::vector<CPUSpecificParams> CPUParams4DDynamic = {
        cpuParams_nhwc,
        cpuParams_nchw
};

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPadDynamic4DConst,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(inputShapesDynamic4D),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin4D),
                ::testing::ValuesIn(padsEnd4D),
                ::testing::ValuesIn(argPadValue),
                ::testing::Values(ngraph::helpers::PadMode::CONSTANT),
                ::testing::ValuesIn(CPUParams4DDynamic)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPadDynamic4DConstBlocked,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::Values(inputShapesDynamic4D[1]),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin4DConstBlocked),
                ::testing::ValuesIn(padsEnd4DConstBlocked),
                ::testing::ValuesIn(argPadValue),
                ::testing::Values(ngraph::helpers::PadMode::CONSTANT),
                ::testing::ValuesIn(CPUParams4DBlocked)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPadDynamic4D,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(inputShapesDynamic4D),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin4D),
                ::testing::ValuesIn(padsEnd4D),
                ::testing::Values(0),
                ::testing::ValuesIn(padMode),
                ::testing::ValuesIn(CPUParams4DDynamic)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPadDynamic4DBlocked,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::Values(inputShapesDynamic4D[1]),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin4DBlocked),
                ::testing::ValuesIn(padsEnd4DBlocked),
                ::testing::Values(0),
                ::testing::ValuesIn(padMode),
                ::testing::ValuesIn(CPUParams4DBlocked)),
        PadLayerCPUTest::getTestCaseName
);

/* *======================* *=====================* *======================* */

/* *======================* Static Shapes Tests 5D *======================* */

const std::vector<std::vector<int64_t>> padsBegin5DConstBlocked = {{0, 0, 0, 0, 0}, {0, 0, 1, 1, 0}, {2, 32, 1, 1, 0}, {0, 0, 1, 3, 1}, {0, 0, 0, 1, 0}};
const std::vector<std::vector<int64_t>> padsEnd5DConstBlocked   = {{0, 0, 0, 0, 0}, {1, 16, 1, 1, 0}, {0, 0, 0, 1, 0}, {0, 0, 0, 1, 1}, {0, 0, 1, 0, 1}};

const std::vector<std::vector<int64_t>> padsBegin5DBlocked = {{0, 0, 0, 0, 0}, {0, 0, 1, 1, 0}, {2, 0, 1, 1, 0}, {0, 0, 1, 3, 1}, {0, 0, 0, 1, 0}};
const std::vector<std::vector<int64_t>> padsEnd5DBlocked   = {{0, 0, 0, 0, 0}, {1, 0, 1, 1, 0}, {0, 0, 0, 1, 0}, {0, 0, 0, 1, 1}, {0, 0, 1, 0, 1}};

const std::vector<std::vector<int64_t>> padsBegin5D = {{0, 0, 0, 0, 0}, {0, 0, 2, 0, 0}, {1, 1, 1, 1, 0}, {2, 0, 1, 0, 1}, {0, 2, 1, 3, 1}};
const std::vector<std::vector<int64_t>> padsEnd5D   = {{0, 0, 0, 0, 0}, {0, 0, 1, 0, 0}, {1, 0, 1, 1, 2}, {2, 2, 0, 1, 0}, {1, 1, 2, 0, 1}};

const std::vector<CPUSpecificParams> CPUParams5DBlocked = {
        cpuParams_nCdhw16c,
        cpuParams_nCdhw8c,
};

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPad5DConstBlocked,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(static_shapes_to_test_representation({{3, 16, 5, 5, 5}})),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin5DConstBlocked),
                ::testing::ValuesIn(padsEnd5DConstBlocked),
                ::testing::ValuesIn(argPadValue),
                ::testing::Values(ngraph::helpers::PadMode::CONSTANT),
                ::testing::ValuesIn(CPUParams5DBlocked)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPad5DConst,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(static_shapes_to_test_representation({{3, 16, 5, 5, 5}})),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin5D),
                ::testing::ValuesIn(padsEnd5D),
                ::testing::ValuesIn(argPadValue),
                ::testing::Values(ngraph::helpers::PadMode::CONSTANT),
                ::testing::Values(cpuParams_ndhwc)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPad5DBlocked,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(static_shapes_to_test_representation({{3, 16, 5, 5, 5}})),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin5DBlocked),
                ::testing::ValuesIn(padsEnd5DBlocked),
                ::testing::Values(0),
                ::testing::ValuesIn(padMode),
                ::testing::ValuesIn(CPUParams5DBlocked)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPad5D,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(static_shapes_to_test_representation({{3, 16, 5, 5, 5}})),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin5D),
                ::testing::ValuesIn(padsEnd5D),
                ::testing::Values(0),
                ::testing::ValuesIn(padMode),
                ::testing::Values(cpuParams_ndhwc)),
        PadLayerCPUTest::getTestCaseName
);

/* *======================* *=====================* *======================* */

/* *======================* Dynamic Shapes Tests 5D *======================* */

const std::vector<InputShape> inputShapesDynamic5D = {
        {{-1, -1, -1, -1, -1},                                            // dynamic
         {{5, 36, 5, 5, 5}, {3, 16, 8, 5, 7}, {3, 24, 10, 10, 10}}},      // target

        {{-1, 32, -1, -1, -1},                                            // dynamic
         {{5, 32, 5, 5, 5}, {3, 32, 8, 5, 7}, {3, 32, 10, 10, 10}}},      // target

        {{{1, 5}, {16, 32}, {1, 16}, {1, 16}, {1, 16}},                   // dynamic
         {{3, 16, 5, 5, 5}, {3, 24, 8, 5, 7}, {4, 32, 10, 10, 10}}},      // target
};

const std::vector<CPUSpecificParams> CPUParams5DDynamic = {
        cpuParams_ndhwc,
        cpuParams_ncdhw
};

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPadDynamic5DConst,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(inputShapesDynamic5D),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin5D),
                ::testing::ValuesIn(padsEnd5D),
                ::testing::ValuesIn(argPadValue),
                ::testing::Values(ngraph::helpers::PadMode::CONSTANT),
                ::testing::ValuesIn(CPUParams5DDynamic)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPadDynamic5DConstBlocked,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::Values(inputShapesDynamic5D[1]),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin5DConstBlocked),
                ::testing::ValuesIn(padsEnd5DConstBlocked),
                ::testing::ValuesIn(argPadValue),
                ::testing::Values(ngraph::helpers::PadMode::CONSTANT),
                ::testing::ValuesIn(CPUParams5DBlocked)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPadDynamic5D,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::ValuesIn(inputShapesDynamic5D),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin5D),
                ::testing::ValuesIn(padsEnd5D),
                ::testing::Values(0),
                ::testing::ValuesIn(padMode),
                ::testing::ValuesIn(CPUParams5DDynamic)),
        PadLayerCPUTest::getTestCaseName
);

INSTANTIATE_TEST_SUITE_P(
        smoke_CPUPadDynamic5DBlocked,
        PadLayerCPUTest,
        ::testing::Combine(
                ::testing::Values(inputShapesDynamic5D[1]),
                ::testing::ValuesIn(inputPrecisions),
                ::testing::ValuesIn(padsBegin5DBlocked),
                ::testing::ValuesIn(padsEnd5DBlocked),
                ::testing::Values(0),
                ::testing::ValuesIn(padMode),
                ::testing::ValuesIn(CPUParams5DBlocked)),
        PadLayerCPUTest::getTestCaseName
);

/* *======================* *=====================* *======================* */

} // namespace
} // namespace CPULayerTestsDefinitions

