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


    29-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fix init_direction handling

    18-12-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * added multithreading capability

    21-03-2009 J-Donald Tournier <d.tournier@brain.org.au>
    * fix minor bug that caused tracking to hang on 64-bit machines

*/

#include <queue>

#include "app.h"
#include "thread.h"
#include "file/config.h"
#include "image/interp.h"
#include "math/matrix.h"
#include "math/least_squares.h"
#include "point.h"
#include "dwi/gradient.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/tracker/dt_stream.h"
#include "dwi/tractography/tracker/sd_stream.h"
#include "dwi/tractography/tracker/sd_prob.h"


using namespace std; 
using namespace MR; 
using namespace MR::DWI::Tractography; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "perform streamlines tracking.",
  NULL
};

const char* type_choices[] = { "DT_STREAM", "DT_PROB", "SD_STREAM", "SD_PROB", NULL };

ARGUMENTS = {
  Argument ("type", "tracking type", "the type of streamlines tracking to be performed. Allowed types are DT_STREAM, SD_STREAM, SD_PROB.").type_choice (type_choices),
  Argument ("source", "source image", "the image containing the source data. The type of data required depends on the type of tracking as set in the preceeding argument. For DT methods, the base DWI are needed. For SD methods, the SH harmonic coefficients of the FOD are needed.").type_image_in(),
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

  Option ("grad", "DW gradient scheme", "specify the diffusion encoding scheme (may be required for DT_STREAM, ignored otherwise).")
    .append (Argument ("scheme", "gradient file", "the DW gradient file.").type_file()),

  Option ("number", "number of tracks", "set the number of tracks to calculate (default is 100 for *_STREAM methods, 1000 for *_PROB methods).")
    .append (Argument ("tracks", "number of tracks", "the number of tracks.").type_integer (1, INT_MAX, 1)),

  Option ("length", "track length", "set the maximum length of any track.")
    .append (Argument ("value", "track distance", "the maximum length to use in mm (default is 200 mm).").type_float (1e-2, 1e6, 200.0)),

  Option ("minlength", "minimum track length", "set the minimum length of any track.")
    .append (Argument ("value", "track distance", "the minimum length to use in mm (default is 10 mm).").type_float (1e-2, 1e6, 10.0)),

  Option ("cutoff", "cutoff threshold", "set the FA or FOD amplitude cutoff for terminating tracks (default is 0.1).")
    .append (Argument ("value", "value", "the cutoff to use.").type_float (0, 1e6, 0.1)),

  Option ("initcutoff", "intial cutoff threshold", "set the minimum FA or FOD amplitude for initiating tracks (default is twice the normal cutoff).")
    .append (Argument ("value", "value", "the initial cutoff to use.").type_float (1e-6, 1e6, 0.1)),

  Option ("trials", "number of trials", "set the maximum number of sampling trials at each point (only used for probabilistic tracking).")
    .append (Argument ("number", "number", "the number of trials.").type_integer(1, 10000, 50)),

  Option ("unidirectional", "unidirectional", "track from the seed point in one direction only (default is to track in both directions)."),

  Option ("initdirection", "initial direction", "specify an initial direction for the tracking.")
    .append (Argument ("dir", "direction", "the vector specifying the initial direction.").type_sequence_float()),

  Option ("noprecomputed", "no precomputation", "do NOT pre-compute legendre polynomial values. Warning: this will slow down the algorithm by a factor of approximately 4."),

  Option::End
};




class Threader : public Thread {
  public:
    Threader (int type_index, Image::Object& source, const std::string& output_file, Properties& properties, Point init_direction, Ptr<Math::Matrix<float> >& grad) :
      init_dir (init_direction),
      currently_running (0)
    {
      source.map();
      trackers = new Tracker::Base* [num_threads];

      switch (type_index) {
        case 0: 
          {
            binv = (grad ? *grad : source.header().DW_scheme);
            info ("found " + str(binv.rows()) + "x" + str(binv.columns()) + " diffusion-weighted encoding");
            DWI::normalise_grad (binv);
            Math::Matrix<float> bmat;
            DWI::grad2bmatrix (bmat, binv);
            Math::pinv (binv, bmat);
            for (int n = 0; n < num_threads; n++) 
              trackers[n] = new Tracker::DTStream (source, properties, binv);
          }
          break;
        case 2: 
          for (int n = 0; n < num_threads; n++) 
            trackers[n] = new Tracker::SDStream (source, properties);
          break;
        case 3: 
          for (int n = 0; n < num_threads; n++) 
            trackers[n] = new Tracker::SDProb (source, properties);
          break;
        default: throw Exception ("tracking method requested is not implemented yet!");
      }

      max_num_tracks = to<int> (properties["max_num_tracks"]);
      unidirectional = to<int> (properties["unidirectional"]);
      min_size = round (to<float> (properties["min_dist"]) / to<float> (properties["step_size"]));

      writer.create (output_file, properties);

    }

