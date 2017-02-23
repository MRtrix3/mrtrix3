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
#include "progressbar.h"

#include "image.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/scalar_file.h"

#include "math/SH.h"

#include "fixel/helpers.h"
#include "fixel/keys.h"
#include "fixel/loop.h"

using namespace MR;
using namespace App;


const char* operations[] = {
  "mean",
  "sum",
  "product",
  "min",
  "max",
  "absmax",
  "magmax",
  "count",
  "complexity",
  "sf",
  "dec_unit",
  "dec_scaled",
  "split_data",
  "split_dir",
  NULL
};


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au) & David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Convert a fixel-based sparse-data image into some form of scalar image";

  DESCRIPTION
  + "Fixel data can be reduced to voxel data in a number of ways:"
  + "- Some statistic computed across all fixel values within a voxel: mean, sum, product, min, max, absmax, magmax"
  + "- The number of fixels in each voxel: count"
  + "- Some measure of crossing-fibre organisation: complexity, sf ('single-fibre')"
  + "- A 4D directionally-encoded colour image: dec_unit, dec_scaled"
  + "- A 4D scalar image of fixel values with one 3D volume per fixel: split_data"
  + "- A 4D image of fixel directions, stored as three 3D volumes per fixel direction: split_dir";

  REFERENCES 
    + "* Reference for 'complexity' operation:\n"
    "Riffert, T. W.; Schreiber, J.; Anwander, A. & Knosche, T. R. "
    "Beyond Fractional Anisotropy: Extraction of bundle-specific structural metrics from crossing fibre models. "
    "NeuroImage, 2014, 100, 176-191";

  ARGUMENTS
  + Argument ("fixel_in", "the input fixel data file").type_image_in ()
  + Argument ("operation", "the operation to apply, one of: " + join(operations, ", ") + ".").type_choice (operations)
  + Argument ("image_out", "the output scalar image.").type_image_out ();

  OPTIONS
  + Option ("weighted", "weight the contribution of each fixel to the per-voxel result according to its volume. "
                        "E.g. when estimating a voxel-based measure of mean axon diameter, a fixel's mean axon diameter "
                        "should be weigthed by its relative volume within the voxel. Note that AFD can be used as a psuedomeasure of fixel volume.")
      + Argument ("fixel_in").type_image_in ();

}



class Mean
{ MEMALIGN (Mean)
  public:
    Mean (Image<float>& data, Image<float>& vol) :
        data (data), vol (vol) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      default_type sum = 0.0;
      default_type sum_volumes = 0.0;
      if (vol.valid()) {
        for (auto f = Fixel::Loop (index) (data, vol); f; ++f) {
          sum += data.value() * vol.value();
          sum_volumes += vol.value();
        }
        out.value() = sum_volumes ? (sum / sum_volumes) : 0.0;
      } else {
        index.index(3) = 0;
        size_t num_fixels = index.value();
        for (auto f = Fixel::Loop (index) (data); f; ++f)
          sum += data.value();
        out.value() = num_fixels ? (sum / default_type(num_fixels)) : 0.0;
      }
    }
  protected:
    Image<float> data, vol;
};


class Sum
{ MEMALIGN (Sum)
  public:
    Sum (Image<float>& data, Image<float>& vol) :
      data (data), vol (vol) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {

      if (vol.valid()) {
        for (auto f = Fixel::Loop (index) (data, vol); f; ++f)
          out.value() += data.value() * vol.value();
      } else {
        for (auto f = Fixel::Loop (index) (data); f; ++f)
          out.value() += data.value();
      }
    }
  protected:
    Image<float> data, vol;
};


class Product
{ MEMALIGN (Product)
  public:
    Product (Image<float>& data) :
      data (data) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      index.index(3) = 0;
      uint32_t num_fixels = index.value();
      if (!num_fixels) {
        out.value() = 0.0;
        return;
      }
      index.index(3) = 1;
      uint32_t offset = index.value();
      data.index(0) = offset;
      out.value() = data.value();
      for (size_t f = 1; f != num_fixels; ++f) {
        data.index(0)++;
        out.value() *= data.value();
      }
    }
  protected:
    Image<float> data;
};


class Min
{ MEMALIGN (Min)
  public:
    Min (Image<float>& data) :
      data (data) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      default_type min = std::numeric_limits<default_type>::infinity();
      for (auto f = Fixel::Loop (index) (data); f; ++f) {
        if (data.value() < min)
          min = data.value();
      }
      out.value() = std::isfinite (min) ? min : NAN;
    }
  protected:
    Image<float> data;
};


class Max
{ MEMALIGN (Max)
  public:
    Max (Image<float>& data) :
      data (data) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      default_type max = -std::numeric_limits<default_type>::infinity();
      for (auto f = Fixel::Loop (index) (data); f; ++f) {
        if (data.value() > max)
          max = data.value();
      }
      out.value() = std::isfinite (max) ? max : NAN;
    }
  protected:
    Image<float> data;
};


