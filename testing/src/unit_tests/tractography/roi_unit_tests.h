/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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


#ifndef __testing_unittests_tractography_roiunittests_h__
#define __testing_unittests_tractography_roiunittests_h__

#include "dwi/tractography/roi.h"
#include "unit_tests/unit_test.h"
using namespace MR::DWI::Tractography;
using namespace Eigen;

using namespace MR::Testing::UnitTests;
namespace MR
{
  namespace Testing
  {
     namespace UnitTests
     {
        namespace Tractography
        {
           /**
           Runs unit tests for classes closely related with ROIs
           */
           class ROIUnitTests : MR::Testing::UnitTests::UnitTest
           {
              



           public:
              ROIUnitTests():
                 MR::Testing::UnitTests::UnitTest("ROIUnitTests")

              {}

              /**
              Runs unit tests for classes closely associated with ROIs
              */
              static bool run() {
                 try {
                    ROIUnitTests t = ROIUnitTests();
                    t.run_ROISet();

                    //Add additional ROI-based tests here

                    return true;
                 }
                 catch (...)
                 {
                    //A test failed
                    return false;
                 }
              }

           private:
              /**
              Unit testing for ROISet
              */
              void run_ROISet()
              {
                 std::cout << "IncludeROIVisitation...\n";
                 run_IncludeROIVisitation();
                 //Add additional tests here

                 std::cout << "passed\n";
              }

