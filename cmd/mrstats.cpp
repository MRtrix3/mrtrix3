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


#include <iomanip>
#include <vector>

#include "command.h"
#include "datatype.h"
#include "image.h"
#include "image_helpers.h"
#include "memory.h"
#include "algo/histogram.h"
#include "algo/loop.h"
#include "file/ofstream.h"
#include "stats.h"


using namespace MR;
using namespace App;


void usage ()
{
AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

DESCRIPTION
  + "compute images statistics.";

ARGUMENTS
  + Argument ("image",
  "the input image from which statistics will be computed.")
  .type_image_in ();

OPTIONS
  + Stats::Options

  + OptionGroup ("Additional options for mrstats")
  + Option ("allvolumes", "generate statistics across all image volumes, rather than one set of statistics per image volume")
  + Option ("ignorezero", "ignore zero-valued input voxels.");

}


typedef Stats::value_type value_type;
typedef Stats::complex_type complex_type;


class Volume_loop
{ NOMEMALIGN
  public:
    Volume_loop (Image<complex_type>& in) :
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
    Image<complex_type>& image;
    const bool is_4D;
    bool status;
};




void run_volume (Stats::Stats& stats, Image<complex_type>& data, Image<bool>& mask)
{
  if (mask.valid()) {
    for (auto l = Loop(0,3) (data, mask); l; ++l) {
      if (mask.value())
        stats (data.value());
    }
  } else {
    for (auto l = Loop(0,3) (data); l; ++l)
      stats (data.value());
  }
}




void run ()
{

  auto header = Header::open (argument[0]);
  if (header.ndim() > 4)
    throw Exception ("mrstats is not designed to handle images greater than 4D");
  const bool is_complex = header.datatype().is_complex();
  auto data = header.get_image<complex_type>();
  const bool ignorezero = get_options("ignorezero").size();

  auto opt = get_options ("mask");
  Image<bool> mask;
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    check_dimensions (mask, header, 0, 3);
  }

  vector<std::string> fields;
  opt = get_options ("output");
  for (size_t n = 0; n < opt.size(); ++n) 
    fields.push_back (opt[n][0]);

  if (App::log_level && fields.empty())
    Stats::print_header (is_complex);

  if (get_options ("allvolumes").size()) {

    Stats::Stats stats (is_complex, ignorezero);
    for (auto i = Volume_loop (data); i; ++i)
      run_volume (stats, data, mask);
    stats.print (data, fields);

  } else {

    for (auto i = Volume_loop (data); i; ++i) {
      Stats::Stats stats (is_complex, ignorezero);
      run_volume (stats, data, mask);
      stats.print (data, fields);
    }

  }
}
