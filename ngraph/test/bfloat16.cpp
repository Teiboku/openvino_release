// Copyright (C) 2018-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "ngraph/type/bfloat16.hpp"

#include <climits>
#include <random>

#include "gtest/gtest.h"
#include "ngraph/log.hpp"
#include "ngraph/runtime/aligned_buffer.hpp"
#include "util/float_util.hpp"

using namespace std;
using namespace ngraph;

template <typename T>
std::string to_hex(T value) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(sizeof(T) * 2) << std::setfill('0') << value;
    return ss.str();
}

//***********************
// NOTE
//***********************
// This test uses exact comparisons of floating point values. It is testing for bit-exact
// creation and truncation/rounding of bfloat16 values.
TEST(bfloat16, conversions) {
    bfloat16 bf;
    const char* source_string;
    std::string bf_string;

    // 1.f, the ground-truth value
    source_string = "0  01111111  000 0000";
    bf = test::bits_to_bfloat16(source_string);
    EXPECT_EQ(bf, bfloat16(1.0));
    bf_string = test::bfloat16_to_bits(bf);
    EXPECT_STREQ(source_string, bf_string.c_str());

    // 1.03125f, the exact upper bound
    source_string = "0  01111111  000 0100";
    bf = test::bits_to_bfloat16(source_string);
    EXPECT_EQ(bf, bfloat16(1.03125));
    bf_string = test::bfloat16_to_bits(bf);
    EXPECT_STREQ(source_string, bf_string.c_str());
}

TEST(bfloat16, round_to_nearest) {
    const char* fstring;
    std::string expected;
    float fvalue;
    uint16_t bf_round;

    fstring = "0  01111111  000 0100 1000 0000 0000 0000";
    fvalue = test::bits_to_float(fstring);
    bf_round = bfloat16::round_to_nearest(fvalue);
    EXPECT_EQ(bf_round, 0x3F85);

    fstring = "0  01111111  000 0100 0000 0000 0000 0000";
    fvalue = test::bits_to_float(fstring);
    bf_round = bfloat16::round_to_nearest(fvalue);
    EXPECT_EQ(bf_round, 0x3F84);

    fstring = "0  01111111  111 1111 1000 0000 0000 0000";
    fvalue = test::bits_to_float(fstring);
    bf_round = bfloat16::round_to_nearest(fvalue);
    EXPECT_EQ(bf_round, 0x4000);

    // 1.9921875f, the next representable number which should not round up
    fstring = "0  01111111  111 1111 0000 0000 0000 0000";
    fvalue = test::bits_to_float(fstring);
    bf_round = bfloat16::round_to_nearest(fvalue);
    EXPECT_EQ(bf_round, 0x3FFF);
}

TEST(bfloat16, round_to_nearest_even) {
    const char* fstring;
    float fvalue;
    uint16_t bf_round;

    fstring = "0  01111111  000 0100 1000 0000 0000 0000";
    fvalue = test::bits_to_float(fstring);
    bf_round = bfloat16::round_to_nearest_even(fvalue);
    EXPECT_EQ(bf_round, 0x3F84);

    fstring = "0  01111111  000 0101 1000 0000 0000 0000";
    fvalue = test::bits_to_float(fstring);
    bf_round = bfloat16::round_to_nearest_even(fvalue);
    EXPECT_EQ(bf_round, 0x3F86);

    fstring = "0  01111111  000 0101 0000 0000 0000 0000";
    fvalue = test::bits_to_float(fstring);
    bf_round = bfloat16::round_to_nearest_even(fvalue);
    EXPECT_EQ(bf_round, 0x3F85);

    fstring = "0  01111111  111 1111 1000 0000 0000 0000";
    fvalue = test::bits_to_float(fstring);
    bf_round = bfloat16::round_to_nearest_even(fvalue);
    EXPECT_EQ(bf_round, 0x4000);

    fstring = "0  01111111  111 1111 0000 0000 0000 0000";
    fvalue = test::bits_to_float(fstring);
    bf_round = bfloat16::round_to_nearest_even(fvalue);
    EXPECT_EQ(bf_round, 0x3FFF);
}

TEST(bfloat16, to_float) {
    bfloat16 bf;
    const char* source_string;

    // 1.f, the ground-truth value
    source_string = "0  01111111  000 0000";
    bf = test::bits_to_bfloat16(source_string);
    float f = static_cast<float>(bf);
    EXPECT_EQ(f, 1.0f);

    // 1.03125f, the exact upper bound
    source_string = "0  01111111  000 0100";
    bf = test::bits_to_bfloat16(source_string);
    f = static_cast<float>(bf);
    EXPECT_EQ(f, 1.03125f);
}

