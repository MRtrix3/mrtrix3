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
  + "resample streamlines according to a specified trajectory";

  ARGUMENTS
  + Argument ("in_tracks",  "the input track file").type_tracks_in()
  + Argument ("out_tracks", "the output resampled tracks").type_tracks_out();

  OPTIONS
    + OptionGroup ("Streamline resampling options")

    + Option ("line",
              "resample tracks at 'num' equidistant locations "
              "along a line between 'start' and 'end' (specified as "
              "comma-separated 3-vectors in scanner coordinates)")
      + Argument ("num").type_integer (2, 10)
      + Argument ("start").type_sequence_float()
      + Argument ("end").type_sequence_float()

    + Option ("arc", "resample tracks at 'num' equidistant locations "
        "along a circular arc specified by points 'start', 'mid' and 'end' "
        "(specified as comma-separated 3-vectors in scanner coordinates)")
      + Argument ("num").type_integer (2, 10)
      + Argument ("start").type_sequence_float()
      + Argument ("mid").type_sequence_float()
      + Argument ("end").type_sequence_float();

    // TODO Resample according to an exemplar streamline
}


typedef float value_type;
typedef Eigen::Vector3f point_type;


class Resampler {
  private:
    class Plane {
      public:
        Plane (const point_type& pos, const point_type& dir) :
            n (dir)
        {
          n.normalize();
          d = n.dot (pos);
        }
        value_type dist (const point_type& pos) { return n.dot (pos) - d; }
      private:
        point_type n;
        value_type d;
    };

    std::vector<Plane> planes;

  public:
    Resampler() :
        nsamples (0), idx_start (0), idx_end (0) { }

    point_type start, mid, end, start_dir, mid_dir, end_dir;
    size_t nsamples, idx_start, idx_end;

    int state (const point_type& p) {
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
        int s = state (tck[i]);
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
      start_dir = (end - start).normalized();
      mid_dir = end_dir = start_dir;
      mid = 0.5f * (start + end);
      for (size_t n = 0; n < nsamples; n++) {
        value_type f = value_type(n) / value_type (nsamples-1);
        planes.push_back (Plane ((1.0f-f) * start + f * end, (1.0f-f) * start_dir + f * end_dir));
      }
    }

    void init (const Eigen::Vector3f& waypoint) {

      mid = 0.5f * (start + end);
      mid_dir = (end - start).normalized();

      Eigen::Matrix3f M;

      M(0,0) = start[0] - waypoint[0];
      M(0,1) = start[1] - waypoint[1];
      M(0,2) = start[2] - waypoint[2];

      M(1,0) = end[0] - waypoint[0];
      M(1,1) = end[1] - waypoint[1];
      M(1,2) = end[2] - waypoint[2];

      point_type n ((start-waypoint).cross (end-waypoint));
      M(2,0) = n[0];
      M(2,1) = n[1];
      M(2,2) = n[2];

      point_type a;
      a[0] = 0.5 * (start+waypoint).dot(start-waypoint);
      a[1] = 0.5 * (end+waypoint).dot(end-waypoint);
      a[2] = start.dot(n);

      point_type c = M.fullPivLu().solve(a);

      point_type x (start-c);
      value_type R = x.norm();
      
      point_type y (waypoint-c);
      y -= y.dot(x)/(x.norm()*y.norm()) * x;
      y *= R / y.norm();

      point_type e (end-c);
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


    void operator() (DWI::Tractography::Streamline<value_type>& tck) {
      assert (tck.size());
      assert (planes.size());
      bool reverse = idx_start > idx_end;
      size_t i = idx_start;
      std::vector<point_type> rtck;

      for (size_t n = 0; n < nsamples; n++) {
        while (i != idx_end) {
          value_type d = planes[n].dist (tck[i]);
          if (d > 0.0) {
            value_type f = d / (d - planes[n].dist (tck[reverse ? i+1 : i-1]));
            rtck.push_back (f*tck[i-1] + (1.0f-f)*tck[i]);
            break;
          }
          reverse ? --i : ++i;
        }
      }
      tck = rtck;
    }

};




inline point_type get_pos (const std::vector<default_type>& s)
{
  if (s.size() != 3)
    throw Exception ("position expected as a comma-seperated list of 3 values");
  return { value_type(s[0]), value_type(s[1]), value_type(s[2]) };
}



void run ()
{
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> read (argument[0], properties);
  Resampler resample;
  DWI::Tractography::Writer<value_type> writer (argument[1], properties);

  auto opt = get_options ("line");
  if (opt.size()) {
    resample.nsamples = opt[0][0];
    resample.start = get_pos (opt[0][1].as_sequence_float());
    resample.end = get_pos (opt[0][2].as_sequence_float());
    resample.init ();
  }

  opt = get_options ("arc");
  if (opt.size()) {
    if (resample.nsamples)
      throw Exception ("Options -line and -arc are mutually exclusive");
    resample.nsamples = opt[0][0];
    resample.start = get_pos (opt[0][1].as_sequence_float());
    resample.end = get_pos (opt[0][3].as_sequence_float());
    resample.init (get_pos (opt[0][2].as_sequence_float()));
  }

  size_t skipped = 0, count = 0;
  auto progress_message = [&](){ return "sampling streamlines (count: " + str(count) + ", skipped: " + str(skipped) + ")"; };
  ProgressBar progress ("sampling streamlines");

  DWI::Tractography::Streamline<value_type> tck;
  while (read (tck)) {
    if (!resample.limits (tck)) { skipped++; continue; }
    resample (tck);
    writer (tck);
    ++count;
    progress.update (progress_message);
  }
  progress.set_text (progress_message());

}

