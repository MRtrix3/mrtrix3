/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "command.h"
#include "progressbar.h"
#include "algo/loop.h"

#include "image.h"

#include "sparse/fixel_metric.h"
#include "sparse/keys.h"
#include "sparse/image.h"

using namespace MR;
using namespace App;

using Sparse::FixelMetric;

const char* operation[] = { "add", "sub", "mult", "div", NULL };

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Perform basic calculations (add, subtract, multiply, divide) between two fixel images";

  ARGUMENTS
  + Argument ("input1", "the input fixel image.").type_image_in ()
  + Argument ("operation", "the type of operation to be applied (either add, sub, mult or divide)").type_choice (operation)
  + Argument ("input2", "the input fixel image.").type_image_in ()
  + Argument ("output", "the output fixel image.").type_image_out ();
}



float add (float a, float b) { return a + b; }
float subtract (float a, float b) { return a - b; }
float multiply (float a, float b) { return a * b; }
float divide (float a, float b) { return a / b; }


void run ()
{
  auto header = Header::open (argument[0]);
  Sparse::Image<FixelMetric> input1 (argument[0]);
  Sparse::Image<FixelMetric> input2 (argument[2]);

  check_dimensions (input1, input2);

  Sparse::Image<FixelMetric> output (argument[3], header);

  const size_t operation = argument[1];

  std::string message;
  float (*op)(float, float) = NULL;
  switch (operation) {
    case 0:
      message = "adding fixel images";
      op = &add;
      break;
    case 1:
      message = "subtracting fixel images";
      op = &subtract;
      break;
    case 2:
      message = "multiplying fixel images";
      op = &multiply;
      break;
    case 3:
      message = "dividing fixel images";
      op = &divide;
      break;
    default:
      break;
  }


  for (auto i = Loop (message, input1) (input1, input2, output); i; ++i) {
    if (input1.value().size() != input2.value().size())
      throw Exception ("the fixel images do not have corresponding fixels in all voxels");
    output.value().set_size (input1.value().size());
    for (size_t fixel = 0; fixel != input1.value().size(); ++fixel) {
      output.value()[fixel] = input1.value()[fixel];
      output.value()[fixel].value = (*op)(input1.value()[fixel].value, input2.value()[fixel].value);
    }
  }
}

