/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "dwi/tractography/roi.h"
#include <gtest/gtest.h>

#include <Eigen/Core>

#include <array>
#include <cstddef>
#include <stdexcept>

using namespace MR::DWI::Tractography;
using namespace Eigen;

namespace {

class ROITests : public ::testing::Test {
protected:
  void ROISet_Initialise(size_t no_rois_unordered, size_t no_rois_ordered) {
    unordered = ROIUnorderedSet();
    ordered = ROIOrderedSet();
    for (size_t i = 0; i < no_rois_unordered; i++)
      unordered.add(ROISet_GetROI(i));
    for (size_t i = 0; i < no_rois_ordered; i++)
      ordered.add(ROISet_GetROI(i, -100));
  }

  void ROISet_Initialise(size_t no_rois) {
    unordered = ROIUnorderedSet();
    ordered = ROIOrderedSet();
    for (size_t i = 0; i < no_rois; i++) {
      unordered.add(ROISet_GetROI(i));
      ordered.add(ROISet_GetROI(i, -100));
    }
  }

  static ROI ROISet_GetROI(size_t i, float offset_z = 0) {
    Vector3f position;
    if (i == 0) {
      position = Vector3f(0, 0, offset_z);
    } else if (i == 1) {
      position = Vector3f(10, 0, offset_z);
    } else if (i == 2) {
      position = Vector3f(0, 10, offset_z);
    } else if (i == 3) {
      position = Vector3f(10, 10, offset_z);
    } else if (i == 4) {
      position = Vector3f(0, 0, 10 + offset_z);
    } else if (i == 5) {
      position = Vector3f(10, 0, 10 + offset_z);
    } else if (i == 6) {
      position = Vector3f(0, 10, 10 + offset_z);
    } else if (i == 7) {
      position = Vector3f(10, 10, 10 + offset_z);
    } else {
      throw std::logic_error("Not implemented for ROI index " + std::to_string(i));
    }
    return ROI(position, 1);
  }

  ROIUnorderedSet unordered;
  ROIOrderedSet ordered;
};

} // namespace

TEST_F(ROITests, NoROIs) {
  ROISet_Initialise(0);
  IncludeROIVisitation visitation(unordered, ordered);
  const std::array<Vector3f, 8> tck = {
      Vector3f(0, 0, 0),
      Vector3f(3, 0, 0),
      Vector3f(0, 5, 0),
      Vector3f(0, 0, 7),
      Vector3f(11, 0, 7),
      Vector3f(11, 13, 7),
      Vector3f(11, 13, 0),
      Vector3f(0, 13, 7),
  };
  ASSERT_TRUE(visitation) << "No ROIs - precheck";
  for (size_t i = 0; i < tck.size(); i++) {
    visitation(tck[i]);
    EXPECT_TRUE(visitation) << "No ROI after visiting point " << i;
  }
}

TEST_F(ROITests, OneUnorderedROI) {
  ROISet_Initialise(1, 0);
  IncludeROIVisitation visitation(unordered, ordered);
  std::array<Vector3f, 7> tck = {Vector3f(3, 0, 0),
                                 Vector3f(0, 5, 0),
                                 Vector3f(0, 0, 7),
                                 Vector3f(11, 0, 7),
                                 Vector3f(11, 13, 7),
                                 Vector3f(11, 13, 0),
                                 Vector3f(0, 13, 7)};

  ASSERT_FALSE(visitation) << "One ROI - pretest";
  for (size_t i = 0; i < tck.size(); i++) {
    visitation(tck[i]);
    // All entered should still say false
    EXPECT_FALSE(visitation) << "One ROI, point " << i;
  }

  // Test one inside
  visitation(Vector3f(0.1, 0.2, 0.3));
  EXPECT_TRUE(visitation) << "One ROI final A";

  // Test another that is outside and ensure that the state still says true
  visitation(Vector3f(11, 17, 310));
  EXPECT_TRUE(visitation) << "One ROI final B";
}

