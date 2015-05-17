/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier 2014.

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
#include "math/math.h"

#include "image/buffer_preload.h"
#include "image/voxel.h"

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
  + Argument ("tracks",  "the input track file").type_file_in()
  + Argument ("image",  "the image to be sampled").type_image_in()
  + Argument ("values", "the output sampled values").type_file_out();

  OPTIONS
    + OptionGroup ("Streamline resampling options")

    + Option ("resample", "resample tracks before sampling from the image, by "
        "resampling the tracks at 'num' equidistant and comparable locations "
        "along the tracks between 'start' and 'end' (specified as "
          "comma-separated 3-vectors in scanner coordinates)")
    +   Argument ("num").type_integer (2, 10)
    +   Argument ("start").type_sequence_float()
    +   Argument ("end").type_sequence_float()

    + Option ("waypoint", "[only used with -resample] together with the start "
        "and end points, this defines an arc of a circle passing through all "
        "points, along which resampling is to occur.")
    +   Argument ("point").type_sequence_float()

    + Option ("warp", "[only used with -resample] specify an image containing "
        "the warp field to the space in which the resampling is to take "
        "place. The tracks will be resampled as per their locations in the "
        "warped space, with sampling itself taking place in the original "
        "space")
    +   Argument ("image").type_image_in();
  
  // TODO add support for SH amplitude along tangent
};





template <class Interp>
class Resampler {
  private:
    class Plane {
      public:
        Plane (const Eigen::Vector3f& pos, const Eigen::Vector3f& dir) : n (dir) { n.normalise(); d = n.dot (pos); }
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
      for ((*warp)[3] = 0; (*warp)[3] < 3; ++(*warp)[3])
        ret[size_t ((*warp)[3])] = warp->value();
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

      Math::Matrix<value_type> M (3,3);

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

      Math::Vector<value_type> centre, a(3);
      a[0] = 0.5 * (start+waypoint).dot(start-waypoint);
      a[1] = 0.5 * (end+waypoint).dot(end-waypoint);
      a[2] = start.dot(n);
      // TODO check whether this line is actually needed at all:
      Math::mult (centre, M, a);

      Math::LU::solve (a, M);
      Eigen::Vector3f c (a[0], a[1], a[2]);

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

inline Eigen::Vector3f get_pos (const std::vector<float>& s)
{
  if (s.size() != 3)
    throw Exception ("position expected as a comma-seperated list of 3 values");
  return { s[0], s[1], s[2] };
}



void run ()
{
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> read (argument[0], properties);

  Image::BufferPreload<value_type> image_buffer (argument[1]);
  auto vox = image_buffer.voxel();
  interp = Interp::make <Interp::Linear> (vox);

  File::OFStream out (argument[2]);

  std::unique_ptr<Image::BufferPreload<value_type>> warp_buffer;
  std::unique_ptr<decltype(warp_buffer)::element_type::voxel_type> warp_vox;
  std::unique_ptr<Image::Interp::Linear<decltype(warp_vox)::element_type>> warp;

  Resampler<decltype(warp)::element_type> resample;

  auto opt = get_options ("resample");
  bool resampling = opt.size();
  if (resampling) {
    resample.nsamples = opt[0][0];
    resample.start = get_pos (opt[0][1]);
    resample.end = get_pos (opt[0][2]);

    resample.start_dir = resample.end - resample.start;
    resample.start_dir.normalise();
    resample.mid_dir = resample.end_dir = resample.start_dir;
    resample.mid = 0.5f * (resample.start + resample.end);

    opt = get_options ("warp");
    if (opt.size()) {
      warp_buffer.reset (new decltype(warp_buffer)::element_type (opt[0][0]));
      if (warp_buffer->ndim() < 4) 
        throw Exception ("warp image should contain at least 4 dimensions");
      if (warp_buffer->dim(3) < 3) 
        throw Exception ("4th dimension of warp image should have length 3");
      warp_vox.reset (new decltype(warp_vox)::element_type (*warp_buffer));
      warp.reset (new decltype(warp)::element_type (*warp_vox));
      resample.warp = warp.get();
    }

    opt = get_options ("waypoint");
    if (opt.size()) 
      resample.init (get_pos (opt[0][0]));
    else 
      resample.init ();
  }

  size_t skipped = 0, count = 0;
  auto progress_message = [&](){ return "sampling streamlines (count: " + str(count) + ", skipped: " + str(skipped) + ")..."; };
  ProgressBar progress ("sampling streamlines...");

  DWI::Tractography::Streamline<value_type> tck;
  while (read (tck)) {
    if (resampling) {
      if (!resample.limits (tck)) { skipped++; continue; }
      resample (tck);
    }
    sample (out, interp, tck);

    ++count;
    progress.update (progress_message);
  }
  progress.set_text (progress_message());
}

