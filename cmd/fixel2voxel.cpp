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


#include "apply.h"
#include "command.h"
#include "progressbar.h"

#include "image.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "formats/mrtrix_utils.h"

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
  + Option ("number", "use only the largest N fixels in calculation of the voxel-wise statistic; "
                      "in the case of \"split_data\" and \"split_dir\", output only the largest N "
                      "fixels, padding where necessary.")
      + Argument ("N").type_integer(1)

  + Option ("weighted", "weight the contribution of each fixel to the per-voxel result according to its volume. "
                        "E.g. when estimating a voxel-based measure of mean axon diameter, a fixel's mean axon diameter "
                        "should be weigthed by its relative volume within the voxel. Note that AFD can be used as a psuedomeasure of fixel volume.")
      + Argument ("fixel_in").type_image_in ();

}



using FixelIndexType = Image<uint32_t>;
using FixelDataType = Image<float>;



struct set_offset { NOMEMALIGN
  FORCE_INLINE set_offset (uint32_t offset) : offset (offset) { }
  template <class DataType>
    FORCE_INLINE void operator() (DataType& data) { data.index(0) = offset; }
  uint32_t offset;
};

struct inc_fixel { NOMEMALIGN
  template <class DataType>
    FORCE_INLINE void operator() (DataType& data) { ++data.index(0); }
};



struct LoopFixelsInVoxelWithMax { NOMEMALIGN
  const size_t num_fixels;
  const size_t max_fixels;
  const uint32_t offset;

  template <class... DataType>
  struct Run { NOMEMALIGN
    const size_t num_fixels;
    const size_t max_fixels;
    const uint32_t offset;
    uint32_t fixel_index;
    const std::tuple<DataType&...> data;
    FORCE_INLINE Run (const size_t num_fixels, const size_t max_fixels, const uint32_t offset, const std::tuple<DataType&...>& data) :
      num_fixels (num_fixels), max_fixels (max_fixels), offset (offset), fixel_index (0), data (data) {
      apply (set_offset (offset), data);
    }
    FORCE_INLINE operator bool() const { return max_fixels ? (fixel_index < max_fixels) : (fixel_index < num_fixels); }
    FORCE_INLINE void operator++() { if (!padding()) apply (inc_fixel (), data); ++fixel_index; }
    FORCE_INLINE void operator++(int) { operator++(); }
    FORCE_INLINE bool padding() const { return (max_fixels && fixel_index >= num_fixels); }
    FORCE_INLINE size_t count() const { return max_fixels ? max_fixels : num_fixels; }
  };

  template <class... DataType>
    FORCE_INLINE Run<DataType...> operator() (DataType&... data) const { return { num_fixels, max_fixels, offset, std::tie (data...) }; }

};



class Base
{ NOMEMALIGN
  public:
    Base (FixelDataType& data, const size_t max_fixels, const bool pad = false, const float pad_value = 0.0) :
        data (data),
        max_fixels (max_fixels),
        pad (pad),
        pad_value (pad_value) { }

  protected:
      FORCE_INLINE LoopFixelsInVoxelWithMax
      Loop (FixelIndexType& index) {
        index.index(3) = 0;
        const size_t num_fixels = index.value();
        index.index(3) = 1;
        const uint32_t offset = index.value();
        return { num_fixels, max_fixels, offset };
      }

    FixelDataType data;
    const size_t max_fixels;
    const bool pad;
    const float pad_value;

};


class Mean : protected Base
{ MEMALIGN (Mean)
  public:
    Mean (FixelDataType& data, const size_t max_fixels, FixelDataType& vol) :
        Base (data, max_fixels),
        vol (vol) {}

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      default_type sum = 0.0;
      default_type sum_volumes = 0.0;
      if (vol.valid()) {
        for (auto f = Base::Loop (index) (data, vol); f; ++f) {
          if (!f.padding()) {
            sum += data.value() * vol.value();
            sum_volumes += vol.value();
          }
        }
        out.value() = sum_volumes ? (sum / sum_volumes) : 0.0;
      } else {
        for (auto f = Base::Loop (index) (data); f; ++f) {
          if (!f.padding()) {
            sum += data.value();
            sum_volumes += 1.0;
          }
        }
        out.value() = sum_volumes ? (sum / sum_volumes) : 0.0;
      }
    }
  protected:
    FixelDataType vol;
};


