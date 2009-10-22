/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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


#include "app.h"
#include "file/config.h"
#include "image/voxel.h"
#include "dataset/interp.h"
#include "math/vector.h"
#include "point.h"
#include "math/SH.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/roi.h"
#include "thread/queue.h"
#include "thread/exec.h"


using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "perform streamlines tracking.",
  NULL
};

const char* type_choices[] = { "DT_STREAM", "DT_PROB", "SD_STREAM", "SD_PROB", NULL };

ARGUMENTS = {
  Argument ("FOD", "FOD image", "the image containing the FOD data, represented in spherical harmonics.").type_image_in(),
  Argument ("tracks", "output tracks file", "the output file containing the tracks generated.").type_file(),
  Argument::End
};


OPTIONS = {
  Option ("seed", "seed region", "specify the seed region of interest.", AllowMultiple)
    .append (Argument ("spec", "ROI specification", "specifies the parameters necessary to define the ROI. This should be either the path to a binary mask image, or a comma-separated list of 4 floating-point values, specifying the [x,y,z] coordinates of the centre and radius of a spherical ROI.").type_string()),

  Option ("include", "inclusion ROI", "specify an inclusion region of interest, in the same format as the seed region. Only tracks that enter all such inclusion ROI will be produced.", Optional | AllowMultiple)
    .append (Argument ("spec", "ROI specification", "specifies the parameters necessary to define the ROI.").type_string()),

  Option ("exclude", "exclusion ROI", "specify an exclusion region of interest, in the same format as the seed region. Only tracks that enter any such exclusion ROI will be discarded.", Optional | AllowMultiple)
    .append (Argument ("spec", "ROI specification", "specifies the parameters necessary to define the ROI.").type_string()),

  Option ("mask", "mask ROI", "specify a mask region of interest, in the same format as the seed region. Tracks will be terminated when they leave any such ROI.", Optional | AllowMultiple)
    .append (Argument ("spec", "ROI specification", "specifies the parameters necessary to define the ROI.").type_string()),

  Option ("step", "step size", "set the step size of the algorithm.")
    .append (Argument ("size", "step size", "the step size to use in mm (default is 0.2 mm).").type_float (1e-6, 10.0, 0.2)),

  Option ("curvature", "radius of curvature", "set the minimum radius of curvature (default is 2 mm for DT_STREAM, 0 for SD_STREAM, 1 mm for SD_PROB and DT_PROB).")
    .append (Argument ("radius", "radius of curvature", "the radius of curvature to use in mm.").type_float (1e-6, 10.0, 2.0)),

  Option ("number", "desired number of tracks", "set the desired number of tracks. The program will continue to generate tracks until this number of tracks have been selected and written to the output file (default is 100 for *_STREAM methods, 1000 for *_PROB methods).")
    .append (Argument ("tracks", "number of tracks", "the number of tracks.").type_integer (1, INT_MAX, 1)),

  Option ("maxnum", "maximum number of tracks to generate", "set the maximum number of tracks to generate. The program will not generate more tracks than this number, even if the desired number of tracks hasn't yet been reached (default is 100 x number).")
    .append (Argument ("tracks", "maximum number of tracks", "the maximum number of tracks.").type_integer (1, INT_MAX, 1)),

  Option ("length", "track length", "set the maximum length of any track.")
    .append (Argument ("value", "track distance", "the maximum length to use in mm (default is 200 mm).").type_float (1e-2, 1e6, 200.0)),

  Option ("minlength", "minimum track length", "set the minimum length of any track.")
    .append (Argument ("value", "track distance", "the minimum length to use in mm (default is 10 mm).").type_float (1e-2, 1e6, 10.0)),

  Option ("cutoff", "cutoff threshold", "set the FA or FOD amplitude cutoff for terminating tracks (default is 0.1).")
    .append (Argument ("value", "value", "the cutoff to use.").type_float (0, 1e6, 0.1)),

  Option ("initcutoff", "intial cutoff threshold", "set the minimum FA or FOD amplitude for initiating tracks (default is twice the normal cutoff).")
    .append (Argument ("value", "value", "the initial cutoff to use.").type_float (0, 1e6, 0.1)),

  Option ("trials", "number of trials", "set the maximum number of sampling trials at each point (only used for probabilistic tracking).")
    .append (Argument ("number", "number", "the number of trials.").type_integer(1, 10000, 50)),

  Option ("unidirectional", "unidirectional", "track from the seed point in one direction only (default is to track in both directions)."),

  Option ("initdirection", "initial direction", "specify an initial direction for the tracking.")
    .append (Argument ("dir", "direction", "the vector specifying the initial direction.").type_sequence_float()),

  Option ("noprecomputed", "no precomputation", "do NOT pre-compute legendre polynomial values. Warning: this will slow down the algorithm by a factor of approximately 4."),

  Option::End
};



typedef std::vector<Point>   Track;

class TrackAllocator {
  public:
    TrackAllocator (size_t number_of_elements) : N (number_of_elements) { }
    Track* alloc () { Track* tck = new Track (); tck->reserve (N); return (tck); }
    void reset (Track* tck) { tck->clear(); }
    void dealloc (Track* tck) { delete tck; }
  private:
    size_t N;
};

