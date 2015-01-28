/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2013.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "command.h"
#include "point.h"
#include "progressbar.h"

#include "image/buffer.h"
#include "image/buffer_sparse.h"
#include "image/loop.h"
#include "image/voxel.h"

#include "image/sparse/fixel_metric.h"
#include "image/sparse/keys.h"
#include "image/sparse/voxel.h"

#include "math/SH.h"


using namespace MR;
using namespace App;


using Image::Sparse::FixelMetric;

const char* operations[] = {
  "mean",
  "sum",
  "product",
  "rms",
  "var",
  "std",
  "min",
  "max",
  "absmax",
  "magmax",
  "count",
  "complexity",
  "sf",
  "dec_unit",
  "dec_scaled",
  "split",
  NULL
};


void usage ()
{

  DESCRIPTION
  + "convert a fixel-based sparse-data image into some form of scalar image. "
    "This could be: \n"
    "- Some statistic computed across all fixel values within a voxel: mean, sum, product, rms, var, std, min, max, absmax, magmax\n"
    "- The number of fixels in each voxel: count\n"
    "- Some measure of crossing-fibre organisation: complexity, sf ('single-fibre')\n"
    "- A 4D directionally-encoded colour image: dec_unit, dec_scaled\n"
    "- A 4D scalar image with one 3D volume per fixel value: split";

  REFERENCES = "Reference for 'complexity' operation:\n"
               "Riffert, T. W.; Schreiber, J.; Anwander, A. & Knosche, T. R. "
               "Beyond Fractional Anisotropy: Extraction of bundle-specific structural metrics from crossing fibre models. "
               "NeuroImage 2014 (in press)";

  ARGUMENTS
  + Argument ("fixel_in",  "the input sparse fixel image.").type_image_in ()
  + Argument ("operation", "the operation to apply, one of: " + join(operations, ", ") + ".").type_choice (operations)
  + Argument ("image_out", "the output scalar image.").type_image_out ();

  OPTIONS
  + Option ("weighted", "weight the contribution of each fixel to the per-voxel result according to its volume "
                        "(note that this option is not applicable for all operations, and should be avoided if the "
                        "value stored in the fixel image is itself the estimated fibre volume)");

}



class OpBase
{
  protected:
    typedef Image::BufferSparse<FixelMetric>::voxel_type in_vox_type;
    typedef Image::Buffer<float>::voxel_type out_vox_type;

  public:
    OpBase (const bool weighted) :
        weighted (weighted) { }
    OpBase (const OpBase& that) :
        weighted (that.weighted) { }
    virtual ~OpBase() { }
    virtual bool operator() (in_vox_type&, out_vox_type&) = 0;
  protected:
    bool weighted;
};


class Mean : public OpBase
{
  public:
    Mean (const bool weighted) :
        OpBase (weighted),
        sum (0.0f),
        sum_volumes (0.0f) { }
    Mean (const Mean& that) :
        OpBase (that),
        sum (0.0f),
        sum_volumes (0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      sum = sum_volumes = 0.0;
      if (weighted) {
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum += in.value()[i].size * in.value()[i].value;
          sum_volumes += in.value()[i].size;
        }
        out.value() = sum_volumes ? (sum / sum_volumes) : 0.0;
      } else {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum += in.value()[i].value;
        out.value() = in.value().size() ? (sum / float(in.value().size())) : 0.0;
      }
      return true;
    }
  private:
    float sum, sum_volumes;
};

class Sum : public OpBase
{
  public:
    Sum (const bool weighted) :
        OpBase (weighted),
        sum (0.0f) { }
    Sum (const Sum& that) :
        OpBase (that),
        sum (0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      sum = 0.0;
      if (weighted) {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum += in.value()[i].size * in.value()[i].value;
        out.value() = sum;
      } else {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum += in.value()[i].value;
        out.value() = sum;
      }
      return true;
    }
  private:
    float sum;
};

class Product : public OpBase
{
  public:
    Product (const bool weighted) :
        OpBase (weighted),
        product (0.0f)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for product operation; ignoring");
    }
    Product (const Product& that) :
        OpBase (that),
        product (0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      if (!in.value().size()) {
        out.value() = 0.0;
        return true;
      }
      product = in.value()[0].value;
      for (size_t i = 1; i != in.value().size(); ++i)
        product *= in.value()[i].value;
      out.value() = product;
      return true;
    }
  private:
    float product;
};

