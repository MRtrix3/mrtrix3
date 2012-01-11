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

#ifndef __dwi_tractography_exec_h__
#define __dwi_tractography_exec_h__

#include "file/config.h"
#include "image/header.h"
#include "thread/queue.h"
#include "thread/exec.h"
#include "dwi/tractography/shared.h"
#include "dwi/tractography/method.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

/*
      class Allocator 
      {
        public:
          Allocator (size_t number_of_elements) : N (number_of_elements) { }
          Track* alloc () { Track* tck = new Track (); tck->reserve (N); return (tck); }
          void reset (Track* tck) { tck->clear(); }
          void dealloc (Track* tck) { delete tck; }
        private:
          size_t N;
      };
      typedef Thread::Queue<Track,Allocator> Queue;

*/



      class WriteKernel 
      {
        public:
          WriteKernel (const SharedBase& shared, 
              const std::string& output_file, 
              DWI::Tractography::Properties& properties) :
            S (shared) { 
              writer.create (output_file, properties); 
            }

          ~WriteKernel () 
          { 
            fprintf (stderr, "\r%8zu generated, %8zu selected    [100%%]\n", writer.total_count, writer.count);
            writer.close(); 
          }

          bool operator() (const std::vector<Point<value_type> >& tck) {
            if (writer.count < S.max_num_tracks && writer.total_count < S.max_num_attempts) {
              writer.append (tck);
              fprintf (stderr, "\r%8zu generated, %8zu selected    [%3d%%]", 
                  writer.total_count, writer.count, int((100.0*writer.count)/float(S.max_num_tracks)));
              return true;
            }
            return false;
          }

        private:
          const SharedBase& S;
          Writer<value_type> writer;
      };






      template <class Method> class Exec {
        public:
          static void run (Image::Header& source, const std::string& destination, DWI::Tractography::Properties& properties)
          {
            typename Method::Shared shared (source, properties);
            MethodBase::init(); 

            WriteKernel writer (shared, destination, properties);
            Exec<Method> tracker (shared);
            
            Thread::run_queue (tracker, 0, std::vector<Point<value_type> >(), writer, 1);
          }



          bool operator() (std::vector<Point<value_type> >& item) {
            gen_track (item);
            if (item.size() < S.min_num_points || track_excluded || track_is_not_included()) 
              item.clear();
            return true;
          }

        protected:
          const typename Method::Shared& S;
          Method method;
          bool track_excluded;
          std::vector<bool> track_included;

          Exec (const typename Method::Shared& shared) : 
            S (shared), method (shared), track_included (S.properties.include.size()) { } 


          bool track_is_not_included () const 
          { 
            for (size_t n = 0; n < track_included.size(); ++n) 
              if (!track_included[n]) 
                return (true); 
            return (false); 
          }


          void gen_track (std::vector<Point<value_type> >& tck) 
          {
            tck.reserve (S.max_num_points);
            tck.clear();
            track_excluded = false;
            track_included.assign (track_included.size(), false);

            size_t num_attempts = 0;
            do { 
              method.pos = S.properties.seed.sample (method.rng);
              num_attempts++;
              if (num_attempts++ > 10000) 
                throw Exception ("failed to find suitable seed point after 10,000 attempts - aborting");
            } while (!method.init ());

            Point<value_type> seed_dir (method.dir);

            tck.push_back (method.pos);
            while (iterate() && tck.size() < S.max_num_points) tck.push_back (method.pos);
            if (!track_excluded && !S.unidirectional) {
              reverse (tck.begin(), tck.end());
              method.dir[0] = -seed_dir[0];
              method.dir[1] = -seed_dir[1];
              method.dir[2] = -seed_dir[2];
              method.pos = tck.back();
              Exec<Method>::method.reverse_track();
              while (iterate() && tck.size() < S.max_num_points) 
                tck.push_back (method.pos);
            }
          }

          bool iterate () 
          {
            if (!method.next()) 
              return (false);

            if (S.properties.mask.size() && !S.properties.mask.contains (method.pos)) 
              return (false);

            if (S.properties.exclude.contains (method.pos)) { 
              track_excluded = true; 
              return (false); 
            }

            S.properties.include.contains (method.pos, track_included);
            return (true);
          };
      };








    }
  }
}

#endif


