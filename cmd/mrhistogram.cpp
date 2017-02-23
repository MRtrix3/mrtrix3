/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "header.h"
#include "image.h"
#include "algo/histogram.h"

using namespace MR;
using namespace App;


void usage ()
{
AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

SYNOPSIS = "Generate a histogram of image intensities";

ARGUMENTS
  + Argument ("image", "the input image from which the histogram will be computed").type_image_in()
  + Argument ("hist", "the output histogram file").type_file_out();

OPTIONS
  + Algo::Histogram::Options

  + OptionGroup ("Additional options for mrhistogram")
  + Option ("allvolumes", "generate one histogram across all image volumes, rather than one per image volume");

}



class Volume_loop { MEMALIGN(Volume_loop)
  public:
    Volume_loop (Image<float>& in) :
        image (in),
        is_4D (in.ndim() == 4),
        status (true)
    {
      if (is_4D)
        image.index(3) = 0;
    }

    void operator++ ()
    {
      if (is_4D) {
        image.index(3)++;
      } else {
        assert (status);
        status = false;
      }
    }
    operator bool() const
    {
      if (is_4D)
        return (image.index(3) >= 0 && image.index(3) < image.size(3));
      else
        return status;
    }

  private:
    Image<float>& image;
    const bool is_4D;
    bool status;
};



template <class Functor>
void run_volume (Functor& functor, Image<float>& data, Image<bool>& mask)
{
  if (mask.valid()) {
    for (auto l = Loop(0,3) (data, mask); l; ++l) {
      if (mask.value())
        functor (float(data.value()));
    }
  } else {
    for (auto l = Loop(0,3) (data); l; ++l)
      functor (float(data.value()));
  }
}



void run ()
{

  auto header = Header::open (argument[0]);
  if (header.ndim() > 4)
    throw Exception ("mrhistogram is not designed to handle images greater than 4D");
  if (header.datatype().is_complex())
    throw Exception ("histogram generation not supported for complex data types");
  auto data = header.get_image<float>();

  const bool allvolumes = get_options ("allvolumes").size();
  size_t nbins = get_option_value ("bins", 0);

  auto opt = get_options ("mask");
  Image<bool> mask;
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    check_dimensions (mask, header, 0, 3);
  }

  File::OFStream output (argument[1]);

  Algo::Histogram::Calibrator calibrator (nbins, get_options ("ignorezero").size());
  opt = get_options ("template");
  if (opt.size()) {
    calibrator.from_file (opt[0][0]);
  } else {
    for (auto v = Volume_loop(data); v; ++v)
      run_volume (calibrator, data, mask);
    // If getting min/max using all volumes, but generating a single histogram per volume,
    //   then want the automatic calculation of bin width to be based on the number of
    //   voxels per volume, rather than the total number of values sent to the calibrator
    calibrator.finalize (header.ndim() > 3 && !allvolumes ? header.size(3) : 1,
                         header.datatype().is_integer() && header.intensity_offset() == 0.0 && header.intensity_scale() == 1.0);
  }
  nbins = calibrator.get_num_bins();
  if (nbins == 0) {
    std::string message;
    message.append(std::string("Zero bins selected") + (get_options ("ignorezero").size() or get_options ("bins").size()?
      "." :", you might want to use the -ignorezero or -bins option."));
    WARN(message);
  }

  for (size_t i = 0; i != nbins; ++i)
    output << (calibrator.get_min() + ((i+0.5) * calibrator.get_bin_width())) << ",";
  output << "\n";

  if (allvolumes) {

    Algo::Histogram::Data histogram (calibrator);
    for (auto v = Volume_loop(data); v; ++v)
      run_volume (histogram, data, mask);
    for (size_t i = 0; i != nbins; ++i)
      output << histogram[i] << ",";
    output << "\n";

  } else {

    for (auto v = Volume_loop(data); v; ++v) {
      Algo::Histogram::Data histogram (calibrator);
      run_volume (histogram, data, mask);
      for (size_t i = 0; i != nbins; ++i)
        output << histogram[i] << ",";
      output << "\n";
    }

  }
}

