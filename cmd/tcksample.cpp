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
#include "image_helpers.h"
#include "memory.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/mapping/mapper.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "math/median.h"




using namespace MR;
using namespace App;


enum stat_tck { MEAN, MEDIAN, MIN, MAX, NONE };
const char* statistics[] = { "mean", "median", "min", "max", nullptr };


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "sample values of an associated image along tracks"

  + "By default, the value of the underlying image at each point along the track "
    "is written to either an ASCII file (with all values for each track on the same "
    "line), or a track scalar file (.tsf). Alternatively, some statistic can be "
    "taken from the values along each streamline and written to a vector file.";

  ARGUMENTS
  + Argument ("tracks", "the input track file").type_tracks_in()
  + Argument ("image",  "the image to be sampled").type_image_in()
  + Argument ("values", "the output sampled values").type_file_out();

  OPTIONS

  + Option ("stat_tck", "compute some statistic from the values along each streamline "
                        "(options are: " + join(statistics, ",") + ")")
    + Argument ("statistic").type_choice (statistics)

  + Option ("precise", "use the precise mechanism for mapping streamlines to voxels "
                       "(obviates the need for trilinear interpolation) "
                       "(only applicable if some per-streamline statistic is requested)")

  + Option ("use_tdi_fraction",
            "each streamline is assigned a fraction of the image intensity "
            "in each voxel based on the fraction of the track density "
            "contributed by that streamline (this is only appropriate for "
            "processing a whole-brain tractogram, and images for which the "
            "quantiative parameter is additive)");

  
  // TODO add support for SH amplitude along tangent
  // TODO add support for reading from fixel image
  //   (this would supersede fixel2tsf when used without -precise or -stat_tck options)
  //   (wait until fixel_twi is merged; should simplify)


}



typedef float value_type;
typedef Eigen::VectorXf vector_type;



class TDI : public Image<value_type> { MEMALIGN(TDI)
  public:
    TDI (const Header& H, const size_t num_tracks) :
        Image<value_type> (Image<value_type>::scratch (H, "TDI scratch image")),
        progress ("Generating initial TDI", num_tracks) { }

    bool operator() (const DWI::Tractography::Mapping::SetVoxel& in)
    {
      for (const auto v : in) {
        assign_pos_of (v, 0, 3).to (*this);
        value() += v.get_length();
      }
      ++progress;
      return true;
    }

    void done() { progress.done(); }

  protected:
    ProgressBar progress;

};



// Guarantees thread-safety
class Sampler { MEMALIGN(Sampler)
  public:
    Sampler (Image<value_type>& image, const stat_tck statistic, const bool precise, std::unique_ptr<TDI>& precalc_tdi) :
        interp ((!precise && !precalc_tdi) ? new Interp::Linear<Image<value_type>> (image) : nullptr),
        mapper ((precise || precalc_tdi) ? new DWI::Tractography::Mapping::TrackMapperBase (image) : nullptr),
        image  ((precise || precalc_tdi) ? new Image<value_type> (image) : nullptr),
        tdi (precalc_tdi ? new TDI (*precalc_tdi) : nullptr),
        statistic (statistic)
    {
      assert (!(statistic == stat_tck::NONE && precise));
      if (mapper)
        mapper->set_use_precise_mapping (precise);
    }