class AbsMax
{ MEMALIGN (AbsMax)
  public:
    AbsMax (Image<float>& data) :
      data (data) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      default_type absmax = -std::numeric_limits<default_type>::infinity();
      for (auto f = Fixel::Loop (index) (data); f; ++f) {
        if (std::abs (data.value()) > absmax)
          absmax = std::abs (data.value());
      }
      out.value() = std::isfinite (absmax) ? absmax : 0.0;
    }
  protected:
    Image<float> data;
};


class MagMax
{ MEMALIGN (MagMax)
  public:
    MagMax (Image<float>& data) :
      data (data) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      default_type magmax = 0.0;
      for (auto f = Fixel::Loop (index) (data); f; ++f) {
        if (std::abs (data.value()) > std::abs (magmax))
          magmax = data.value();
      }
      out.value() = std::isfinite (magmax) ? magmax : 0.0;
    }
  protected:
    Image<float> data;
};


class Complexity
{ MEMALIGN (Complexity)
  public:
    Complexity (Image<float>& data) :
      data (data) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      index.index(3) = 0;
      uint32_t num_fixels = index.value();
      if (num_fixels <= 1) {
        out.value() = 0.0;
        return;
      }
      default_type max = 0.0;
      default_type sum = 0.0;
      for (auto f = Fixel::Loop (index) (data); f; ++f) {
        max = std::max (max, default_type(data.value()));
        sum += data.value();
      }
      out.value() = (default_type(num_fixels) / default_type(num_fixels-1.0)) * (1.0 - (max / sum));
    }
  protected:
    Image<float> data;
};


class SF
{ MEMALIGN (SF)
  public:
    SF (Image<float>& data) : data (data) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      default_type max = 0.0;
      default_type sum = 0.0;
      for (auto f = Fixel::Loop (index) (data); f; ++f) {
        max = std::max (max, default_type(data.value()));
        sum += data.value();
      }
      out.value() = sum ? (max / sum) : 0.0;
    }
  protected:
    Image<float> data;
};


class DEC_unit
{ MEMALIGN (DEC_unit)
  public:
    DEC_unit (Image<float>& data, Image<float>& vol, Image<float>& dir) :
      data (data), vol (vol), dir (dir) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      Eigen::Vector3 sum_dec = {0.0, 0.0, 0.0};
      if (vol.valid()) {
        for (auto f = Fixel::Loop (index) (data, vol, dir); f; ++f)
          sum_dec += Eigen::Vector3 (std::abs (dir.row(1)[0]), std::abs (dir.row(1)[1]), std::abs (dir.row(1)[2])) * data.value() * vol.value();
      } else {
        for (auto f = Fixel::Loop (index) (data, dir); f; ++f)
          sum_dec += Eigen::Vector3 (std::abs (dir.row(1)[0]), std::abs (dir.row(1)[1]), std::abs (dir.row(1)[2])) * data.value();
      }
      if ((sum_dec.array() != 0.0).any())
        sum_dec.normalize();
      for (out.index(3) = 0; out.index(3) != 3; ++out.index(3))
        out.value() = sum_dec[size_t(out.index(3))];
    }
  protected:
    Image<float> data, vol, dir;
};


class DEC_scaled
{ MEMALIGN (DEC_scaled)
  public:
    DEC_scaled (Image<float>& data, Image<float>& vol, Image<float>& dir) :
      data (data), vol (vol), dir (dir) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      Eigen::Vector3 sum_dec = {0.0, 0.0, 0.0};
      default_type sum_value = 0.0;
      if (vol.valid()) {
        default_type sum_volume = 0.0;
        for (auto f = Fixel::Loop (index) (data, vol, dir); f; ++f) {
          sum_dec += Eigen::Vector3 (std::abs (dir.row(1)[0]), std::abs (dir.row(1)[1]), std::abs (dir.row(1)[2])) * data.value() * vol.value();
          sum_volume += vol.value();
          sum_value += vol.value() * data.value();
        }
        if ((sum_dec.array() != 0.0).any())
          sum_dec.normalize();
        sum_dec *= (sum_value / sum_volume);
      } else {
        for (auto f = Fixel::Loop (index) (data, dir); f; ++f) {
          sum_dec += Eigen::Vector3 (std::abs (dir.row(1)[0]), std::abs (dir.row(1)[1]), std::abs (dir.row(1)[2])) * data.value();
          sum_value += data.value();
        }
        if ((sum_dec.array() != 0.0).any())
          sum_dec.normalize();
        sum_dec *= sum_value;
      }
      for (out.index(3) = 0; out.index(3) != 3; ++out.index(3))
        out.value() = sum_dec[size_t(out.index(3))];
    }
  protected:
    Image<float> data, vol, dir;
};