class Sum : protected Base
{ MEMALIGN (Sum)
  public:
    Sum (FixelDataType& data, const size_t max_fixels, FixelDataType& vol) :
        Base (data, max_fixels),
        vol (vol) {}

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      if (vol.valid()) {
        for (auto f = Base::Loop (index) (data, vol); f; ++f) {
          if (!f.padding())
            out.value() += data.value() * vol.value();
        }
      } else {
        for (auto f = Base::Loop (index) (data); f; ++f) {
          if (!f.padding())
            out.value() += data.value();
        }
      }
    }
  protected:
    FixelDataType vol;
};


class Product : protected Base
{ MEMALIGN (Product)
  public:
    Product (FixelDataType& data, const size_t max_fixels) :
        Base (data, max_fixels) { }

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      index.index(3) = 0;
      size_t num_fixels = index.value();
      if (!num_fixels) {
        out.value() = 0.0;
        return;
      }
      index.index(3) = 1;
      uint32_t offset = index.value();
      data.index(0) = offset;
      out.value() = data.value();
      num_fixels = max_fixels ? std::min (max_fixels, num_fixels) : num_fixels;
      for (size_t f = 1; f != num_fixels; ++f) {
        data.index(0)++;
        out.value() *= data.value();
      }
    }
};


class Min : protected Base
{ MEMALIGN (Min)
  public:
    Min (FixelDataType& data, const size_t max_fixels) :
        Base (data, max_fixels) { }

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      default_type min = std::numeric_limits<default_type>::infinity();
      for (auto f = Base::Loop (index) (data); f; ++f) {
        if (!f.padding() && data.value() < min)
          min = data.value();
      }
      out.value() = std::isfinite (min) ? min : NAN;
    }
};


class Max : protected Base
{ MEMALIGN (Max)
  public:
    Max (FixelDataType& data, const size_t max_fixels) :
        Base (data, max_fixels) { }

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      default_type max = -std::numeric_limits<default_type>::infinity();
      for (auto f = Base::Loop (index) (data); f; ++f) {
        if (!f.padding() && data.value() > max)
          max = data.value();
      }
      out.value() = std::isfinite (max) ? max : NAN;
    }
};


class AbsMax : protected Base
{ MEMALIGN (AbsMax)
  public:
    AbsMax (FixelDataType& data, const size_t max_fixels) :
        Base (data, max_fixels) { }

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      default_type absmax = -std::numeric_limits<default_type>::infinity();
      for (auto f = Base::Loop (index) (data); f; ++f) {
        if (!f.padding() && std::abs (data.value()) > absmax)
          absmax = std::abs (data.value());
      }
      out.value() = std::isfinite (absmax) ? absmax : 0.0;
    }
};


class MagMax : protected Base
{ MEMALIGN (MagMax)
  public:
    MagMax (FixelDataType& data, const size_t num_fixels) :
        Base (data, num_fixels) { }

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      default_type magmax = 0.0;
      for (auto f = Base::Loop (index) (data); f; ++f) {
        if (!f.padding() && std::abs (data.value()) > std::abs (magmax))
          magmax = data.value();
      }
      out.value() = std::isfinite (magmax) ? magmax : 0.0;
    }
};


class Complexity : protected Base
{ MEMALIGN (Complexity)
  public:
    Complexity (FixelDataType& data, const size_t max_fixels) :
        Base (data, max_fixels) { }

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      index.index(3) = 0;
      size_t num_fixels = index.value();
      num_fixels = max_fixels ? std::min (num_fixels, max_fixels) : num_fixels;
      if (num_fixels <= 1) {
        out.value() = 0.0;
        return;
      }
      default_type max = 0.0;
      default_type sum = 0.0;
      for (auto f = Base::Loop (index) (data); f; ++f) {
        if (!f.padding()) {
          max = std::max (max, default_type(data.value()));
          sum += data.value();
        }
      }
      out.value() = (default_type(num_fixels) / default_type(num_fixels-1.0)) * (1.0 - (max / sum));
    }
};


class SF : protected Base
{ MEMALIGN (SF)
  public:
    SF (FixelDataType& data, const size_t max_fixels) :
        Base (data, max_fixels) { }

