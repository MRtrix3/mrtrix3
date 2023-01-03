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

#include "command.h"
#include "image.h"
#include "algo/threaded_loop.h"
#include "interp/linear.h"

using namespace MR;
using namespace App;


class TransformBase { MEMALIGN(TransformBase)
  public:
    virtual ~TransformBase(){}
    virtual Eigen::Vector3d transform_point (const Eigen::Vector3d& input) = 0;
};


class Warp : public TransformBase { MEMALIGN(Warp)
  public:
    Warp (Image<default_type>& in) : interp (in) {}

    Eigen::Vector3d transform_point (const Eigen::Vector3d &input) {
      Eigen::Vector3d output;
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

    Eigen::Vector3d transform_point (const Eigen::Vector3d &input) {
       Eigen::Vector3d output = transform * input;
       return output;
    }

    const transform_type transform;
};


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Compose any number of linear transformations and/or warps into a single transformation";

  DESCRIPTION
  + "Any linear transforms must be supplied as a 4x4 matrix in a text file (e.g. as per the output of mrregister). "
    "Any warp fields must be supplied as a 4D image representing a deformation field (e.g. as output from mrrregister -nl_warp)."

  + "Input transformations should be provided to the command in the order in which they would be applied "
    "to an image if they were to be applied individually."

  + "If all input transformations are linear, and the -template option is not provided, then the file output by "
    "the command will also be a linear transformation saved as a 4x4 matrix in a text file. If a template image is "
    "supplied, then the output will always be a deformation field. If at least one of the inputs is a warp field, "
    "then the output will be a deformation field, which will be defined on the grid of the last input warp image "
    "supplied if the -template option is not used.";

  ARGUMENTS
  + Argument ("input", "the input transforms (either linear or non-linear warps).").type_file_in().allow_multiple()
  + Argument ("output", "the output file (may be a linear transformation text file, or a deformation warp field image, depending on usage)").type_various();

  OPTIONS
  + Option ("template", "define the output grid defined by a template image")
    + Argument ("image").type_image_in();

}


using value_type = float;


void run ()
{
  vector<std::unique_ptr<TransformBase>> transform_list;
  std::unique_ptr<Header> template_header;

  for (size_t i = 0; i < argument.size() - 1; ++i) {
    try {
      template_header.reset (new Header (Header::open (argument[i])));
      auto image = Image<default_type>::open (argument[i]);

      if (image.ndim() != 4)
        throw Exception ("input warp is not a 4D image");

      if (image.size(3) != 3)
        throw Exception ("input warp should have 3 volumes in the 4th dimension");

      std::unique_ptr<TransformBase> transform (new Warp (image));
      transform_list.push_back (std::move (transform));

    } catch (Exception& E) {
      try {
        std::unique_ptr<TransformBase> transform (new Linear (load_transform (argument[i])));
        transform_list.push_back (std::move (transform));
      } catch (Exception& E) {
        throw Exception ("error reading input file: " + str(argument[i]) + ". Does not appear to be a 4D warp image or 4x4 linear transform.");
      }
    }

  }
  auto opt = get_options("template");

  if (opt.size()) {
    template_header.reset (new Header (Header::open (opt[0][0])));
  // no template is supplied and there are input warps, then make sure the last transform in the list is a warp
  } else if (template_header) {
    if (!dynamic_cast<Warp*> (transform_list[transform_list.size() - 1].get()))
      throw Exception ("Output deformation field grid not defined. When composing warps either use the -template "
                       "option to define the output deformation field grid, or ensure the last input transformation is a warp.");
  }

  // all inputs are linear so compose and output as text file
  if (!template_header) {

    transform_type composed = dynamic_cast<Linear*>(transform_list[transform_list.size() - 1].get())->transform;
    ssize_t index = transform_list.size() - 2;
    ProgressBar progress ("composing linear transformations", transform_list.size());
    progress++;
    while (index >= 0) {
      composed = dynamic_cast<Linear*>(transform_list[index].get())->transform * composed;
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
      Eigen::Vector3d voxel ((default_type) output.index(0),
                            (default_type) output.index(1),
                            (default_type) output.index(2));

      Eigen::Vector3d position = template_transform.voxel2scanner * voxel;
      ssize_t index = transform_list.size() - 1;
      while (index >= 0) {
        position = transform_list[index]->transform_point (position);
        index--;
      }
      output.row(3) = position;
    }
  }
}
