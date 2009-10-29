/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 25/10/09.

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

ARGUMENTS = {
  Argument ("FOD", "FOD image", "the image containing the FOD data, expressed in spherical harmonics.").type_image_in(),
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


namespace Track {

  typedef std::vector<Point> Line;

  class Allocator {
    public:
      Allocator (size_t number_of_elements) : N (number_of_elements) { }
      Line* alloc () { Line* tck = new Line (); tck->reserve (N); return (tck); }
      void reset (Line* tck) { tck->clear(); }
      void dealloc (Line* tck) { delete tck; }
    private:
      size_t N;
  };

  typedef Thread::Queue<Line,Allocator> Queue;



  class SharedBase {
    public:
      SharedBase (const Image::Header& source_header, DWI::Tractography::Properties& property_set) :
        source (source_header),
        properties (property_set), 
        max_num_tracks (1000),
        min_curv (1.0),
        step_size (0.1),
        threshold (0.1), 
        unidirectional (false) {
          float max_dist = 200.0;
          float min_dist = 10.0;

          properties["source"] = source.name();

          properties.set (step_size, "step_size");
          properties.set (threshold, "threshold");
          properties.set (min_curv, "min_curv");
          properties.set (unidirectional, "unidirectional");
          properties.set (max_num_tracks, "max_num_tracks");
          properties.set (max_dist, "max_dist");
          properties.set (min_dist, "min_dist");

          init_threshold = 2.0*threshold;
          properties.set (init_threshold, "init_threshold");

          max_num_attempts = 100 * max_num_tracks;
          properties.set (max_num_attempts, "max_num_attempts");

          if (properties["initdirection"].size()) {
            std::vector<float> V = parse_floats (properties["initdirection"]);
            if (V.size() != 3) throw Exception (std::string ("invalid initial direction \"") + properties["initdirection"] + "\"");
            init_dir[0] = V[0];
            init_dir[1] = V[1];
            init_dir[2] = V[2];
            init_dir.normalise();
          }

          max_num_points = round (max_dist/step_size);
          min_num_points = round (min_dist/step_size);
        }

      const Image::Header& source;
      DWI::Tractography::Properties& properties;
      Point init_dir;
      size_t max_num_tracks, max_num_attempts, min_num_points, max_num_points;
      float min_curv, step_size, threshold, init_threshold;
      bool unidirectional;

      static float curv2angle (float step_size_, float curv)     { return (2.0 * asin (step_size_ / (2.0 * curv))); }
  };




  class MethodBase {
    public:
      MethodBase (const Image::Header& source_header) : 
        source (source_header), interp (source), rng (rng_seed++), values (new float [source.dim(3)]) { }

      MethodBase (const MethodBase& base) : 
        source (base.source), interp (source), rng (rng_seed++), values (new float [source.dim(3)]) { }

      ~MethodBase () { delete values; }

      Image::Voxel<float> source;
      DataSet::Interp<Image::Voxel<float> > interp;
      Math::RNG rng;
      Point pos, dir;
      float* values;

      bool get_data () {
        interp.R (pos); 
        if (!interp) return (false);
        for (source.pos(3,0); source.pos(3) < source.dim(3); source.move(3,1)) 
          values[source.pos(3)] = interp.value();
        return (!isnan (values[0]));
      }

      static void init () { rng_seed = time (NULL); }

    private:
      static size_t rng_seed;
  };

  size_t MethodBase::rng_seed;




  template <class Method> class Exec {
    public:
      Exec (const typename Method::Shared& shared, Queue& queue) : 
        S (shared), method (shared), writer (queue), track_included (S.properties.include.size()) { } 

      void execute () { 
        Queue::Writer::Item item (writer);
        do {
          gen_track (*item);
          if (item->size() < S.min_num_points || track_excluded || track_is_not_included()) item->clear();
        } while (item.write());
      }


    private:
      const typename Method::Shared& S;
      Method method;
      Queue::Writer writer;
      bool track_excluded;
      std::vector<bool> track_included;

      bool track_is_not_included () const { 
        for (size_t n = 0; n < track_included.size(); ++n) 
          if (!track_included[n]) return (true); 
        return (false); 
      }


      void gen_track (Line& tck) {
        track_excluded = false;
        track_included.assign (track_included.size(), false);

        size_t num_attempts = 0;
        do { 
          method.pos = S.properties.seed.sample (method.rng);
          num_attempts++;
          if (num_attempts++ > 10000) throw Exception ("failed to find suitable seed point after 10,000 attempts - aborting");
        } while (!method.init ());
        Point seed_dir (method.dir);

        tck.push_back (method.pos);
        while (iterate() && tck.size() < S.max_num_points) tck.push_back (method.pos);
        if (!track_excluded && !S.unidirectional) {
          reverse (tck.begin(), tck.end());
          method.dir[0] = -seed_dir[0];
          method.dir[1] = -seed_dir[1];
          method.dir[2] = -seed_dir[2];
          method.pos = tck.back();
          while (iterate() && tck.size() < S.max_num_points) tck.push_back (method.pos);
        }
      }

      bool iterate () {
        if (!method.next()) return (false);
        if (S.properties.mask.size() && S.properties.mask.contains (method.pos) == SIZE_MAX) return (false);
        if (S.properties.exclude.contains (method.pos) < SIZE_MAX) { track_excluded = true; return (false); }
        size_t n = S.properties.include.contains (method.pos);
        if (n < SIZE_MAX) track_included[n] = true;
        return (true);
      };
  };








  class Writer {
    public:
      Writer (Queue& queue, const SharedBase& shared, const std::string& output_file, DWI::Tractography::Properties& properties) :
        tracks (queue), S (shared) { writer.create (output_file, properties); }

      ~Writer () { 
        fprintf (stderr, "\r%8u generated, %8u selected    [100%%]\n", writer.total_count, writer.count);
        writer.close(); 
      }

      void execute () { 
        Queue::Reader::Item tck (tracks);
        while (tck.read() && writer.count < S.max_num_tracks && writer.total_count < S.max_num_attempts) {
          writer.append (*tck);
          fprintf (stderr, "\r%8u generated, %8u selected    [%3d%%]", 
              writer.total_count, writer.count, int((100.0*writer.count)/float(S.max_num_tracks)));
        }
      }

    private:
      Queue::Reader tracks;
      const SharedBase& S;
      DWI::Tractography::Writer writer;
  };





  template <class Method> void run (const Image::Header& source, const std::string& destination, DWI::Tractography::Properties& properties) 
  {
    typename Method::Shared shared (source, properties);
    MethodBase::init(); 

    Queue queue ("writer", 100, Allocator (shared.max_num_points));

    Writer writer (queue, shared, destination, properties);

    Exec<Method> tracker (shared, queue);
    Thread::Array<Exec<Method> > tracker_list (tracker);

    Thread::Exec threads (tracker_list, "tracker");
    writer.execute();
  }




  class MethodFOD : public MethodBase {
    public:
      class Shared : public SharedBase {
        public:
          Shared (const Image::Header& source, DWI::Tractography::Properties& property_set) :
            SharedBase (source, property_set),
            lmax (Math::SH::LforN (source.dim(3))), 
            max_trials (100),
            dist_spread (curv2angle (step_size, min_curv)) {
              properties["method"] = "FOD_PROB";
              properties.set (lmax, "lmax");
              properties.set (max_trials, "max_trials");
              bool precomputed = true;
              properties.set (precomputed, "sh_precomputed");
              if (precomputed) precomputer.init (lmax);
          }

          size_t lmax, max_trials;
          float dist_spread;
          Math::SH::PrecomputedAL<float> precomputer;
      };

      MethodFOD (const Shared& shared) : MethodBase (shared.source), S (shared) { } 

      bool init () { 
        if (!get_data ()) return (false);

        if (!S.init_dir) {
          for (size_t n = 0; n < S.max_trials; n++) {
            dir.set (rng.normal(), rng.normal(), rng.normal());
            dir.normalise();
            float val = FOD (dir);
            if (!isnan (val)) if (val > S.init_threshold) return (true);
          }   
        }   
        else {
          dir = S.init_dir;
          float val = FOD (dir);
          if (finite (val)) if (val > S.init_threshold) return (true);
        }   

        return (false);
      }   

      bool next () {
        if (!get_data ()) return (false);

        float max_val = prev_FOD_val;
        float max_val_actual = 0.0;
        for (int n = 0; n < 50; n++) {
          Point new_dir = rand_dir();
          float val = FOD (new_dir);
          if (val > max_val_actual) max_val_actual = val;
          if (val > max_val) max_val = val;
        }
        prev_FOD_val = max_val;

        if (isnan (max_val)) return (false);
        if (max_val < S.threshold) return (false);
        max_val *= 1.5;

        size_t nmax = max_val_actual > S.threshold ? 10000 : S.max_trials;
        for (size_t n = 0; n < nmax; n++) {
          Point new_dir = rand_dir();
          float val = FOD (new_dir);
          if (val > S.threshold) {
            if (val > max_val) info ("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str (max_val) + ")");
            if (rng.uniform() < val/max_val) {
              dir = new_dir;
              pos += S.step_size * dir;
              return (true);
            }
          }
        }

        return (false);
      }

    private:
      const Shared& S;
      float prev_FOD_val;

      float FOD (const Point& d) const {
        return (S.precomputer ?  S.precomputer.value (values, d) : Math::SH::value (values, d, S.lmax)); }

      Point rand_dir () {
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

} // namespace Track



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
    properties.seed.add (ROI (std::string ((*i)[0].get_string())));

  opt = get_options (1); // include
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.include.add (ROI (std::string ((*i)[0].get_string())));

  opt = get_options (2); // exclude
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.exclude.add (ROI (std::string ((*i)[0].get_string())));

  opt = get_options (3); // mask
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.mask.add (ROI (std::string ((*i)[0].get_string())));

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

  opt = get_options (14); // initdirection
  if (opt.size()) {
    properties["init_direction"] = opt[0][0].get_string();
  }

  opt = get_options (15); // noprecomputed
  if (opt.size()) properties["sh_precomputed"] = "0";

  Track::run<Track::MethodFOD> (argument[0].get_image(), argument[1].get_string(), properties);
}