    ~Threader () { for (int n = 0; n < num_threads; n++) delete trackers[n]; delete [] trackers; }

    void run () {

      currently_running = num_threads;
      uint rng_seed = time (NULL);
      for (int n = 0; n < num_threads; n++) 
        trackers[n]->set_rng_seed (rng_seed + n);

      num_threads++;
      Thread::run();
      num_threads--;
    }
    



  protected:
    Math::Matrix<float> binv;
    const Point init_dir;
    uint max_num_tracks, min_size;
    int  currently_running, num_threads;
    bool unidirectional;
    Thread::Cond data_ready;
    Thread::Mutex mutex;

    Tracker::Base** trackers;
    Writer writer;

    std::queue<std::vector<Point>*> fifo;

    void append (std::vector<Point>*& tck, bool accept)
    {
      mutex.lock();
      if (accept) {
        fifo.push (tck);
        tck = NULL;
        data_ready.signal();
      }
      else writer.total_count++;
      mutex.unlock();
    }

    void write ()
    {
      std::vector<Point>* tck;
      do {
        mutex.lock();
        while (currently_running > 0 && fifo.empty()) data_ready.wait (mutex);
        if (fifo.size()) {
          tck = fifo.front();
          fifo.pop();
        }
        else tck = NULL;
        mutex.unlock();

        if (tck) {
          if (writer.count < max_num_tracks) {
            writer.append (*tck);
            writer.total_count++;
            fprintf (stderr, "\r%8u generated, %8u selected    [%3d%%]", 
                writer.total_count, writer.count, (int) ((100.0*writer.count)/(float) max_num_tracks));
          }
          delete tck;
        }
      } while (currently_running > 0);

      fprintf (stderr, "\r%8u generated, %8u selected    [100%%]\n", writer.total_count, writer.count);
      writer.close ();
    }



    void execute (int thread_number) 
    {
      if (thread_number == 0) {
        write ();
        return;
      }

      Tracker::Base* tracker = trackers[thread_number-1];

      std::vector<Point>* tck = NULL;
      while (writer.count < max_num_tracks) {

        tracker->new_seed (init_dir);
        Point seed_dir (tracker->direction());

        if (!tck) tck = new std::vector<Point>;
        else tck->clear();
        tck->push_back (tracker->position());

        while (tracker->next()) tck->push_back (tracker->position());
        if (!tracker->track_excluded() && !unidirectional) {
          reverse (tck->begin(), tck->end());
          seed_dir[0] = -seed_dir[0];
          seed_dir[1] = -seed_dir[1];
          seed_dir[2] = -seed_dir[2];
          tracker->set (tck->back(), seed_dir);
          while (tracker->next()) tck->push_back (tracker->position());
        }

        append (tck, (!tracker->track_excluded() && tracker->track_included() && tck->size() > min_size));
      }

      if (tck) delete tck;

      mutex.lock();
      currently_running--;
      data_ready.signal();
      mutex.unlock();
    }

};








EXECUTE {

  Properties properties;
  properties["step_size"] = "0.2";
  properties["max_dist"] = "200";
  properties["min_dist"] = "10";
  properties["threshold"] = "0.1";
  properties["unidirectional"] = "0";
  properties["sh_precomputed"] = "1";

  std::vector<OptBase> opt = get_options (0); // seed
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.roi.push_back (RefPtr<ROI> (new ROI (ROI::Seed, (*i)[0].get_string())));

  opt = get_options (1); // include
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.roi.push_back (RefPtr<ROI> (new ROI (ROI::Include, (*i)[0].get_string())));

  opt = get_options (2); // exclude
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.roi.push_back (RefPtr<ROI> (new ROI (ROI::Exclude, (*i)[0].get_string())));

  opt = get_options (3); // mask
  for (std::vector<OptBase>::iterator i = opt.begin(); i != opt.end(); ++i)
    properties.roi.push_back (RefPtr<ROI> (new ROI (ROI::Mask, (*i)[0].get_string())));

  opt = get_options (4); // step
  if (opt.size()) properties["step_size"] = str (opt[0][0].get_float());

  opt = get_options (5); // curvature
  if (opt.size()) properties["min_curv"] = str (opt[0][0].get_float());

  opt = get_options (6); // grad
  Ptr<Math::Matrix<float> > grad;
  if (opt.size()) { 
    grad = new Math::Matrix<float> ;
    grad->load (opt[0][0].get_string());
  }

  opt = get_options (7); // number
  if (opt.size()) properties["max_num_tracks"] = str (opt[0][0].get_int());

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

  Threader thread (argument[0].get_int(), *argument[1].get_image(), argument[2].get_string(), properties, init_dir, grad);
  thread.run();
}