class RMS : public OpBase
{
  public:
    RMS (const bool weighted) :
        OpBase (weighted),
        sum (0.0f),
        sum_volumes (0.0f) { }
    RMS (const RMS& that) :
        OpBase (that),
        sum (0.0f),
        sum_volumes (0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      sum = sum_volumes = 0.0f;
      if (weighted) {
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum *= in.value()[i].size * Math::pow2 (in.value()[i].value);
          sum_volumes += in.value()[i].size;
        }
        out.value() = std::sqrt (sum / sum_volumes);
      } else {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum += Math::pow2 (in.value()[i].value);
        out.value() = std::sqrt (sum / float(in.value().size()));
      }
      return true;
    }
  private:
    float sum, sum_volumes;
};

class Var : public OpBase
{
  public:
    Var (const bool weighted) :
        OpBase (weighted),
        sum (0.0f),
        weighted_mean (0.0f),
        sum_volumes (0.0f),
        sum_volumes_sqr (0.0f),
        sum_sqr (0.0f) { }
    Var (const Var& that) :
        OpBase (that),
        sum (0.0f),
        weighted_mean (0.0f),
        sum_volumes (0.0f),
        sum_volumes_sqr (0.0f),
        sum_sqr (0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      if (!in.value().size()) {
        out.value() = NAN;
        return true;
      } else if (in.value().size() == 1) {
        out.value() = 0.0f;
        return true;
      }
      if (weighted) {
        sum = sum_volumes = 0.0f;
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum += in.value()[i].size * in.value()[i].value;
          sum_volumes += in.value()[i].size;
        }
        weighted_mean = sum / sum_volumes;
        sum = sum_volumes_sqr = 0.0f;
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum += in.value()[i].size * Math::pow2 (weighted_mean - in.value()[i].value);
          sum_volumes_sqr += Math::pow2 (in.value()[i].size);
        }
        // Unbiased variance estimator in the presence of weighting
        out.value() = sum / (sum_volumes - (sum_volumes_sqr / sum_volumes));
      } else {
        sum = sum_sqr = 0.0f;
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum += in.value()[i].value;
          sum_sqr += Math::pow2 (in.value()[i].value);
        }
        out.value() = (sum_sqr - (Math::pow2 (sum) / float(in.value().size()))) / float(in.value().size() - 1);
      }
      return true;
    }
  private:
    // Used for both calculations
    float sum;
    // Used for weighted calculations
    float weighted_mean, sum_volumes, sum_volumes_sqr;
    // Used for unweighted calculations
    float sum_sqr;
};

class Std : public Var
{
  public:
    Std (const bool weighted) :
        Var (weighted) { }
    Std (const Std& that) :
        Var (that) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      Var::operator() (in, out);
      out.value() = std::sqrt (out.value());
      return true;
    }
};

class Min : public OpBase
{
  public:
    Min (const bool weighted) :
        OpBase (weighted),
        min (std::numeric_limits<float>::infinity())
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for min operation; ignoring");
    }
    Min (const Min& that) :
        OpBase (that),
        min (std::numeric_limits<float>::infinity()) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      min = std::numeric_limits<float>::infinity();
      for (size_t i = 0; i != in.value().size(); ++i) {
        if (in.value()[i].value < min)
          min = in.value()[i].value;
      }
      out.value() = std::isfinite (min) ? min : NAN;
      return true;
    }
  private:
    float min;
};

class Max : public OpBase
{
  public:
    Max (const bool weighted) :
        OpBase (weighted),
        max (-std::numeric_limits<float>::infinity())
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for max operation; ignoring");
    }
    Max (const Max& that) :
        OpBase (that),
        max (-std::numeric_limits<float>::infinity()) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      max = -std::numeric_limits<float>::infinity();
      for (size_t i = 0; i != in.value().size(); ++i) {
        if (in.value()[i].value > max)
          max = in.value()[i].value;
      }
      out.value() = std::isfinite (max) ? max : NAN;
      return true;
    }
  private:
    float max;
};