              /**
              Unit testing for external looping with contains(pos,state) --> state.contains_all()
              */
              void run_IncludeROIVisitation()
              {
                 //NO ROIS
                 {
                    ROISet_Initialise(0);
                    IncludeROIVisitation visitation (unordered, ordered);
                    std::array<Vector3f, 8> tck = {
                       Vector3f(0,0,0),
                       Vector3f(3,0,0),
                       Vector3f(0,5,0),
                       Vector3f(0,0,7),
                       Vector3f(11,0,7),
                       Vector3f(11,13,7),
                       Vector3f(11,13,0),
                       Vector3f(0,13,7),
                    };

                    //All entered should be true when there are no ROIs - there are none NOT entered.
                    check (visitation, "No ROIs - precheck");


                    for (size_t i = 0; i < tck.size(); i++)
                    {
                       visitation (tck[i]);
                       //All entered should still say true
                       check (visitation, "No ROI");
                    }
                 }

                 //UNORDERED ONLY

                 //---ONE ROI
                 {
                    ROISet_Initialise(1, 0);
                    IncludeROIVisitation visitation (unordered, ordered);
                    std::array<Vector3f, 7> tck = {
                       Vector3f(3,0,0),
                       Vector3f(0,5,0),
                       Vector3f(0,0,7),
                       Vector3f(11,0,7),
                       Vector3f(11,13,7),
                       Vector3f(11,13,0),
                       Vector3f(0,13,7)
                    };

                    //All entered should be false before anything is tested
                    check (!visitation, "One ROI - pretest");

                    for (size_t i = 0; i < tck.size(); i++)
                    {
                       visitation (tck[i]);
                       //All entered should still say false
                       check (!visitation, "One ROI: " + str(i));
                    }

                    //Test one inside
                    visitation (Vector3f(0.1, 0.2, 0.3));
                    check (visitation, "One ROI final A");

                    //Test another that is outside and ensure that the state still says true
                    visitation (Vector3f(11, 17, 310));
                    check (visitation, "One ROI final B");
                 }

                 //---THREE ROIS
                 {
                    ROISet_Initialise(3, 0);
                    IncludeROIVisitation visitation (unordered, ordered);
                    std::array<Vector3f, 11> tck = {
                       Vector3f(3,0,0),
                       Vector3f(0,5,0),
                       Vector3f(0,0,7),
                       Vector3f(11,0,7),
                       Vector3f(10,0,0),//inside roi[1]
                       Vector3f(11,13,7),
                       Vector3f(11,13,0),
                       Vector3f(0,10,0),//inside roi[2]
                       Vector3f(0,13,7),
                       Vector3f(0,0,0),//inside roi[0]
                       Vector3f(1000,100,70)
                    };

                    //All entered should be false before anything is tested
                    check (!visitation, "three ROIs - pretest");

                    for (size_t i = 0; i < tck.size(); i++)
                    {
                       visitation (tck[i]);
                       //we enter all of them on the [9]th
                       check (bool(visitation) == bool(i >= 9), "Three ROIs: " + str(i));
                    }

                 }



                 //ORDERED ONLY
                 //---ONE ROI
                 {
                    /*The rois are at
                    0,0,-100
                    */
                    ROISet_Initialise (0, 1);
                    IncludeROIVisitation visitation (unordered, ordered);
                    std::array<Vector3f, 1> tck = {
                       Vector3f(0,0,-100),//inside [0]
                    };

                    //All entered should be false before anything is tested
                    check (!visitation, "one ROI ordered - pretest");

                    for (size_t i = 0; i < tck.size(); i++)
                    {
                       visitation (tck[i]);
                       check (bool(visitation) == bool(i >= 0), "One ROI ordered: " + str(i));
                    }

                 }
                 //---THREE ROIS CORRECT ORDER SIMPLE
                 {
                    /*The rois are at
                    0,0,-100
                    10,0,-100
                    0,10,-100
                    */
                    ROISet_Initialise (0, 3);
                    IncludeROIVisitation visitation (unordered, ordered);
                    std::array<Vector3f, 4> tck = {
                       //outside

                       Vector3f(0,0,-100),//inside [0]
                       Vector3f(10,0,-100),//inside [1]
                       Vector3f(0,10,-100),//inside [2]
                       Vector3f(0,10,100),//outside
                    };

                    //All entered should be false before anything is tested
                    check (!visitation, "three ROIs ordered - pretest");

                    for (size_t i = 0; i < tck.size(); i++)
                    {
                       visitation (tck[i]);
                       //we enter all of them on the [12]th
                       check (bool(visitation) == bool(i >= 2), "Three ROIs ordered (simple): " + str(i));
                    }

                 }
                 //---THREE ROIS CORRECT ORDER
                 {
                    /*The rois are at
                    0,0,-100
                    10,0,-100
                    0,10,-100
                    */
                    ROISet_Initialise (0, 3);
                    IncludeROIVisitation visitation (unordered, ordered);
                    std::array<Vector3f, 15> tck = {
                       //outside
                       Vector3f(3,0,0),
                       Vector3f(0,5,0),
                       Vector3f(0,0,7),
                       Vector3f(0,0,-100),//enter [0]
                       Vector3f(11,0,7),//outside, [0] done
                       Vector3f(0,0,-100),//re-enter [0] (this is legal)
                       Vector3f(110,0,7),//outside, [0] done
                       Vector3f(0,0,-100),//re-enter [0] (this is legal)
                       Vector3f(110,0,7),//outside, [0] done
                       Vector3f(10,0,-100),//inside; [0],[1] done
                       Vector3f(-110,0,7),//outside, [0],[1] done
                       Vector3f(10,0,-100),//re-enter [1] (this is legal)
                       Vector3f(0,10,-100),//inside roi[2]; [0],[1],[2] done
                       Vector3f(11,13,7),//outside, [0],[1],[2] done
                       Vector3f(11,13,0),//outside, [0],[1],[2] done
                    };

                    //All entered should be false before anything is tested
                    check (!visitation, "three ROIs ordered - pretest");


                    for (size_t i = 0; i < tck.size(); i++)
                    {
                       visitation (tck[i]);
                       //we enter all of them on the [12]th
                       check (bool(visitation) == bool(i >= 12), "Three ROIs ordered: " + str(i));
                    }

                 }
                 //---THREE ROIS INCORRECT ORDER A->B->A
                 {
                    /*The rois are at
                    0,0,-100
                    10,0,-100
                    0,10,-100
                    */
                    ROISet_Initialise (0, 3);
                    IncludeROIVisitation visitation (unordered, ordered);
                    std::array<Vector3f, 16> tck = {
                       //outside
                       Vector3f(3,0,0),
                       Vector3f(0,5,0),
                       Vector3f(0,0,7),
                       Vector3f(0,0,-100),//enter first
                       Vector3f(11,0,7),//outside, [0] done
                       Vector3f(0,0,-100),//re-enter first (this is legal)
                       Vector3f(110,0,7),//outside, [0] done
                       Vector3f(0,0,-100),//re-enter first (this is legal)
                       Vector3f(110,0,7),//outside, [0] done
                       Vector3f(10,0,-100),//inside; [0],[1] done
                       Vector3f(0,0,-100),//re-enter first  <--------- this is NOT legal as we have entered region [1]
                       Vector3f(-110,0,7),//outside, [0],[1] done
                       Vector3f(10,0,-100),//re-enter second
                       Vector3f(0,10,-100),//inside roi[2]; [0],[1],[2] done
                       Vector3f(11,13,7),//outside, [0],[1],[2] done
                       Vector3f(11,13,0),//outside, [0],[1],[2] done
                    };

                    //All entered should be false before anything is tested
                    check (!visitation, "three ROIs ordered - pretest");


                    for (size_t i = 0; i < tck.size(); i++)
                    {
                       visitation (tck[i]);
                       check (!visitation, "Three ROIs ordered illegal ABA");
                    }

                 }
                 //---FOUR ROIS INCORRECT ORDER A->B->C->A->D
                 {
                    /*The rois are at
                    0,0,-100
                    10,0,-100
                    0,10,-100
                    10,10,-100
                    */
                    ROISet_Initialise (0, 4);
                    IncludeROIVisitation visitation (unordered, ordered);
                    std::array<Vector3f, 17> tck = {
                       //outside
                       Vector3f(3,0,0),
                       Vector3f(0,5,0),
                       Vector3f(0,0,7),
                       Vector3f(0,0,-100),//enter first
                       Vector3f(11,0,7),//outside, [0] done
                       Vector3f(0,0,-100),//re-enter first (this is legal)
                       Vector3f(110,0,7),//outside, [0] done
                       Vector3f(0,0,-100),//re-enter first (this is legal)
                       Vector3f(110,0,7),//outside, [0] done
                       Vector3f(10,0,-100),//inside; [0],[1] done
                       Vector3f(-110,0,7),//outside, [0],[1] done
                       Vector3f(10,0,-100),//re-enter second (this is legal)
                       Vector3f(0,10,-100),//inside roi[2]; [0],[1],[2] done
                       Vector3f(11,13,7),//outside, [0],[1],[2] done
                       Vector3f(11,13,0),//outside, [0],[1],[2] done
                       Vector3f(0,0,-100),//re-enter first  <--------- this is NOT legal as we have entered region [1] and [2]
                       Vector3f(10,10,-100),//inside roi[2]; [0],[1],[2],[3] done
                    };

                    //All entered should be false before anything is tested
                    check (!visitation, "three ROIs ordered - pretest");

                    for (size_t i = 0; i < tck.size(); i++)
                    {
                       visitation (tck[i]);
                       check (!visitation, "Three ROIs ordered - illegal ABCA");
                    }

                 }


                 //COMBINATION
                 //---FOUR ORDERED ROIS (A-D), and Two unordered ROIs (J,K)
                 //---A->B->J->C->D->K->D->J->K->B
                 {
                    //The rois are at
                    Vector3f A = Vector3f(0, 0, -100);
                    Vector3f B = Vector3f(10, 0, -100);
                    Vector3f C = Vector3f(0, 10, -100);
                    Vector3f D = Vector3f(10, 10, -100);
                    //J and K:
                    Vector3f J = Vector3f(0, 0, 0);
                    Vector3f K = Vector3f(10, 0, 0);

                    ROISet_Initialise(2, 4);
                    IncludeROIVisitation visitation (unordered, ordered);
                    std::array<Vector3f, 16> tck = {
                       //outside
                       Vector3f(3,0,0),
                       Vector3f(0,5,0),
                       Vector3f(0,0,7),
                       A,//enter A
                       Vector3f(11,0,7),//outside
                       B,//enter B; A->B
                       Vector3f(11,0,7),//outside
                       J,C,D,K,//all entered once we enter K
                       D,//Legal re-entry into D
                       J,K,//Legal re-entry into unordered rois
                       B,//<---- Illegal re-entry into B
                       Vector3f(110,0,7)//outside
                    };

                    //All entered should be false before anything is tested
                    check (!visitation, "three ROIs ordered - pretest");

                    for (size_t i = 0; i < tck.size(); i++)
                    {
                       visitation (tck[i]);
                       check (bool(visitation) == bool(i >= 10 && i < 14), "FOUR ORDERED ROIS (A-D), and Two unordered ROIs (J,K): " + str(i));
                    }
                 }

              }

