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
#include "image.h"
#include "algo/threaded_loop.h"
#include "interp/linear.h"

using namespace MR;
using namespace App;


class TransformBase { MEMALIGN(TransformBase)
  public:
    virtual ~TransformBase(){}
    virtual Eigen::Vector3 transform_point (const Eigen::Vector3& input) = 0;
};


class Warp : public TransformBase { MEMALIGN(Warp)
  public:
    Warp (Image<default_type>& in) : interp (in) {}

    Eigen::Vector3 transform_point (const Eigen::Vector3 &input) {
      Eigen::Vector3 output;
      if (interp.scanner (input))
        output = interp.row(3);
      else
        output.fill (NaN);
      return output;
    }

  protected:
    Interp::Linear<Image<default_type> > interp;
};


class Linear : public TransformBase { MEMALIGN(Linear)
  public:
    Linear (const transform_type& transform) : transform (transform) {}

    Eigen::Vector3 transform_point (const Eigen::Vector3 &input) {
       Eigen::Vector3 output = transform * input;
       return output;
    }

    const transform_type transform;
};


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "composes any number of linear transformations and/or warps into a single transformation. "
    "The input linear transforms must be supplied in as a 4x4 matrix in a text file (as per the output of mrregister)."
    "The input warp fields must be supplied as a 4D image representing a deformation field (as output from mrrregister -nl_warp).";

  ARGUMENTS
  + Argument ("input", "the input transforms (either linear or non-linear warps). List transforms in the order you like them to be "
                       "applied to an image (as if you were applying them seperately with mrtransform).").type_file_in().allow_multiple()

  + Argument ("output", "the output file. If all input transformations are linear, then the output will also be a linear "
                        "transformation saved as a 4x4 matrix in a text file. If a template image is supplied, then the output will "
                        "always be a deformation field (see below). If all inputs are warps, or a mix of linear and warps, then the "
                        "output will be a deformation field defined on the grid of the last input warp supplied.").type_file_out ();

  OPTIONS
  + Option ("template", "define the output  grid defined by a template image")
  + Argument ("image").type_image_in();

}


typedef float value_type;


void run ()
{
  vector<TransformBase*> transform_list;
  std::shared_ptr<Header> template_header;

  for (size_t i = 0; i < argument.size() - 1; ++i) {
    TransformBase* transform (nullptr);
    try {
      template_header = std::make_shared<Header> (Header::open (argument[i]));
      auto image = Image<default_type>::open (argument[i]);

      if (image.ndim() != 4)
        throw Exception ("input warp is not a 4D image");

      if (image.size(3) != 3)
        throw Exception ("input warp should have 3 volumes in the 4th dimension");

      transform = new Warp (image);
      transform_list.push_back (transform);

    } catch (Exception& E) {
      try {
        transform = new Linear (load_transform (argument[i]));
        transform_list.push_back (transform);
      } catch (Exception& E) {
        throw Exception ("error reading input file: " + str(argument[i]) + ". Does not appear to be a 4D warp image or 4x4 linear transform.");
      }
    }

  }
  auto opt = get_options("template");

  if (opt.size()) {
    template_header = std::make_shared<Header> (Header::open (opt[0][0]));
  // no template is supplied and there are input warps, then make sure the last transform in the list is a warp
  } else if (template_header) {
    if (!dynamic_cast<Warp*> (transform_list[transform_list.size() - 1]))
      throw Exception ("Output deformation field grid not defined. When composing warps either use the -template "
                       "option to define the output deformation field grid, or ensure the last input transformation is a warp.");
  }

  // all inputs are linear so compose and output as text file
  if (!template_header) {

    transform_type composed = dynamic_cast<Linear*>(transform_list[transform_list.size() - 1])->transform;
    ssize_t index = transform_list.size() - 2;
    ProgressBar progress ("composing linear transformations", transform_list.size());
    progress++;
    while (index >= 0) {
      composed = dynamic_cast<Linear*>(transform_list[index])->transform * composed;
      index--;
      progress++;
    }
    save_transform (composed, argument[argument.size() - 1]);

  } else {
    Header output_header (*template_header);
    output_header.ndim() = 4;
    output_header.size(3) = 3;
    output_header.datatype() = DataType::Float32;

    Image<float> output = Image<value_type>::create (argument [argument.size() - 1], output_header);

    Transform template_transform (output);
    for (auto i = Loop ("composing transformations", output, 0, 3) (output); i ; ++i) {
      Eigen::Vector3 voxel ((default_type) output.index(0),
                            (default_type) output.index(1),
                            (default_type) output.index(2));

      Eigen::Vector3 position = template_transform.voxel2scanner * voxel;
      ssize_t index = transform_list.size() - 1;
      while (index >= 0) {
        position = transform_list[index]->transform_point (position);
        index--;
      }
      output.row(3) = position;
    }
  }
}