typedef Thread::Queue<Track,TrackAllocator> TrackQueue;

class TrackShared {
  public:
    TrackShared (const Image::Header& FOD, DWI::Tractography::Properties& properties, Point init_direction) :
      FOD_object (FOD),
      precomputer (lmax),
      init_dir (init_direction), 
      max_num_tracks (1000),
      lmax (Math::SH::LforN (FOD.dim(3))),
      max_trials (50),
      min_curv (1.0),
      total_seed_volume (0.0),
      step_size (0.1),
      threshold (0.1), 
      precomputed (true) {

        properties["method"] = "SD_PROB";
        properties["source"] = FOD_object.name();

        if (properties["step_size"].empty()) properties["step_size"] = str (step_size); 
        else step_size = to<float> (properties["step_size"]); 

        if (properties["threshold"].empty()) properties["threshold"] = str (threshold);
        else threshold = to<float> (properties["threshold"]); 

        if (properties["init_threshold"].empty()) { init_threshold = 2.0*threshold; properties["init_threshold"] = str (init_threshold); }
        else init_threshold = to<float> (properties["init_threshold"]); 

        if (properties["min_curv"].empty()) properties["min_curv"] = str (min_curv); 
        else min_curv = to<float> (properties["min_curv"]);

        if (properties["max_num_tracks"].empty()) properties["max_num_tracks"] = "1000";

        if (properties["max_dist"].empty()) properties["max_dist"] = "200.0";
        max_num_points = round (to<float> (properties["max_dist"])/step_size);

        if (properties["lmax"].empty()) properties["lmax"] = str (lmax); 
        else lmax = to<int> (properties["lmax"]);

        if (properties["max_trials"].empty()) properties["max_trials"] = str (max_trials); 
        else max_trials = to<int> (properties["max_trials"]);

        if (properties["sh_precomputed"].empty()) properties["sh_precomputed"] = ( precomputed ? "1" : "0" ); 
        else precomputed = to<int> (properties["sh_precomputed"]);

        dist_spread = curv2angle (step_size, min_curv);

        max_num_attempts = properties["max_num_attempts"].empty() ? 100 * max_num_tracks : to<int> (properties["max_num_attempts"]);
        unidirectional = to<int> (properties["unidirectional"]);
        min_size = round (to<float> (properties["min_dist"]) / to<float> (properties["step_size"]));
      }

    size_t max_track_size () const { return (max_num_points); }

  private:
    const Image::Header& FOD_object;
    Math::SH::PrecomputedAL<float> precomputer;
    const Point init_dir;
    size_t max_num_tracks, max_num_attempts, min_size, lmax, max_trials, num_points, max_num_points;
    float min_curv, min_dpi, dist_spread, total_seed_volume, step_size, threshold, init_threshold;
    bool unidirectional, precomputed;

    static float curv2angle (float step_size_, float curv)     { return (2.0 * asin (step_size_ / (2.0 * curv))); }

    void new_seed (Point& pos, Point& dir) const { }

    friend class Tracker;
    friend class TrackWriter;
};







class Tracker {
  public:
    Tracker (TrackQueue& queue, const TrackShared& shared) :
      writer (queue),
      S (shared),
      rng (rng_seed++) { }

    Tracker (const Tracker& T) : writer (T.writer), S (T.S), rng (rng_seed++) { }

    void execute () { 
      TrackQueue::Writer::Item item (writer);
      do {
        gen_track (*item);
        if (item->size() > S.min_size || track_excluded || !track_included()) item->clear();
      } while (item.write());
    }


  private:
    TrackQueue::Writer writer;
    const TrackShared& S;
    Math::RNG rng;
    Point pos, dir;
    bool track_excluded;
    bool track_included () const;

    static size_t rng_seed;

    bool excluded (const Point& p) const;
    bool included (const Point& p) const;

    void gen_track (Track& tck) {
      S.new_seed (pos, dir);
      Point seed_dir (dir);

      while (iterate()) tck.push_back (pos);
      if (!track_excluded && !S.unidirectional) {
        reverse (tck.begin(), tck.end());
        dir[0] = -seed_dir[0];
        dir[1] = -seed_dir[1];
        dir[2] = -seed_dir[2];
        pos = tck.back();
        while (iterate()) tck.push_back (pos);
      }
    }

    bool iterate () {
      return (true);
    }