TEST_F(ROITests, ThreeUnorderedROIs) {
  ROISet_Initialise(3, 0);
  IncludeROIVisitation visitation(unordered, ordered);
  const std::array<Vector3f, 11> tck = {Vector3f(3, 0, 0),
                                        Vector3f(0, 5, 0),
                                        Vector3f(0, 0, 7),
                                        Vector3f(11, 0, 7),
                                        Vector3f(10, 0, 0), // inside roi[1]
                                        Vector3f(11, 13, 7),
                                        Vector3f(11, 13, 0),
                                        Vector3f(0, 10, 0), // inside roi[2]
                                        Vector3f(0, 13, 7),
                                        Vector3f(0, 0, 0), // inside roi[0]
                                        Vector3f(1000, 100, 70)};

  // All entered should be false before anything is tested
  ASSERT_FALSE(visitation) << "three ROIs - pretest";
  for (size_t i = 0; i < tck.size(); i++) {
    visitation(tck[i]);
    // we enter all of them on the [9]th
    EXPECT_EQ(bool(visitation), (i >= 9)) << "Three ROIs: " << i;
  }
}

TEST_F(ROITests, OneOrderedROI) {
  ROISet_Initialise(0, 1);
  IncludeROIVisitation visitation(unordered, ordered);
  std::array<Vector3f, 1> tck = {
      Vector3f(0, 0, -100), // inside [0]
  };

  ASSERT_FALSE(visitation) << "one ROI ordered - pretest";
  for (size_t i = 0; i < tck.size(); i++) {
    visitation(tck[i]);
    EXPECT_TRUE(bool(visitation)) << "One ROI ordered: " << i;
  }
}

TEST_F(ROITests, ThreeOrderedROIsSimple) {
  /*The rois are at
  0,0,-100
  10,0,-100
  0,10,-100
  */
  ROISet_Initialise(0, 3);
  IncludeROIVisitation visitation(unordered, ordered);
  std::array<Vector3f, 4> tck = {
      Vector3f(0, 0, -100),  // inside [0]
      Vector3f(10, 0, -100), // inside [1]
      Vector3f(0, 10, -100), // inside [2]
      Vector3f(0, 10, 100),  // outside
  };

  ASSERT_FALSE(visitation) << "three ROIs ordered - pretest";
  for (size_t i = 0; i < tck.size(); i++) {
    visitation(tck[i]);
    // we enter all of them on the [12]th
    EXPECT_EQ(bool(visitation), (i >= 2)) << "Three ROIs ordered (simple): " << i;
  }
}

TEST_F(ROITests, ThreeOrderedROIsCorrectOrder) {
  /*The rois are at
    0,0,-100
    10,0,-100
    0,10,-100
  */
  ROISet_Initialise(0, 3);
  IncludeROIVisitation visitation(unordered, ordered);

  std::array<Vector3f, 15> tck = {
      Vector3f(3, 0, 0),
      Vector3f(0, 5, 0),
      Vector3f(0, 0, 7),
      Vector3f(0, 0, -100),  // enter [0]
      Vector3f(11, 0, 7),    // outside, [0] done
      Vector3f(0, 0, -100),  // re-enter [0] (legal)
      Vector3f(110, 0, 7),   // outside, [0] done
      Vector3f(0, 0, -100),  // re-enter [0] (legal)
      Vector3f(110, 0, 7),   // outside, [0] done
      Vector3f(10, 0, -100), // inside; [0],[1] done
      Vector3f(-110, 0, 7),  // outside, [0],[1] done
      Vector3f(10, 0, -100), // re-enter [1] (legal)
      Vector3f(0, 10, -100), // inside roi[2]; [0],[1],[2] done
      Vector3f(11, 13, 7),   // outside, [0],[1],[2] done
      Vector3f(11, 13, 0)    // outside, [0],[1],[2] done
  };

  ASSERT_FALSE(visitation) << "three ROIs ordered - pretest";
  for (size_t i = 0; i < tck.size(); ++i) {
    visitation(tck[i]);
    EXPECT_EQ(bool(visitation), (i >= 12)) << "Three ROIs ordered: " << i;
  }
}

