#pragma once

#include <gtest/gtest.h>
#include "gpu.h"

class GPUTest : public ::testing::Test {
protected:
    MR::GPU::ComputeContext context;

    void SetUp() {
        ASSERT_NO_THROW(
            context = MR::GPU::ComputeContext();
        );
    }
};