class AbsMax : public OpBase
{
  public:
    AbsMax (const bool weighted) :
        OpBase (weighted),
        max (0.0f)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for absmax operation; ignoring");
    }
    AbsMax (const AbsMax& that) :
        OpBase (that),
        max (0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      max = 0.0f;
      for (size_t i = 0; i != in.value().size(); ++i) {
        if (std::abs (in.value()[i].value) > max)
          max = std::abs (in.value()[i].value);
      }
      out.value() = max;
      return true;
    }
  private:
    float max;
};

class MagMax : public OpBase
{
  public:
    MagMax (const bool weighted) :
        OpBase (weighted),
        max (0.0f)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for magmax operation; ignoring");
    }
    MagMax (const AbsMax& that) :
        OpBase (that),
        max (0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      max = 0.0f;
      for (size_t i = 0; i != in.value().size(); ++i) {
        if (std::abs (in.value()[i].value) > std::abs (max))
          max = in.value()[i].value;
      }
      out.value() = max;
      return true;
    }
  private:
    float max;
};

class Count : public OpBase
{
  public:
    Count (const bool weighted) :
        OpBase (weighted)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for count operation; ignoring");
    }
    Count (const Count& that) :
        OpBase (that) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      out.value() = in.value().size();
      return true;
    }
};

class Complexity : public OpBase
{
  public:
    Complexity (const bool weighted) :
        OpBase (weighted),
        max (0.0f),
        sum (0.0f)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for complexity operation; ignoring");
    }
    Complexity (const Complexity& that) :
        OpBase (that),
        max (0.0f),
        sum (0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      if (in.value().size() <= 1) {
        out.value() = 0.0;
        return true;
      }
      max = sum = 0.0f;
      for (size_t i = 0; i != in.value().size(); ++i) {
        max = std::max (max, in.value()[i].value);
        sum += in.value()[i].value;
      }
      out.value() = (float(in.value().size()) / float(in.value().size()-1.0)) * (1.0 - (max / sum));
      return true;
    }
  private:
    float max, sum;
};

class SF : public OpBase
{
  public:
    SF (const bool weighted) :
        OpBase (weighted),
        max (0.0f),
        sum (0.0f)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for sf operation; ignoring");
    }
    SF (const Complexity& that) :
        OpBase (that),
        max (0.0f),
        sum (0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      max = sum = 0.0f;
      for (size_t i = 0; i != in.value().size(); ++i) {
        max = std::max (max, in.value()[i].value);
        sum += in.value()[i].value;
      }
      out.value() = sum ? (max / sum) : 0.0f;
      return true;
    }
  private:
    float max, sum;
};

class DEC_unit : public OpBase
{
  public:
    DEC_unit (const bool weighted) :
        OpBase (weighted),
        sum_dec (0.0f, 0.0f, 0.0f) { }
    DEC_unit (const DEC_unit& that) :
        OpBase (that),
        sum_dec (0.0f, 0.0f, 0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      sum_dec.set (0.0f, 0.0f, 0.0f);
      if (weighted) {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum_dec += Point<float> (std::abs (in.value()[i].dir[0]), std::abs (in.value()[i].dir[1]), std::abs (in.value()[i].dir[2])) * in.value()[i].value * in.value()[i].size;
      } else {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum_dec += Point<float> (std::abs (in.value()[i].dir[0]), std::abs (in.value()[i].dir[1]), std::abs (in.value()[i].dir[2])) * in.value()[i].value;
      }
      sum_dec.normalise();
      for (out[3] = 0; out[3] != 3; ++out[3])
        out.value() = sum_dec[size_t(out[3])];
      return true;
    }
  private:
    Point<float> sum_dec;
};