    bool operator() (DWI::Tractography::Streamline<>& tck, std::pair<size_t, value_type>& out)
    {
      assert (statistic != stat_tck::NONE);
      out.first = tck.index;
      value_type sum_lengths = value_type(0);

      // Only if _not_ using precise mapping, and _not_ using a pre-calculated TDI
      //   (in the latter case, the mapper will still be used, just without the precise mapping;
      //   each traversed voxel will return a length of 1)
      if (interp) {

        std::pair<size_t, vector_type> values;
        (*this) (tck, values);

        if (statistic == MEAN) {
          // Take distance between points into account in mean calculation
          //   (Should help down-weight endpoints)
          value_type integral = value_type(0), sum_lengths = value_type(0);
          for (size_t i = 0; i != tck.size(); ++i) {
            value_type length = value_type(0);
            if (i)
              length += (tck[i] - tck[i-1]).norm();
            if (i < tck.size() - 1)
              length += (tck[i+1] - tck[i]).norm();
            length *= 0.5;
            integral += values.second[i] * length;
            sum_lengths += length;
          }
          out.second = sum_lengths ? (integral / sum_lengths) : 0.0;
        } else {
          sum_lengths = tck.calc_length();
          if (statistic == MEDIAN) {
            // Don't bother with a weighted median here
            std::vector<value_type> data;
            data.assign (values.second.data(), values.second.data() + values.second.size());
            out.second = Math::median (data);
          } else if (statistic == MIN) {
            out.second = std::numeric_limits<value_type>::infinity();
            for (size_t i = 0; i != tck.size(); ++i)
              out.second = std::min (out.second, values.second[i]);
          } else if (statistic == MAX) {
            out.second = -std::numeric_limits<value_type>::infinity();
            for (size_t i = 0; i != tck.size(); ++i)
              out.second = std::max (out.second, values.second[i]);
          } else {
            assert (0);
          }
        }

      } else {

        DWI::Tractography::Mapping::SetVoxel voxels;
        (*mapper) (tck, voxels);

        if (statistic == MEAN) {
          value_type integral = value_type(0.0);
          for (const auto v : voxels) {
            assign_pos_of (v).to (*image);
            integral += v.get_length() * (image->value() * get_tdi_multiplier (v));
            sum_lengths += v.get_length();
          }
          out.second = integral / sum_lengths;
        } else if (statistic == MEDIAN) {
          // Should be a weighted median...
          // Just use the n.log(n) algorithm
          class WeightSort { NOMEMALIGN
            public:
              WeightSort (const DWI::Tractography::Mapping::Voxel& voxel, const value_type value) :
                  value (value),
                  length (voxel.get_length()) { }
              bool operator< (const WeightSort& that) const { return value < that.value; }
              value_type value, length;
          };
          std::vector<WeightSort> data;
          for (const auto v : voxels) {
            assign_pos_of (v).to (*image);
            data.push_back (WeightSort (v, (image->value() * get_tdi_multiplier (v))));
            sum_lengths += v.get_length();
          }
          std::sort (data.begin(), data.end());
          const value_type target_length = 0.5 * sum_lengths;
          sum_lengths = value_type(0.0);
          value_type prev_value = data.front().value;
          for (const auto d : data) {
            if ((sum_lengths += d.length) > target_length) {
              out.second = prev_value;
              break;
            }
            prev_value = d.value;
          }
        } else if (statistic == MIN) {
          out.second = std::numeric_limits<value_type>::infinity();
          for (const auto v : voxels) {
            assign_pos_of (v).to (*image);
            out.second = std::min (out.second, value_type (image->value() * get_tdi_multiplier (v)));
            sum_lengths += v.get_length();
          }
        } else if (statistic == MAX) {
          out.second = -std::numeric_limits<value_type>::infinity();
          for (const auto v : voxels) {
            assign_pos_of (v).to (*image);
            out.second = std::max (out.second, value_type (image->value() * get_tdi_multiplier (v)));
            sum_lengths += v.get_length();
          }
        } else {
          assert (0);
        }
      }

      if (!std::isfinite (out.second))
        out.second = NaN;

      return true;
    }

    bool operator() (const DWI::Tractography::Streamline<>& tck, std::pair<size_t, vector_type>& out)
    {
      assert (interp);
      out.first = tck.index;
      out.second.resize (tck.size());
      for (size_t i = 0; i != tck.size(); ++i) {
        if (interp->scanner (tck[i]))
          out.second[i] = interp->value();
        else
          out.second[i] = std::numeric_limits<value_type>::quiet_NaN();
      }
      return true;
    }

  private:
    MR::copy_ptr<Interp::Linear<Image<value_type>>> interp;
    std::shared_ptr<DWI::Tractography::Mapping::TrackMapperBase> mapper;
    MR::copy_ptr<Image<value_type>> image;
    MR::copy_ptr<TDI> tdi;
    const stat_tck statistic;

    value_type get_tdi_multiplier (const DWI::Tractography::Mapping::Voxel& v)
    {
      if (!tdi)
        return value_type(1);
      assign_pos_of (v).to (*tdi);
      assert (!is_out_of_bounds (*tdi));
      return v.get_length() / tdi->value();
    }

};



