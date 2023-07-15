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


#ifndef __testing_unittests_unittest_h__
#define __testing_unittests_unittest_h__



using namespace std;
namespace MR
{
   namespace Testing
   {
      namespace UnitTests
      {

         class UnitTest
         {
            
         public:
            UnitTest(string unit_name_arg) {
               unit_name = unit_name_arg;
            }


         public:
            /**
            If the condition is FALSE, the message and class name are printed out and execution aborts
            */
            void check(bool pass, std::string message = "(no message provided)")
            {
               if (!pass)
               {
                  //We write info on the error here becasue C++ assert doesn't accept a string and other ways around this are horrible language hacks
                  message = "FAIL: " + unit_name + ":\t" + message + "\n";
                  WARN(message);
                  throw Exception(message);
               }

            }


            string unit_name;
         };


      }
   }
}


#endif