class DEC_scaled : public OpBase
{
  public:
    DEC_scaled (const bool weighted) :
        OpBase (weighted),
        sum_dec (0.0f, 0.0f, 0.0f),
        sum_volume (0.0f),
        sum_value (0.0f) { }
    DEC_scaled (const DEC_scaled& that) :
        OpBase (that),
        sum_dec (0.0f, 0.0f, 0.0f),
        sum_volume (0.0f),
        sum_value (0.0f) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      sum_dec.set (0.0f, 0.0f, 0.0f);
      sum_value = 0.0f;
      if (weighted) {
        sum_volume = 0.0f;
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum_dec += Point<float> (std::abs (in.value()[i].dir[0]), std::abs (in.value()[i].dir[1]), std::abs (in.value()[i].dir[2])) * in.value()[i].value * in.value()[i].size;
          sum_volume += in.value()[i].size;
          sum_value += in.value()[i].size * in.value()[i].value;
        }
        sum_dec.normalise();
        sum_dec *= (sum_value / sum_volume);
      } else {
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum_dec += Point<float> (std::abs (in.value()[i].dir[0]), std::abs (in.value()[i].dir[1]), std::abs (in.value()[i].dir[2])) * in.value()[i].value;
          sum_value += in.value()[i].value;
        }
        sum_dec.normalise();
        sum_dec *= sum_value;
      }
      for (out[3] = 0; out[3] != 3; ++out[3])
        out.value() = sum_dec[size_t(out[3])];
      return true;
    }
  private:
    Point<float> sum_dec;
    float sum_volume, sum_value;
};

class Split : public OpBase
{
  public:
    Split (const bool weighted) :
        OpBase (weighted)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for split operation; ignoring");
    }
    Split (const Split& that) :
        OpBase (that) { }
    bool operator() (in_vox_type& in, out_vox_type& out)
    {
      for (out[3] = 0; out[3] < out.dim(3); ++out[3]) {
        if (out[3] < in.value().size())
          out.value() = in.value()[out[3]].value;
        else
          out.value() = 0.0;
      }
      return true;
    }
};








void run ()
{
  Image::Header H_in (argument[0]);
  Image::BufferSparse<FixelMetric> fixel_data (H_in);
  auto voxel = fixel_data.voxel();

  const int op = argument[1];

  Image::Header H_out (H_in);
  H_out.datatype() = DataType::Float32;
  H_out.datatype().set_byte_order_native();
  H_out.erase (Image::Sparse::name_key);
  H_out.erase (Image::Sparse::size_key);
  if (op == 10) { // count
    H_out.datatype() = DataType::UInt8;
  } else if (op == 13 || op == 14) { // dec
    H_out.set_ndim (4);
    H_out.dim (3) = 3;
  } else if (op == 15) { // split
    H_out.set_ndim (4);
    uint32_t max_count = 0;
    Image::BufferSparse<FixelMetric>::voxel_type voxel (fixel_data);
    Image::LoopInOrder loop (voxel, "determining largest fixel count... ");
    for (loop.start (voxel); loop.ok(); loop.next (voxel))
      max_count = std::max (max_count, voxel.value().size());
    if (max_count == 0)
      throw Exception ("fixel image is empty");
    H_out.dim(3) = max_count;
  }

  Image::Buffer<float> out_data (argument[2], H_out);
  auto out = out_data.voxel();

  Options opt = get_options ("weighted");
  const bool weighted = opt.size();

  Image::ThreadedLoop loop ("converting sparse fixel data to scalar image... ", voxel);

  switch (op) {
    case 0:  loop.run (Mean       (weighted), voxel, out); break;
    case 1:  loop.run (Sum        (weighted), voxel, out); break;
    case 2:  loop.run (Product    (weighted), voxel, out); break;
    case 3:  loop.run (RMS        (weighted), voxel, out); break;
    case 4:  loop.run (Var        (weighted), voxel, out); break;
    case 5:  loop.run (Std        (weighted), voxel, out); break;
    case 6:  loop.run (Min        (weighted), voxel, out); break;
    case 7:  loop.run (Max        (weighted), voxel, out); break;
    case 8:  loop.run (AbsMax     (weighted), voxel, out); break;
    case 9:  loop.run (MagMax     (weighted), voxel, out); break;
    case 10: loop.run (Count      (weighted), voxel, out); break;
    case 11: loop.run (Complexity (weighted), voxel, out); break;
    case 12: loop.run (SF         (weighted), voxel, out); break;
    case 13: loop.run (DEC_unit   (weighted), voxel, out); break;
    case 14: loop.run (DEC_scaled (weighted), voxel, out); break;
    case 15: loop.run (Split      (weighted), voxel, out); break;
  }

}