    void operator() (Image<uint32_t>& index, FixelDataType& out)
    {
      default_type max = 0.0;
      default_type sum = 0.0;
      for (auto f = Base::Loop (index) (data); f; ++f) {
        if (!f.padding()) {
          max = std::max (max, default_type(data.value()));
          sum += data.value();
        }
      }
      out.value() = sum ? (max / sum) : 0.0;
    }
};


class DEC_unit : protected Base
{ MEMALIGN (DEC_unit)
  public:
    DEC_unit (FixelDataType& data, const size_t max_fixels, FixelDataType& vol, Image<float>& dir) :
        Base (data, max_fixels),
        vol (vol), dir (dir) {}

    void operator() (Image<uint32_t>& index, Image<float>& out)
    {
      Eigen::Vector3 sum_dec = {0.0, 0.0, 0.0};
      if (vol.valid()) {
        for (auto f = Base::Loop (index) (data, vol, dir); f; ++f) {
          if (!f.padding())
            sum_dec += Eigen::Vector3 (std::abs (dir.row(1)[0]), std::abs (dir.row(1)[1]), std::abs (dir.row(1)[2])) * data.value() * vol.value();
        }
      } else {
        for (auto f = Base::Loop (index) (data, dir); f; ++f) {
          if (!f.padding())
            sum_dec += Eigen::Vector3 (std::abs (dir.row(1)[0]), std::abs (dir.row(1)[1]), std::abs (dir.row(1)[2])) * data.value();
        }
      }
      if ((sum_dec.array() != 0.0).any())
        sum_dec.normalize();
      for (out.index(3) = 0; out.index(3) != 3; ++out.index(3))
        out.value() = sum_dec[size_t(out.index(3))];
    }
  protected:
    FixelDataType vol;
    Image<float> dir;
};


class DEC_scaled : protected Base
{ MEMALIGN (DEC_scaled)
  public:
    DEC_scaled (FixelDataType& data, const size_t max_fixels, FixelDataType& vol, Image<float>& dir) :
        Base (data, max_fixels),
        vol (vol), dir (dir) {}

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      Eigen::Vector3 sum_dec = {0.0, 0.0, 0.0};
      default_type sum_value = 0.0;
      if (vol.valid()) {
        default_type sum_volume = 0.0;
        for (auto f = Base::Loop (index) (data, vol, dir); f; ++f) {
          if (!f.padding()) {
            sum_dec += Eigen::Vector3 (std::abs (dir.row(1)[0]), std::abs (dir.row(1)[1]), std::abs (dir.row(1)[2])) * data.value() * vol.value();
            sum_volume += vol.value();
            sum_value += vol.value() * data.value();
          }
        }
        if ((sum_dec.array() != 0.0).any())
          sum_dec.normalize();
        sum_dec *= (sum_value / sum_volume);
      } else {
        for (auto f = Base::Loop (index) (data, dir); f; ++f) {
          if (!f.padding()) {
            sum_dec += Eigen::Vector3 (std::abs (dir.row(1)[0]), std::abs (dir.row(1)[1]), std::abs (dir.row(1)[2])) * data.value();
            sum_value += data.value();
          }
        }
        if ((sum_dec.array() != 0.0).any())
          sum_dec.normalize();
        sum_dec *= sum_value;
      }
      for (out.index(3) = 0; out.index(3) != 3; ++out.index(3))
        out.value() = sum_dec[size_t(out.index(3))];
    }
  protected:
    FixelDataType vol;
    Image<float> dir;
};


class SplitData : protected Base
{ MEMALIGN (SplitData)
  public:
    SplitData (FixelDataType& data, const size_t max_fixels) :
        Base (data, max_fixels, true) { }

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      for (auto f = Base::Loop (index) (data); f; ++f) {
        out.index(3) = f.fixel_index;
        out.value() = f.padding() ? pad_value : data.value();
      }
    }
};


class SplitDir : protected Base
{ MEMALIGN (SplitDir)
  public:
    SplitDir (FixelDataType& dir, const size_t max_fixels) :
        Base (dir, max_fixels, true, NAN) { }

