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
#include "math/math.h"
#include "image.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"




using namespace MR;
using namespace App;


void usage ()
{

  DESCRIPTION
  + "sample values of associated image at each location along tracks"

  + "The values are written to the output file as ASCII text, in the same order "
  "as the track vertices, with all values for each track on the same line (space "
  "separated), and individual tracks on separate lines."; 

  ARGUMENTS
  + Argument ("tracks", "the input track file").type_tracks_in()
  + Argument ("image",  "the image to be sampled").type_image_in()
  + Argument ("values", "the output sampled values").type_file_out();

  OPTIONS
    + OptionGroup ("Streamline resampling options")

    + Option ("resample", "resample tracks before sampling from the image, by "
        "resampling the tracks at 'num' equidistant and comparable locations "
        "along the tracks between 'start' and 'end' (specified as "
          "comma-separated 3-vectors in scanner coordinates)")
    +   Argument ("num").type_integer (2)
    +   Argument ("start").type_sequence_float()
    +   Argument ("end").type_sequence_float()

    + Option ("waypoint", "[only used with -resample] together with the start "
        "and end points, this defines an arc of a circle passing through all "
        "points, along which resampling is to occur.")
    +   Argument ("point").type_sequence_float()

    + Option ("locations", "[only used with -resample] output a new track file "
        "with vertices at the locations resampled by the algorithm.")
    +   Argument ("file").type_tracks_out()

    + Option ("warp", "[only used with -resample] specify an image containing "
        "the warp field to the space in which the resampling is to take "
        "place. The tracks will be resampled as per their locations in the "
        "warped space, with sampling itself taking place in the original "
        "space")
    +   Argument ("image").type_image_in();
  
  // TODO add support for SH amplitude along tangent
}


typedef float value_type;


template <class Interp>
class Resampler {
  private:
    class Plane {
      public:
        Plane (const Eigen::Vector3f& pos, const Eigen::Vector3f& dir) : n (dir) { n.normalize(); d = n.dot (pos); }
        value_type dist (const Eigen::Vector3f& pos) { return n.dot (pos) - d; }
      private:
        Eigen::Vector3f n;
        value_type d;
    };

    std::vector<Plane> planes;

  public:
    Interp* warp;
    Eigen::Vector3f start, mid, end, start_dir, mid_dir, end_dir;
    size_t nsamples, idx_start, idx_end;

    Eigen::Vector3f position_of (const Eigen::Vector3f& p) { 
      if (!warp) return p;
      warp->scanner (p);
      Eigen::Vector3f ret;
      if (!(*warp)) return ret;
      for (warp->index(3) = 0; warp->index(3) < 3; warp->index(3)++)
        ret[size_t (warp->index(3))] = warp->value();
      return ret;
    }

    int state (const Eigen::Vector3f& p) {
      bool after_start = start_dir.dot (p - start) >= 0;
      bool after_mid = mid_dir.dot (p - mid) > 0.0;
      bool after_end = end_dir.dot (p - end) >= 0.0;
      if (!after_start && !after_mid) return -1; // before start;
      if (after_start && !after_mid) return 0; // after start;
      if (after_mid && !after_end) return 1; // before end
      return 2; // after end
    }

    int limits (const DWI::Tractography::Streamline<value_type>& tck) {
      idx_start = idx_end = 0;
      size_t a (0), b (0);

      int prev_s = -1;
      for (size_t i = 0; i < tck.size(); ++i) {
        int s = state (position_of (tck[i]));
        if (i) {
          if (prev_s == -1 && s == 0) a = i-1;
          if (prev_s == 0 && s == -1) a = i;
          if (prev_s == 1 && s == 2) b = i;
          if (prev_s == 2 && s == 1) b = i-1;

          if (a && b) {
            if (b - a > idx_end - idx_start) {
              idx_start = a;
              idx_end = b;
            }
            a = b = 0;
          }
        }
        prev_s = s;
      }

      ++idx_end;

      return (idx_start && idx_end);
    }

    void init () {
      for (size_t n = 0; n < nsamples; n++) {
        value_type f = value_type(n) / value_type (nsamples-1);
        planes.push_back (Plane ((1.0f-f) * start + f * end, (1.0f-f) * start_dir + f * end_dir));
      }
    }