    Point new_rand_dir () {
      float v[3];
      do { 
        v[0] = 2.0*rng.uniform() - 1.0; 
        v[1] = 2.0*rng.uniform() - 1.0; 
      } while (v[0]*v[0] + v[1]*v[1] > 1.0); 

      v[0] *= S.dist_spread;
      v[1] *= S.dist_spread;
      v[2] = 1.0 - (v[0]*v[0] + v[1]*v[1]);
      v[2] = v[2] < 0.0 ? 0.0 : sqrt (v[2]);

      if (dir[0]*dir[0] + dir[1]*dir[1] < 1e-4) 
        return (Point (v[0], v[1], dir[2] > 0.0 ? v[2] : -v[2]));

      float y[] = { dir[0], dir[1], 0.0 };
      Math::normalise (y);
      float x[] =  { -y[1], y[0], 0.0 };
      float y2[] = { -x[1]*dir[2], x[0]*dir[2], x[1]*dir[0] - x[0]*dir[1] };
      Math::normalise (y2);

      float cx = v[0]*x[0] + v[1]*x[1];
      float cy = v[0]*y[0] + v[1]*y[1];

      return (Point (
            cx*x[0] + cy*y2[0] + v[2]*dir[0], 
            cx*x[1] + cy*y2[1] + v[2]*dir[1],
            cy*y2[2] + v[2]*dir[2]) );
    }
};


size_t Tracker::rng_seed;


class TrackWriter {
  public:
    TrackWriter (TrackQueue& queue, const TrackShared& shared, const std::string& output_file, DWI::Tractography::Properties& properties) :
      tracks (queue), S (shared) { writer.create (output_file, properties); }

    ~TrackWriter () { 
      fprintf (stderr, "\r%8u generated, %8u selected    [100%%]\n", writer.total_count, writer.count);
      writer.close(); 
    }

    void execute () { 
      TrackQueue::Reader::Item tck (tracks);
      while (tck.read() && writer.count < S.max_num_tracks && writer.total_count < S.max_num_attempts) {
        writer.append (*tck);
        fprintf (stderr, "\r%8u generated, %8u selected    [%3d%%]", 
            writer.total_count, writer.count, int((100.0*writer.count)/float(S.max_num_tracks)));
      }
    }

  private:
    TrackQueue::Reader tracks;
    const TrackShared& S;
    DWI::Tractography::Writer writer;
};




inline void properties_add (DWI::Tractography::Properties& properties, DWI::Tractography::ROI::Type type, const std::string& spec) {
  using DWI::Tractography::ROI;
  properties.roi.push_back (RefPtr<ROI> (new ROI (type, spec)));
}


EXECUTE {
  using DWI::Tractography::ROI;

  DWI::Tractography::Properties properties;
  properties["step_size"] = "0.2";
  properties["max_dist"] = "200";
  properties["min_dist"] = "10";
  properties["threshold"] = "0.1";
  properties["unidirectional"] = "0";
  properties["sh_precomputed"] = "1";

  std::vector<OptBase> opt = get_options (0); // seed
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties_add (properties, ROI::Seed, (*i)[0].get_string());

  opt = get_options (1); // include
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties_add (properties, ROI::Include, (*i)[0].get_string());

  opt = get_options (2); // exclude
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties_add (properties, ROI::Exclude, (*i)[0].get_string());

  opt = get_options (3); // mask
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties_add (properties, ROI::Mask, (*i)[0].get_string());

  opt = get_options (4); // step
  if (opt.size()) properties["step_size"] = str (opt[0][0].get_float());

  opt = get_options (5); // curvature
  if (opt.size()) properties["min_curv"] = str (opt[0][0].get_float());

  opt = get_options (6); // number
  if (opt.size()) properties["max_num_tracks"] = str (opt[0][0].get_int());

  opt = get_options (7); // maxnum
  if (opt.size()) properties["max_num_attempts"] = str (opt[0][0].get_int());

  opt = get_options (8); // length
  if (opt.size()) properties["max_dist"] = str (opt[0][0].get_float());

  opt = get_options (9); // min_length
  if (opt.size()) properties["min_dist"] = str (opt[0][0].get_float());

  opt = get_options (10); // cutoff
  if (opt.size()) properties["threshold"] = str (opt[0][0].get_float());

  opt = get_options (11); // initcutoff
  if (opt.size()) properties["init_threshold"] = str (opt[0][0].get_float());

  opt = get_options (12); // trials
  if (opt.size()) properties["max_trials"] = str (opt[0][0].get_int());

  opt = get_options (13); // unidirectional
  if (opt.size()) properties["unidirectional"] = "1";

  Point init_dir;
  opt = get_options (14); // initdirection
  if (opt.size()) {
    std::vector<float> V = parse_floats (opt[0][0].get_string());
    if (V.size() != 3) throw Exception (std::string ("invalid initial direction \"") + opt[0][0].get_string() + "\"");
    init_dir[0] = V[0];
    init_dir[1] = V[1];
    init_dir[2] = V[2];
    init_dir.normalise();
    properties["init_direction"] = opt[0][0].get_string();
  }

  opt = get_options (15); // noprecomputed
  if (opt.size()) properties["sh_precomputed"] = "0";


  TrackShared shared (argument[0].get_image(), properties, init_dir);

  Thread::init();
  TrackQueue queue ("track serialiser", 100, TrackAllocator (shared.max_track_size()));

  TrackWriter writer (queue, shared, argument[1].get_string(), properties);
  Tracker tracker (queue, shared);
  Thread::Array<Tracker> tracker_list (tracker);

  Thread::Exec threads (tracker_list, "tracker thread");
  writer.execute();
}
