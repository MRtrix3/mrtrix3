/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 25/01/13.


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

#include <fstream>
#include <cstdio>

#include "command.h"
#include "timer.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/weights.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/loader.h"
#include "thread/queue.h"



using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "filter streamlines according to criteria such as inclusion / exclusion ROIs and length";

  ARGUMENTS
  + Argument ("in_tracks",  "the input track file").type_file()
  + Argument ("out_tracks", "the output track file").type_file();

  OPTIONS
  + Option ("include",
            "specify an inclusion region of interest, as either a binary mask image, "
            "or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines "
            "must traverse ALL inclusion regions to be accepted.")
          .allow_multiple()
          + Argument ("spec")

  + Option ("exclude",
            "specify an exclusion region of interest, as either a binary mask image, "
            "or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines "
            "that enter ANY exclude region will be discarded.")
          .allow_multiple()
          + Argument ("spec")

  + Option ("maxlength", "set the maximum length of any track in mm.")
          + Argument ("value").type_float (0.0, 0.0, INFINITY)

  + Option ("minlength", "set the minimum length of any track in mm.")
          + Argument ("value").type_float (0.0, 0.0, INFINITY)

  + Tractography::TrackWeightsInOption
  + Tractography::TrackWeightsOutOption;

}


typedef Tractography::TrackData<float> TrackData;


class Filter
{

  public:
    Filter (Tractography::Properties& p, const float step_size) :
      properties (p)
    {
      if (properties.find ("min_dist") != properties.end())
        min_num_points = Math::round (to<float>(properties["min_dist"]) / step_size) + 1;
      else
        min_num_points = 0;
      if (properties.find ("max_dist") != properties.end())
        max_num_points = Math::round (to<float>(properties["max_dist"]) / step_size);
      else
        max_num_points = INT_MAX;
    }


    bool operator() (const TrackData& in, TrackData& out)
    {
      out.clear();
      if ((in.size() < min_num_points) || (in.size() > max_num_points))
        return true;
      track_included.assign (properties.include.size(), false);
      for (TrackData::const_iterator i = in.begin(); i != in.end(); ++i) {
        if (!test_point (*i))
          return true;
      }
      if (traversed_all_include_regions())
        out = in;
      return true;
    }


  private:

    // Settings
    Tractography::Properties& properties;
    size_t min_num_points, max_num_points;


    // Members to be re-used
    std::vector<bool> track_included;


    // Helper functions
    bool test_point (const Point<float>& p)
    {
      if (properties.exclude.contains (p))
        return false;
      properties.include.contains (p, track_included);
      return true;
    }

    bool traversed_all_include_regions () const
    {
      for (size_t n = 0; n < track_included.size(); ++n)
        if (!track_included[n])
          return false;
      return true;
    }

};




#define UPDATE_INTERVAL 0.0333333 // 30 Hz - most monitors are 60Hz

class Writer : public Tractography::Writer<float>
{

  typedef Tractography::Writer<float> WriterBase;

  public:
    Writer (const std::string& path, Tractography::Properties& properties) :
      WriterBase (path, properties),
      in_count (properties.find ("count") == properties.end() ? 0 : to<size_t>(properties["count"])),
      timer (),
      next_time (timer.elapsed()) { }

    ~Writer ()
    {
      if (App::log_level > 0)
        fprintf (stderr, "\r%8zu read, %8zu filtered    [100%%]\n", WriterBase::total_count, WriterBase::count);
    }

    bool operator() (const TrackData& in)
    {
      if (App::log_level > 0 && timer.elapsed() >= next_time) {
        next_time += UPDATE_INTERVAL;
        fprintf (stderr, "\r%8zu read, %8zu filtered    [%3d%%]",
            WriterBase::total_count, WriterBase::count,
            in_count ? (int(100.0 * WriterBase::total_count / float(in_count))) : 0);
      }
      WriterBase::append (in);
      return true;
    }


  private:
    const size_t in_count;
    Timer timer;
    double next_time;

};




void run ()
{

  Tractography::Properties properties;
  Tractography::Reader<float> reader (argument[0], properties);

  Options opt = get_options ("include");
  for (size_t i = 0; i < opt.size(); ++i)
    properties.include.add (Tractography::ROI (opt[i][0]));

  opt = get_options ("exclude");
  for (size_t i = 0; i < opt.size(); ++i)
    properties.exclude.add (Tractography::ROI (opt[i][0]));

  bool using_length_filtering = false;

  opt = get_options ("maxlength");
  if (opt.size()) {
    properties["max_dist"] = std::string (opt[0][0]);
    using_length_filtering = true;
  }

  opt = get_options ("minlength");
  if (opt.size()) {
    properties["min_dist"] = std::string (opt[0][0]);
    using_length_filtering = true;
  }

  float step_size = 0.0;
  if (properties.find ("output_step_size") != properties.end())
    step_size = to<float> (properties["output_step_size"]);
  else
    step_size = to<float> (properties["step_size"]);

  if (using_length_filtering && (!step_size || !std::isfinite (step_size)))
    throw Exception ("Cannot filter streamlines by length as tractography step size is malformed");

  Filter filter (properties, step_size);
  Writer writer (argument[1], properties);

  Thread::run_batched_queue_threaded_pipe (reader, TrackData(), 100, filter, TrackData(), 100, writer);

}

