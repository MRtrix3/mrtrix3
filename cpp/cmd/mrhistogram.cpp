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

#include "algo/histogram.h"
#include "command.h"
#include "header.h"
#include "image.h"

#include <filesystem>

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Generate a histogram of image intensities";

  ARGUMENTS
  + Argument ("image", "the input image from which the histogram will be computed").type_image_in()
  + Argument ("hist", "the output histogram file").type_file_out();

  OPTIONS
  + Algo::Histogram::Options

  + OptionGroup ("Additional options for mrhistogram")
  + Option ("allvolumes", "generate one histogram across all image volumes,"
                          " rather than one per image volume");

}
// clang-format on

class Volume_loop {
public:
  Volume_loop(Image<float> &in) : image(in), is_4D(in.ndim() == 4), status(true) {
    if (is_4D)
      image.index(3) = 0;
  }

  void operator++() {
    if (is_4D) {
      image.index(3)++;
    } else {
      assert(status);
      status = false;
    }
  }
  operator bool() const {
    if (is_4D)
      return (image.index(3) >= 0 && image.index(3) < image.size(3));
    else
      return status;
  }

private:
  Image<float> &image;
  const bool is_4D;
  bool status;
};

template <class Functor> void run_volume(Functor &functor, Image<float> &data, Image<bool> &mask) {
  if (mask.valid()) {
    for (auto l = Loop(0, 3)(data, mask); l; ++l) {
      if (mask.value())
        functor(float(data.value()));
    }
  } else {
    for (auto l = Loop(0, 3)(data); l; ++l)
      functor(float(data.value()));
  }
}

void run() {
  const std::filesystem::path input_path{argument[0]};
  const std::filesystem::path output_path{argument[1]};

  auto header = Header::open(input_path);
  if (header.ndim() > 4)
    throw Exception("mrhistogram is not designed to handle images greater than 4D");
  if (header.datatype().is_complex())
    throw Exception("histogram generation not supported for complex data types");
  auto data = header.get_image<float>();

  const bool allvolumes = !get_options("allvolumes").empty();
  const size_t nbins_user = get_option_value("bins", 0);
  const bool ignorezero = !get_options("ignorezero").empty();

  auto opt = get_options("mask");
  Image<bool> mask;
  if (!opt.empty()) {
    const std::filesystem::path mask_path{opt[0][0]};
    mask = Image<bool>::open(mask_path);
    check_dimensions(mask, header, 0, 3);
  }

  File::OFStream output(output_path);
  output << "# " << App::command_history_string << "\n";

  Algo::Histogram::Calibrator calibrator(nbins_user, ignorezero);
  opt = get_options("template");
  if (!opt.empty()) {
    const std::filesystem::path template_path{opt[0][0]};
    calibrator.from_file(template_path);
  } else {
    for (auto v = Volume_loop(data); v; ++v)
      run_volume(calibrator, data, mask);
    // If getting min/max using all volumes, but generating a single histogram per volume,
    //   then want the automatic calculation of bin width to be based on the number of
    //   voxels per volume, rather than the total number of values sent to the calibrator
    calibrator.finalize(header.ndim() > 3 && !allvolumes ? header.size(3) : 1,
                        header.datatype().is_integer() && header.intensity_offset() == 0.0 &&
                            header.intensity_scale() == 1.0);
  }
  const size_t nbins_data = calibrator.get_num_bins();
  if (nbins_data == 0)
    throw Exception(
        std::string("No histogram bins constructed") +
        ((ignorezero || nbins_user > 1) ? "." : "; you might want to use the -ignorezero or -bins option."));

  for (size_t i = 0; i != nbins_data; ++i)
    output << (calibrator.get_min() + ((i + 0.5) * calibrator.get_bin_width())) << ",";
  output << "\n";

  if (allvolumes) {

    Algo::Histogram::Data histogram(calibrator);
    for (auto v = Volume_loop(data); v; ++v)
      run_volume(histogram, data, mask);
    for (size_t i = 0; i != nbins_data; ++i)
      output << histogram[i] << ",";
    output << "\n";

  } else {

    for (auto v = Volume_loop(data); v; ++v) {
      Algo::Histogram::Data histogram(calibrator);
      run_volume(histogram, data, mask);
      for (size_t i = 0; i != nbins_data; ++i)
        output << histogram[i] << ",";
      output << "\n";
    }
  }
}