              /**
              Sets up the ROI set ready for tests to be run
              */
              void ROISet_Initialise (size_t no_rois_unordered, size_t no_rois_ordered)
              {
                 unordered = ROIUnorderedSet();
                 ordered = ROIOrderedSet();
                 for (size_t i = 0; i < no_rois_unordered; i++)
                    unordered.add (ROISet_GetROI (i));
                 for (size_t i = 0; i < no_rois_ordered; i++)
                    ordered.add (ROISet_GetROI (i, -100));
              }
              /**
              Sets up the ROI set ready for tests to be run
              */
              void ROISet_Initialise (size_t no_rois)
              {
                 unordered = ROIUnorderedSet();
                 ordered = ROIOrderedSet();
                 for (size_t i = 0; i < no_rois; i++)
                 {
                    unordered.add (ROISet_GetROI (i));
                    ordered.add (ROISet_GetROI (i, -100));
                 }
              }

              /**
              Returns a spherical ROI for the ROISet_Initialise method or similar
              */
              ROI ROISet_GetROI(size_t i, float offset_z = 0)
              {
                 Vector3f position;
                 //Method could be made much nicer with a modulo operator but time is of the essence

                 if (i == 0)
                 {
                    position = Vector3f(0, 0, offset_z);

                 }
                 else if (i == 1)
                 {
                    position = Vector3f(10, 0, offset_z);
                 }
                 else if (i == 2)
                 {
                    position = Vector3f(0, 10, offset_z);
                 }
                 else if (i == 3)
                 {
                    position = Vector3f(10, 10, offset_z);
                 }
                 else if (i == 4)
                 {
                    position = Vector3f(0, 0, 10 + offset_z);
                 }
                 else if (i == 5)
                 {
                    position = Vector3f(10, 0, 10 + offset_z);
                 }
                 else if (i == 6)
                 {
                    position = Vector3f(0, 10, 10 + offset_z);
                 }
                 else if (i == 7)
                 {
                    position = Vector3f(10, 10, 10 + offset_z);
                 }
                 else
                 {
                    throw Exception("Not implemented");
                 }

                 return ROI(position, 1);
              }

              //Parameters:
              ROIUnorderedSet unordered;
              ROIOrderedSet ordered;



           };
        }
     }
  }
}

#endif