TEST_F(ROITests, ThreeOrderedROIsIllegalABA) {
  /*The rois are at
    0,0,-100
    10,0,-100
    0,10,-100
  */
  ROISet_Initialise(0, 3);
  IncludeROIVisitation visitation(unordered, ordered);

  std::array<Vector3f, 16> tck = {
      Vector3f(3, 0, 0),
      Vector3f(0, 5, 0),
      Vector3f(0, 0, 7),
      Vector3f(0, 0, -100),  // enter first
      Vector3f(11, 0, 7),    // outside, [0] done
      Vector3f(0, 0, -100),  // re-enter first (legal)
      Vector3f(110, 0, 7),   // outside, [0] done
      Vector3f(0, 0, -100),  // re-enter first (legal)
      Vector3f(110, 0, 7),   // outside, [0] done
      Vector3f(10, 0, -100), // inside; [0],[1] done
      Vector3f(0, 0, -100),  // re-enter first  <-- illegal after [1]
      Vector3f(-110, 0, 7),  // outside, [0],[1] done
      Vector3f(10, 0, -100), // re-enter second
      Vector3f(0, 10, -100), // inside roi[2]; [0],[1],[2] done
      Vector3f(11, 13, 7),   // outside, [0],[1],[2] done
      Vector3f(11, 13, 0)    // outside, [0],[1],[2] done
  };

  ASSERT_FALSE(visitation) << "three ROIs ordered - pretest";
  for (size_t i = 0; i < tck.size(); ++i) {
    visitation(tck[i]);
    EXPECT_FALSE(visitation) << "Three ROIs ordered illegal ABA at step " << i;
  }
}

TEST_F(ROITests, FourOrderedROIsIllegalABCAD) {
  /*The rois are at
    0,0,-100
    10,0,-100
    0,10,-100
    10,10,-100
  */
  ROISet_Initialise(0, 4);
  IncludeROIVisitation visitation(unordered, ordered);

  std::array<Vector3f, 17> tck = {
      Vector3f(3, 0, 0),
      Vector3f(0, 5, 0),
      Vector3f(0, 0, 7),
      Vector3f(0, 0, -100),  // enter first
      Vector3f(11, 0, 7),    // outside, [0] done
      Vector3f(0, 0, -100),  // re-enter first (legal)
      Vector3f(110, 0, 7),   // outside, [0] done
      Vector3f(0, 0, -100),  // re-enter first (legal)
      Vector3f(110, 0, 7),   // outside, [0] done
      Vector3f(10, 0, -100), // inside; [0],[1] done
      Vector3f(-110, 0, 7),  // outside, [0],[1] done
      Vector3f(10, 0, -100), // re-enter second (legal)
      Vector3f(0, 10, -100), // inside roi[2]; [0],[1],[2] done
      Vector3f(11, 13, 7),   // outside, [0],[1],[2] done
      Vector3f(11, 13, 0),   // outside, [0],[1],[2] done
      Vector3f(0, 0, -100),  // re-enter first  <-- illegal after [1],[2]
      Vector3f(10, 10, -100) // inside roi[3]; would complete all
  };

  ASSERT_FALSE(visitation) << "four ROIs ordered - pretest";
  for (size_t i = 0; i < tck.size(); ++i) {
    visitation(tck[i]);
    EXPECT_FALSE(visitation) << "Four ROIs ordered illegal ABCA at step " << i;
  }
}

TEST_F(ROITests, CombinationOrderedAndUnorderedROIs) {
  // FOUR ORDERED ROIS (A-D), and Two unordered ROIs (J,K)
  // A->B->J->C->D->K->D->J->K->B
  const Vector3f A = Vector3f(0, 0, -100);
  const Vector3f B = Vector3f(10, 0, -100);
  const Vector3f C = Vector3f(0, 10, -100);
  const Vector3f D = Vector3f(10, 10, -100);
  const Vector3f J = Vector3f(0, 0, 0);
  const Vector3f K = Vector3f(10, 0, 0);

  ROISet_Initialise(2, 4);
  IncludeROIVisitation visitation(unordered, ordered);

  std::array<Vector3f, 16> tck = {
      Vector3f(3, 0, 0),
      Vector3f(0, 5, 0),
      Vector3f(0, 0, 7),
      A,                  // enter A
      Vector3f(11, 0, 7), // outside
      B,                  // enter B; A->B
      Vector3f(11, 0, 7), // outside
      J,                  // enter unordered J
      C,                  // next ordered
      D,                  // next ordered
      K,                  // enter unordered K -> all done here
      D,                  // Legal re-entry into D
      J,                  // Legal re-entry into unordered J
      K,                  // Legal re-entry into unordered K
      B,                  // <---- Illegal re-entry into ordered B
      Vector3f(110, 0, 7) // outside
  };

  ASSERT_FALSE(visitation) << "combination ROIs - pretest";
  for (size_t i = 0; i < tck.size(); ++i) {
    visitation(tck[i]);
    const bool expect = (i >= 10 && i < 14);
    EXPECT_EQ(bool(visitation), expect) << "Combination ROIs at step " << i;
  }
}