    void init (const Eigen::Vector3f& waypoint) {

      Eigen::Matrix3f M;

      M(0,0) = start[0] - waypoint[0];
      M(0,1) = start[1] - waypoint[1];
      M(0,2) = start[2] - waypoint[2];

      M(1,0) = end[0] - waypoint[0];
      M(1,1) = end[1] - waypoint[1];
      M(1,2) = end[2] - waypoint[2];

      Eigen::Vector3f n ((start-waypoint).cross (end-waypoint));
      M(2,0) = n[0];
      M(2,1) = n[1];
      M(2,2) = n[2];

      Eigen::Vector3f a;
      a[0] = 0.5 * (start+waypoint).dot(start-waypoint);
      a[1] = 0.5 * (end+waypoint).dot(end-waypoint);
      a[2] = start.dot(n);

      Eigen::Vector3f c = M.fullPivLu().solve(a);

      Eigen::Vector3f x (start-c);
      value_type R = x.norm();
      
      Eigen::Vector3f y (waypoint-c);
      y -= y.dot(x)/(x.norm()*y.norm()) * x;
      y *= R / y.norm();

      Eigen::Vector3f e (end-c);
      value_type ex (x.dot (e)), ey (y.dot (e));

      value_type angle = std::atan2 (ey, ex);
      if (angle < 0.0) angle += 2.0 * Math::pi;

      for (size_t n = 0; n < nsamples; n++) {
        value_type f = angle * value_type(n) / value_type (nsamples-1);
        planes.push_back (Plane (c + x*cos(f) + y*sin(f), y*cos(f) - x*sin(f)));
      }

      start_dir = y;
      end_dir = y*cos(angle) - x*sin(angle);
    }


    void operator() (std::vector<Eigen::Vector3f>& tck) {
      assert (tck.size());
      assert (planes.size());
      bool reverse = idx_start > idx_end;
      size_t i = idx_start;
      std::vector<Eigen::Vector3f> rtck;

      for (size_t n = 0; n < nsamples; n++) {
        while (i != idx_end) {
          value_type d = planes[n].dist (position_of (tck[i]));
          if (d > 0.0) {
            value_type f = d / (d - planes[n].dist (position_of (tck[reverse ? i+1 : i-1])));
            rtck.push_back (f*tck[i-1] + (1.0f-f)*tck[i]);
            break;
          }
          reverse ? --i : ++i;
        }
      }
      tck = rtck;
    }

};


template <class Streamline, class Interp>
inline void sample (File::OFStream& out, Interp& interp, const Streamline& tck) 
{
  for (auto& pos : tck) {
    interp.scanner (pos);
    out << interp.value() << " ";
  }
  out << "\n";
}

inline Eigen::Vector3f get_pos (const std::vector<default_type>& s)
{
  if (s.size() != 3)
    throw Exception ("position expected as a comma-seperated list of 3 values");
  return { float(s[0]), float(s[1]), float(s[2]) };
}



void run ()
{
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> read (argument[0], properties);

  auto image = Image<value_type>::open (argument[1]);
  auto interp = Interp::Linear< decltype(image) > (image);

  File::OFStream out (argument[2]);

  std::unique_ptr< Interp::Linear< Image<value_type> > > warp;

  Resampler<decltype(warp)::element_type> resample;
  std::unique_ptr<DWI::Tractography::Writer<value_type>> writer;

  auto opt = get_options ("resample");
  bool resampling = opt.size();
  if (resampling) {
    resample.nsamples = opt[0][0];
    resample.start = get_pos (opt[0][1].as_sequence_float());
    resample.end = get_pos (opt[0][2].as_sequence_float());

    resample.start_dir = resample.end - resample.start;
    resample.start_dir.normalize();
    resample.mid_dir = resample.end_dir = resample.start_dir;
    resample.mid = 0.5f * (resample.start + resample.end);

    opt = get_options ("warp");
    if (opt.size()) {
      warp.reset (new Interp::Linear< Image<value_type> >(Image<value_type>::open(opt[0][0]) ));
      if (warp->ndim() < 4)
        throw Exception ("warp image should contain at least 4 dimensions");
      if (warp->size(3) < 3)
        throw Exception ("4th dimension of warp image should have length 3");
      resample.warp = warp.get();
    }

    opt = get_options ("waypoint");
    if (opt.size()) 
      resample.init (get_pos (opt[0][0].as_sequence_float()));
    else 
      resample.init ();

    opt = get_options ("locations");
    if (opt.size())
      writer.reset (new DWI::Tractography::Writer<value_type> (opt[0][0], properties));
  }

  size_t skipped = 0, count = 0;
  auto progress_message = [&](){ return "sampling streamlines (count: " + str(count) + ", skipped: " + str(skipped) + ")"; };
  ProgressBar progress ("sampling streamlines");

  DWI::Tractography::Streamline<value_type> tck;
  while (read (tck)) {
    if (resampling) {
      if (!resample.limits (tck)) { skipped++; continue; }
      resample (tck);

      if (writer) 
        (*writer) (tck);
    }
    sample (out, interp, tck);

    ++count;
    progress.update (progress_message);
  }
  progress.set_text (progress_message());
}

