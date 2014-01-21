/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith and J-Donald Tournier, 2011.

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

#ifndef __dwi_tractography_exec_h__
#define __dwi_tractography_exec_h__


#include "thread/exec.h"
#include "thread/queue.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/method.h"
#include "dwi/tractography/shared.h"

#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"


#define MAX_NUM_SEED_ATTEMPTS 100000



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {



#define UPDATE_INTERVAL 0.0333333 // 30 Hz - most monitors are 60Hz



      class WriteKernel
      {
        public:

          WriteKernel (const SharedBase& shared,
              const std::string& output_file,
              DWI::Tractography::Properties& properties) :
                S (shared),
                writer (output_file, properties),
                next_time (timer.elapsed())
          {
            if (properties.find ("seed_output") != properties.end()) {
              seeds = new std::ofstream (properties["seed_output"].c_str(), std::ios_base::trunc);
              (*seeds) << "#Track_index,Seed_index,Pos_x,Pos_y,Pos_z,\n";
            }
          }

          ~WriteKernel ()
          {
            if (App::log_level > 0)
              fprintf (stderr, "\r%8zu generated, %8zu selected    [100%%]\n", writer.total_count, writer.count);
            if (seeds) {
              (*seeds) << "\n";
              seeds->close();
            }

          }

          bool operator() (const GeneratedTrack& tck)
          {
            if (complete())
              return false;
            if (tck.size() && seeds) {
              const Point<float>& p = tck[tck.get_seed_index()];
              (*seeds) << str(writer.count) << "," << str(tck.get_seed_index()) << "," << str(p[0]) << "," << str(p[1]) << "," << str(p[2]) << ",\n";
            }
            writer.append (tck);
            if (App::log_level > 0 && timer.elapsed() >= next_time) {
              next_time += UPDATE_INTERVAL;
              fprintf (stderr, "\r%8zu generated, %8zu selected    [%3d%%]",
                  writer.total_count, writer.count,
                  (int(100.0 * std::max (writer.total_count/float(S.max_num_attempts), writer.count/float(S.max_num_tracks)))));
            }
            return true;
          }

          bool operator() (const GeneratedTrack& in, Tractography::TrackData<float>& out)
          {
            out.index = writer.count;
            if (!operator() (in))
              return false;
            out = in;
            return true;
          }


          bool complete() const { return (writer.count >= S.max_num_tracks || writer.total_count >= S.max_num_attempts); }


        private:
          const SharedBase& S;
          Writer<value_type> writer;
          Ptr<std::ofstream> seeds;
          Timer timer;
          double next_time;
      };




      template <class Method> class Exec {

        public:

          static void run (const std::string& diff_path, const std::string& destination, DWI::Tractography::Properties& properties)
          {
            typename Method::Shared shared (diff_path, properties);
            WriteKernel writer (shared, destination, properties);
            Exec<Method> tracker (shared);
            Thread::run_queue_threaded_source (tracker, GeneratedTrack(), writer);
          }



          Exec (const typename Method::Shared& shared) :
            S (shared),
            method (shared),
            track_excluded (false),
            track_included (S.properties.include.size(), false) { }


          bool operator() (GeneratedTrack& item) {
            if (!gen_track (item))
              return false;
            if (track_rejected (item))
              item.clear();
            else if (S.downsample > 1)
              downsample_track (item, S.downsample);
            return true;
          }


        private:

          const typename Method::Shared& S;
          Method method;
          bool track_excluded;
          std::vector<bool> track_included;


          term_t iterate ()
          {

            const term_t method_term = (S.rk4 ? next_rk4() : method.next());

            if (method_term)
              return method_term;

            if (S.properties.mask.size() && !S.properties.mask.contains (method.pos))
              return EXIT_MASK;

            if (S.properties.exclude.contains (method.pos))
              return ENTER_EXCLUDE;

            S.properties.include.contains (method.pos, track_included);

            return CONTINUE;

          };


          bool gen_track (GeneratedTrack& tck)
          {
            tck.clear();
            track_excluded = false;
            track_included.assign (track_included.size(), false);
            method.dir.invalidate();

            bool unidirectional = S.unidirectional;

            if (S.properties.seeds.is_finite()) {

              if (!S.properties.seeds.get_seed (method.pos, method.dir))
                return false;
              if (!method.check_seed() || !method.init()) {
                track_excluded = true;
                return true;
              }

            } else {

              for (size_t num_attempts = 0; num_attempts != MAX_NUM_SEED_ATTEMPTS; ++num_attempts) {
                if (S.properties.seeds.get_seed (method.pos, method.dir) && method.check_seed() && method.init())
                  break;
              }
              if (!method.pos.valid()) {
                FAIL ("Failed to find suitable seed point after " + str (MAX_NUM_SEED_ATTEMPTS) + " attempts - aborting");
                return false;
              }

            }

            S.properties.include.contains (method.pos, track_included);

            const Point<value_type> seed_dir (method.dir);
            tck.push_back (method.pos);

            gen_track_unidir (tck);

            if (!track_excluded && !unidirectional) {
              tck.reverse();
              method.pos = tck.back();
              method.dir = -seed_dir;
              method.reverse_track ();
              gen_track_unidir (tck);
            }

            return true;

          }




          void gen_track_unidir (GeneratedTrack& tck)
          {

            term_t termination = CONTINUE;

            do {
              termination = iterate();
              if (term_add_to_tck[termination])
                tck.push_back (method.pos);
              if (!termination && tck.size() >= S.max_num_points)
                termination = LENGTH_EXCEED;
            } while (!termination);
            
            if (termination == ENTER_EXCLUDE) {
              track_excluded = true;
              S.add_rejection (ENTER_EXCLUDE_REGION);
            }

#ifdef DEBUG_TERMINATIONS
            S.add_termination (termination, method.pos);
#else
            S.add_termination (termination);
#endif

          }




          bool track_rejected (const std::vector< Point<float> >& tck)
          {

            if (track_excluded)
              return true;

            if (tck.size() < S.min_num_points) {
              S.add_rejection (TRACK_TOO_SHORT);
              return true;
            }

            if (!traversed_all_include_regions()) {
              S.add_rejection (MISSED_INCLUDE_REGION);
              return true;
            }

            return false;

          }



          bool traversed_all_include_regions ()
          {
            for (size_t n = 0; n < track_included.size(); ++n)
              if (!track_included[n])
                return false;
            return true;
          }




          term_t next_rk4()
          {
            term_t termination = CONTINUE;
            const Point<value_type> init_pos (method.pos);
            const Point<value_type> init_dir (method.dir);
            if ((termination = method.next()))
              return termination;
            const Point<value_type> dir_rk1 (method.dir);
            method.pos = init_pos + (dir_rk1 * (0.5 * S.step_size));
            method.dir = init_dir;
            if ((termination = method.next()))
              return termination;
            const Point<value_type> dir_rk2 (method.dir);
            method.pos = init_pos + (dir_rk2 * (0.5 * S.step_size));
            method.dir = init_dir;
            if ((termination = method.next()))
              return termination;
            const Point<value_type> dir_rk3 (method.dir);
            method.pos = init_pos + (dir_rk3 * S.step_size);
            method.dir = (dir_rk2 + dir_rk3).normalise();
            if ((termination = method.next()))
              return termination;
            const Point<value_type> dir_rk4 (method.dir);
            method.dir = (dir_rk1 + (dir_rk2 * 2.0) + (dir_rk3 * 2.0) + dir_rk4).normalise();
            method.pos = init_pos + (method.dir * S.step_size);
            const Point<value_type> final_pos (method.pos);
            const Point<value_type> final_dir (method.dir);
            if ((termination = method.next()))
              return termination;
            if (dir_rk1.dot (method.dir) < S.cos_max_angle_rk4)
              return HIGH_CURVATURE;
            method.pos = final_pos;
            method.dir = final_dir;
            return CONTINUE;
          }



          void downsample_track (GeneratedTrack& tck, const int factor)
          {
            size_t index_old = factor;
            if (tck.get_seed_index()) {
              index_old = (((tck.get_seed_index() - 1) % factor) + 1);
              tck.set_seed_index (1 + ((tck.get_seed_index() - index_old) / factor));
            }
            size_t index_new = 1;
            while (index_old < tck.size() - 1) {
              tck[index_new++] = tck[index_old];
              index_old += factor;
            }
            tck[index_new] = tck.back();
            tck.resize (index_new + 1);
          }





      };










    }
  }
}

#endif


