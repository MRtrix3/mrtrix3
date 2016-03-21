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
#include "dwi/tractography/mapping/mapper.h"




using namespace MR;
using namespace App;


enum stat_tck { INTEGRAL, MEAN, MEDIAN, MIN, MAX, NONE };
const char* statistics[] = { "integral", "mean", "median", "min", "max", NULL };


void usage ()
{

  DESCRIPTION
  + "sample values of an associated image along tracks"

  + "By default, the value of the underlying image at each point along the track "
    "is written to an ASCII file (with all values for each track on the same line). "
    "Alternatively, some statistic can be taken from the values along each streamline "
    "and written to a vector file.";

  ARGUMENTS
  + Argument ("tracks", "the input track file").type_tracks_in()
  + Argument ("image",  "the image to be sampled").type_image_in()
  + Argument ("values", "the output sampled values").type_file_out();

  OPTIONS
  + Option ("stat_tck", "compute some statistic from the values along each streamline "
                        "(options are: " + join(statistics, ","))
    + Argument ("statistic").type_choice (statistics)

  + Option ("precise", "use the precise mechanism for mapping streamlines to voxels "
                       "(obviates the need for trilinear interpolation) "
                       "(only applicable if some per-streamline statistic is requested)");
  
  // TODO add support for SH amplitude along tangent

}


typedef float value_type;
typedef Eigen::VectorXf vector_type;




// TODO Guarantee thread-safety
class Sampler {
  public:
    Sampler (Image<value_type>& image, const stat_tck statistic, const bool precise) :
        interp (precise ? nullptr : new Interp::Linear<Image<value_type>> (image)),
        mapper (precise ? new DWI::Tractography::Mapping::TrackMapperBase (image) : nullptr),
        image  (precise ? new Image<value_type> (image) : nullptr),
        statistic (statistic)
    {
      assert (!(statistic == stat_tck::NONE && precise));
      if (mapper)
        mapper->set_use_precise_mapping (true);
    }

    bool operator() (DWI::Tractography::Streamline<>& tck, std::pair<size_t, value_type>& out)
    {
      assert (statistic != stat_tck::NONE);
      out.first = tck.index;
      if (interp) {
        std::pair<size_t, vector_type> values;
        (*this) (tck, values);

        // TODO Compute statistic
        // Take distance between points into account in integral / mean calculation

      } else {
        DWI::Tractography::Mapping::SetVoxel voxels;
        (*mapper) (tck, voxels);
        if (statistic == INTEGRAL) {
          value_type integral = value_type(0.0);
          for (const auto v : voxels) {
            assign_pos_of (v).to (*image);
            integral += v.get_length() * image->value();
          }
          out.second = integral;
        } else if (statistic == MEAN) {
          value_type integral = value_type(0.0), sum_lengths = value_type (0.0);
          for (const auto v : voxels) {
            assign_pos_of (v).to (*image);
            integral += v.get_length() * image->value();
            sum_lengths += v.get_length();
          }
          out.second = integral / sum_lengths;
        } else if (statistic == MEDIAN) {
          // Should be a weighted median...
          // Just use the n.log(n) algorithm
          class WeightSort {
            public:
              WeightSort (const DWI::Tractography::Mapping::Voxel& voxel, const value_type value) :
                  value (value),
                  length (voxel.get_length()) { }
              bool operator< (const WeightSort& that) const { return value < that.value; }
              value_type value, length;
          };
          std::vector<WeightSort> data;
          value_type sum_lengths = value_type(0.0);
          for (const auto v : voxels) {
            assign_pos_of (v).to (*image);
            data.push_back (WeightSort (v, image->value()));
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
          value_type min = std::numeric_limits<value_type>::infinity();
          for (const auto v : voxels) {
            assign_pos_of (v).to (*image);
            min = std::min (min, value_type (image->value()));
          }
          out.second = min;
        } else if (statistic == MAX) {
          value_type max = -std::numeric_limits<value_type>::infinity();
          for (const auto v : voxels) {
            assign_pos_of (v).to (*image);
            max = std::max (max, value_type (image->value()));
          }
          out.second = max;
        }
      }
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
    const stat_tck statistic;

};



class ReceiverBase
{
  public:
    ReceiverBase (const size_t num_tracks) :
        expected (num_tracks),
        received (0),
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

  private:
    const size_t expected;
    size_t received;
    ProgressBar progress;

};


class Receiver_Statistic : private ReceiverBase
{
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



class Receiver_NoStatistic : private ReceiverBase
{
  public:
    Receiver_NoStatistic (const std::string& path, const size_t num_tracks) :
        ReceiverBase (num_tracks),
        file (path, std::ios_base::trunc) { }
    Receiver_NoStatistic (const Receiver_NoStatistic&) = delete;

    bool operator() (std::pair<size_t, vector_type>& in)
    {
      assert (in.first == ReceiverBase::received);
      file << in.second.transpose() << "\n";
      ++(*this);
      return true;
    }

  private:
    std::ofstream file;
};



void run ()
{
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> reader (argument[0], properties);
  auto image = Image<value_type>::open (argument[1]);

  auto opt = get_options ("stat_tck");
  const stat_tck statistic = opt.size() ? stat_tck(int(opt[0][0])) : stat_tck::NONE;
  const bool precise = get_options ("precise").size();
  const size_t num_tracks = properties.find("num_tracks") == properties.end() ?
                            0 :
                            to<size_t>(properties["num_tracks"]);

  if (statistic == stat_tck::NONE && precise)
    throw Exception ("Precise streamline mapping may only be used with per-streamline statistics");

  Sampler sampler (image, statistic, precise);

  if (statistic == stat_tck::NONE) {
    Receiver_NoStatistic receiver (argument[2], num_tracks);
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