class ReceiverBase { MEMALIGN(ReceiverBase)
  public:
    ReceiverBase (const size_t num_tracks) :
        received (0),
        expected (num_tracks),
        progress ("Sampling values underlying streamlines", num_tracks) { }

    ReceiverBase (const ReceiverBase&) = delete;

    virtual ~ReceiverBase() {
      if (received != expected)
        WARN ("Track file reports " + str(expected) + " tracks, but contains " + str(received));
    }

  protected:
    void operator++ () {
      ++received;
      ++progress;
    }

    size_t received;

  private:
    const size_t expected;
    ProgressBar progress;

};


class Receiver_Statistic : private ReceiverBase { MEMALIGN(Receiver_Statistic)
  public:
    Receiver_Statistic (const size_t num_tracks) :
        ReceiverBase (num_tracks),
        vector_data (vector_type::Zero (num_tracks)) { }
    Receiver_Statistic (const Receiver_Statistic&) = delete;

    bool operator() (std::pair<size_t, value_type>& in) {
      if (in.first >= size_t(vector_data.size()))
        vector_data.conservativeResizeLike (vector_type::Zero (in.first + 1));
      vector_data[in.first] = in.second;
      ++(*this);
      return true;
    }

    void save (const std::string& path) {
      MR::save_vector (vector_data, path);
    }

  private:
    vector_type vector_data;
};



class Receiver_NoStatistic : private ReceiverBase { MEMALIGN(Receiver_NoStatistic)
  public:
    Receiver_NoStatistic (const std::string& path,
                          const size_t num_tracks,
                          const DWI::Tractography::Properties& properties) :
        ReceiverBase (num_tracks)
    {
      if (Path::has_suffix (path, ".tsf"))
        tsf.reset (new DWI::Tractography::ScalarWriter<value_type> (path, properties));
      else
        ascii.reset (new File::OFStream (path));
    }
    Receiver_NoStatistic (const Receiver_NoStatistic&) = delete;

    bool operator() (std::pair<size_t, vector_type>& in)
    {
      // Requires preservation of order
      assert (in.first == ReceiverBase::received);
      if (ascii)
        (*ascii) << in.second.transpose() << "\n";
      else
        (*tsf) (in.second);
      ++(*this);
      return true;
    }

  private:
    std::unique_ptr<File::OFStream> ascii;
    std::unique_ptr<DWI::Tractography::ScalarWriter<value_type>> tsf;
};



void run ()
{
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> reader (argument[0], properties);
  auto H = Header::open (argument[1]);
  auto image = H.get_image<value_type>();

  auto opt = get_options ("stat_tck");
  const stat_tck statistic = opt.size() ? stat_tck(int(opt[0][0])) : stat_tck::NONE;
  const bool precise = get_options ("precise").size();
  const size_t num_tracks = properties.find("count") == properties.end() ?
                            0 :
                            to<size_t>(properties["count"]);

  if (statistic == stat_tck::NONE && precise)
    throw Exception ("Precise streamline mapping may only be used with per-streamline statistics");

  std::unique_ptr<TDI> tdi;
  if (get_options ("use_tdi_fraction").size()) {
    DWI::Tractography::Reader<value_type> tdi_reader (argument[0], properties);
    DWI::Tractography::Mapping::TrackMapperBase mapper (H);
    mapper.set_use_precise_mapping (precise);
    tdi.reset (new TDI (H, num_tracks));
    Thread::run_queue (tdi_reader,
                       Thread::batch (DWI::Tractography::Streamline<value_type>()),
                       Thread::multi (mapper),
                       Thread::batch (DWI::Tractography::Mapping::SetVoxel()),
                       *tdi);
    tdi->done();
  }

  Sampler sampler (image, statistic, precise, tdi);

  if (statistic == stat_tck::NONE) {
    Receiver_NoStatistic receiver (argument[2], num_tracks, properties);
    DWI::Tractography::Streamline<value_type> tck;
    std::pair<size_t, vector_type> values;
    while (reader (tck)) {
      sampler (tck, values);
      receiver (values);
    }
  } else {
    Receiver_Statistic receiver (num_tracks);
    Thread::run_queue (reader,
                       Thread::batch (DWI::Tractography::Streamline<value_type>()),
                       Thread::multi (sampler),
                       Thread::batch (std::pair<size_t, value_type>()),
                       receiver);
    receiver.save (argument[2]);
  }

}