TEST(bfloat16, numeric_limits) {
    bfloat16 infinity = numeric_limits<bfloat16>::infinity();
    bfloat16 neg_infinity = -numeric_limits<bfloat16>::infinity();
    bfloat16 quiet_nan = numeric_limits<bfloat16>::quiet_NaN();
    bfloat16 signaling_nan = numeric_limits<bfloat16>::signaling_NaN();

    // Would be nice if we could have bfloat16 overloads for these, but it would require adding
    // overloads into ::std. So we just cast to float here. We can't rely on an implicit cast
    // because it fails with some versions of AppleClang.
    EXPECT_TRUE(isinf(static_cast<float>(infinity)));
    EXPECT_TRUE(isinf(static_cast<float>(neg_infinity)));
    EXPECT_TRUE(isnan(static_cast<float>(quiet_nan)));
    EXPECT_TRUE(isnan(static_cast<float>(signaling_nan)));
}

TEST(benchmark, bfloat16) {
    NGRAPH_SUPPRESS_DEPRECATED_START
    size_t buffer_size = 128 * 3 * 224 * 224;
    ngraph::runtime::AlignedBuffer data(buffer_size * sizeof(float), 4096);
    float* f = static_cast<float*>(data.get_ptr());
    // vector<float> data(buffer_size);
    std::mt19937 rng(2112);
    std::uniform_real_distribution<float> distribution(-300, 300);
    for (size_t i = 0; i < buffer_size; ++i) {
        f[i] = distribution(rng);
    }
    NGRAPH_INFO << "buffer size " << buffer_size << " floats or " << data.size() << " bytes";

    {
        ngraph::runtime::AlignedBuffer bf_data(buffer_size * sizeof(bfloat16), 4096);
        bfloat16* p = static_cast<bfloat16*>(bf_data.get_ptr());
        stopwatch timer;
        timer.start();
        for (size_t i = 0; i < buffer_size; ++i) {
            p[i] = bfloat16(f[i]);
        }
        timer.stop();
        NGRAPH_INFO << "float to bfloat16 ctor                  " << timer.get_milliseconds() << "ms";
    }

    {
        ngraph::runtime::AlignedBuffer bf_data(buffer_size * sizeof(bfloat16), 4096);
        bfloat16* p = static_cast<bfloat16*>(bf_data.get_ptr());
        stopwatch timer;
        timer.start();
        for (size_t i = 0; i < buffer_size; ++i) {
            p[i] = bfloat16::truncate(f[i]);
        }
        timer.stop();
        NGRAPH_INFO << "float to bfloat16 truncate              " << timer.get_milliseconds() << "ms";
    }

    {
        ngraph::runtime::AlignedBuffer bf_data(buffer_size * sizeof(bfloat16), 4096);
        bfloat16* p = static_cast<bfloat16*>(bf_data.get_ptr());
        stopwatch timer;
        timer.start();
        for (size_t i = 0; i < buffer_size; ++i) {
            p[i] = bfloat16::round_to_nearest(f[i]);
        }
        timer.stop();
        NGRAPH_INFO << "float to bfloat16 round to nearest      " << timer.get_milliseconds() << "ms";
    }

    {
        ngraph::runtime::AlignedBuffer bf_data(buffer_size * sizeof(bfloat16), 4096);
        bfloat16* p = static_cast<bfloat16*>(bf_data.get_ptr());
        stopwatch timer;
        timer.start();
        for (size_t i = 0; i < buffer_size; ++i) {
            p[i] = bfloat16::round_to_nearest_even(f[i]);
        }
        timer.stop();
        NGRAPH_INFO << "float to bfloat16 round to nearest even " << timer.get_milliseconds() << "ms";
    }
    NGRAPH_SUPPRESS_DEPRECATED_END
}

TEST(bfloat16, assigns) {
    bfloat16 bf16;
    bf16 = 2.0;
    EXPECT_EQ(bf16, float16(2.0));

    std::vector<float> f32vec{1.0, 2.0, 4.0};
    std::vector<bfloat16> bf16vec;
    std::copy(f32vec.begin(), f32vec.end(), std::back_inserter(bf16vec));
    for (size_t i = 0; i < f32vec.size(); ++i) {
        EXPECT_EQ(f32vec.at(i), bf16vec.at(i));
    }

    f32vec = {-1.0, -2.0, -3.0};
    for (size_t i = 0; i < f32vec.size(); ++i) {
        bf16vec[i] = f32vec[i];
    }
    for (size_t i = 0; i < f32vec.size(); ++i) {
        EXPECT_EQ(f32vec.at(i), bf16vec.at(i));
    }

    float f32arr[] = {1.0, 2.0, 4.0};
    bfloat16 bf16arr[sizeof(f32arr)];
    for (size_t i = 0; i < sizeof(f32arr) / sizeof(f32arr[0]); ++i) {
        bf16arr[i] = f32arr[i];
        EXPECT_EQ(f32arr[i], bf16arr[i]);
    }
}

TEST(bfloat16, operators) {
    bfloat16 a(2.0);
    bfloat16 b(3.5);
    bfloat16 c(5.5);
    bfloat16 d(7.0);
    ASSERT_TRUE(a + b == c);
    ASSERT_TRUE(a == c - b);
    ASSERT_TRUE(a * b == d);
    ASSERT_TRUE(a == d / b);
}