class SplitData
{ MEMALIGN (SplitData)
  public:
    SplitData (Image<float>& data) : data (data) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      index.index(3) = 0;
      int num_fixels = (int)index.value();
      index.index(3) = 1;
      uint32_t offset = index.value();
      for (out.index(3) = 0; out.index(3) < out.size(3); ++out.index(3)) {
        if (out.index(3) < num_fixels) {
          data.index(0) = offset + out.index(3);
          out.value() = data.value();
        } else {
          out.value() = 0.0;
        }
      }
    }
  protected:
    Image<float> data;
};


class SplitDir
{ MEMALIGN (SplitDir)
  public:
    SplitDir (Image<float>& dir) : dir (dir) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      out.index(3) = 0;
      for (auto f = Fixel::Loop (index) (dir); f; ++f) {
        for (size_t axis = 0; axis < 3; ++axis) {
          dir.index(1) = axis;
          out.value() = dir.value();
          ++out.index(3);
        }
      }
      for (; out.index(3) < out.size(3); ++out.index(3))
        out.value() = 0.0;
    }
  protected:
    Image<float> dir;
};







void run ()
{
  auto in_data = Fixel::open_fixel_data_file<float> (argument[0]);
  if (in_data.size(2) != 1)
    throw Exception ("Input fixel data file must have a single scalar value per fixel (i.e. have dimensions Nx1x1)");

  Header in_index_header = Fixel::find_index_header (Fixel::get_fixel_directory (argument[0]));
  auto in_index_image = in_index_header.get_image<uint32_t>();

  Image<float> in_directions;

  const int op = argument[1];

  Header H_out (in_index_header);
  H_out.datatype() = DataType::Float32;
  H_out.datatype().set_byte_order_native();
  H_out.keyval().erase (Fixel::n_fixels_key);
  if (op == 7) { // count
    H_out.datatype() = DataType::UInt8;
  } else if (op == 10 || op == 11) { // dec
    H_out.ndim() = 4;
    H_out.size (3) = 3;
  } else if (op == 12 || op == 13) { // split_*
    H_out.ndim() = 4;
    uint32_t max_count = 0;
    for (auto l = Loop ("determining largest fixel count", in_index_image, 0, 3) (in_index_image); l; ++l)
      max_count = std::max (max_count, (uint32_t)in_index_image.value());
    if (max_count == 0)
      throw Exception ("fixel image is empty");

    // 3 volumes per fixel if performing split_dir
    H_out.size(3) = (op == 13) ? (3 * max_count) : max_count;
  }

  if (op == 10 || op == 11 || op == 13)  // dec or split_dir
    in_directions = Fixel::find_directions_header (
                    Fixel::get_fixel_directory (in_data.name())).get_image<float>().with_direct_io();

  Image<float> in_vol;
  auto opt = get_options ("weighted");
  if (opt.size())
    in_vol = Image<float>::open(opt[0][0]);

  if (op == 2 || op == 3 || op == 4 || op == 5 || op == 6 ||
      op == 7 || op == 8 || op == 9 || op == 12 || op == 13) {
    if (in_vol.valid())
      WARN ("Option -weighted has no meaningful interpretation for the operation specified; ignoring");
  }

  auto out = Image<float>::create (argument[2], H_out);

  auto loop = ThreadedLoop ("converting sparse fixel data to scalar image", in_index_image, 0, 3);

  switch (op) {
    case 0:  loop.run (Mean       (in_data, in_vol), in_index_image, out); break;
    case 1:  loop.run (Sum        (in_data, in_vol), in_index_image, out); break;
    case 2:  loop.run (Product    (in_data), in_index_image, out); break;
    case 3:  loop.run (Min        (in_data), in_index_image, out); break;
    case 4:  loop.run (Max        (in_data), in_index_image, out); break;
    case 5:  loop.run (AbsMax     (in_data), in_index_image, out); break;
    case 6:  loop.run (MagMax     (in_data), in_index_image, out); break;
    case 7:  loop.run ([](Image<uint32_t>& index, Image<float>& out) { // count
                          out.value() = index.value();
                       }, in_index_image, out); break;
    case 8:  loop.run (Complexity (in_data), in_index_image, out); break;
    case 9:  loop.run (SF         (in_data), in_index_image, out); break;
    case 10: loop.run (DEC_unit   (in_data, in_vol, in_directions), in_index_image, out); break;
    case 11: loop.run (DEC_scaled (in_data, in_vol, in_directions), in_index_image, out); break;
    case 12: loop.run (SplitData  (in_data), in_index_image, out); break;
    case 13: loop.run (SplitDir   (in_directions), in_index_image, out); break;
  }

}