    void operator() (FixelIndexType& index, Image<float>& out)
    {
      out.index(3) = 0;
      for (auto f = Base::Loop (index) (data); f; ++f) {
        if (f.padding()) {
          for (size_t axis = 0; axis < 3; ++axis) {
            out.value() = pad_value;
            ++out.index(3);
          }
        } else {
          for (size_t axis = 0; axis < 3; ++axis) {
            data.index(1) = axis;
            out.value() = data.value();
            ++out.index(3);
          }
        }
      }
    }
};







void run ()
{
  auto in_data = Fixel::open_fixel_data_file<typename FixelDataType::value_type> (argument[0]);
  if (in_data.size(2) != 1)
    throw Exception ("Input fixel data file must have a single scalar value per fixel (i.e. have dimensions Nx1x1)");

  Header in_index_header = Fixel::find_index_header (Fixel::get_fixel_directory (argument[0]));
  auto in_index_image = in_index_header.get_image<typename FixelIndexType::value_type>();

  Image<float> in_directions;

  const int op = argument[1];

  const size_t max_fixels = get_option_value ("number", 0);
  if (max_fixels && op == 7)
    throw Exception ("\"count\" statistic is meaningless if constraining the number of fixels per voxel using the -number option");

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
    if (max_fixels) {
      // 3 volumes per fixel if performing split_dir
      H_out.size(3) = (op == 13) ? (3 * max_fixels) : max_fixels;
    } else {
      uint32_t max_count = 0;
      for (auto l = Loop ("determining largest fixel count", in_index_image, 0, 3) (in_index_image); l; ++l)
        max_count = std::max (max_count, (uint32_t)in_index_image.value());
      if (max_count == 0)
        throw Exception ("fixel image is empty");
      // 3 volumes per fixel if performing split_dir
      H_out.size(3) = (op == 13) ? (3 * max_count) : max_count;
    }
  }

  if (op == 10 || op == 11 || op == 13)  // dec or split_dir
    in_directions = Fixel::find_directions_header (
                    Fixel::get_fixel_directory (in_data.name())).get_image<float>().with_direct_io();

  FixelDataType in_vol;
  auto opt = get_options ("weighted");
  if (opt.size()) {
    in_vol = FixelDataType::open (opt[0][0]);
    check_dimensions (in_data, in_vol);
  }

  if (op == 2 || op == 3 || op == 4 || op == 5 || op == 6 ||
      op == 7 || op == 8 || op == 9 || op == 12 || op == 13) {
    if (in_vol.valid())
      WARN ("Option -weighted has no meaningful interpretation for the operation specified; ignoring");
  }

  auto out = Image<float>::create (argument[2], H_out);

  auto loop = ThreadedLoop ("converting sparse fixel data to scalar image", in_index_image, 0, 3);

  switch (op) {
    case 0:  loop.run (Mean       (in_data, max_fixels, in_vol), in_index_image, out); break;
    case 1:  loop.run (Sum        (in_data, max_fixels, in_vol), in_index_image, out); break;
    case 2:  loop.run (Product    (in_data, max_fixels), in_index_image, out); break;
    case 3:  loop.run (Min        (in_data, max_fixels), in_index_image, out); break;
    case 4:  loop.run (Max        (in_data, max_fixels), in_index_image, out); break;
    case 5:  loop.run (AbsMax     (in_data, max_fixels), in_index_image, out); break;
    case 6:  loop.run (MagMax     (in_data, max_fixels), in_index_image, out); break;
    case 7:  loop.run ([](Image<uint32_t>& index, Image<float>& out) { // count
                          out.value() = index.value();
                       }, in_index_image, out); break;
    case 8:  loop.run (Complexity (in_data, max_fixels), in_index_image, out); break;
    case 9:  loop.run (SF         (in_data, max_fixels), in_index_image, out); break;
    case 10: loop.run (DEC_unit   (in_data, max_fixels, in_vol, in_directions), in_index_image, out); break;
    case 11: loop.run (DEC_scaled (in_data, max_fixels, in_vol, in_directions), in_index_image, out); break;
    case 12: loop.run (SplitData  (in_data, max_fixels), in_index_image, out); break;
    case 13: loop.run (SplitDir   (in_directions, max_fixels), in_index_image, out); break;
  }

}

