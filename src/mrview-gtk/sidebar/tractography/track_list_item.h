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

    
    14-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fixed Track::index() for use on 64 systems.
      now uses size_t rather than uint in pointer arithmetic

    15-12-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * a few bug fixes + memory performance improvements for the depth blend option
    
*/

#ifndef __mrview_sidebar_tractography_tracklist_item_h__
#define __mrview_sidebar_tractography_tracklist_item_h__

#include <list>
#include <GL/gl.h>
#include <glib/gstdio.h>

#include "ptr.h"
#include "point.h"
#include "math/vector.h"
#include "dwi/tractography/properties.h"

#define TRACK_ALLOCATOR_SLAB_SIZE 0x100000U
#define TRANSPARENCY_EXPONENT 4.9 

namespace MR {
  namespace Viewer {
    namespace SideBar {

      class TrackListItem 
      {
        public:
          class Allocator;

          class Track {
            public:

              class Point {
                public:
                  float   pos[3];
                  GLubyte C[4];

                  void set_pos (const MR::Point& p) { pos[0] = p[0]; pos[1] = p[1]; pos[2] = p[2]; }
                  void set_colour (const MR::Point& dir) { C[0] = abs((int) (255*dir[0])); C[1] = abs((int) (255*dir[1])); C[2] = abs((int) (255*dir[2])); }
                  void set_colour (GLubyte c[3]) { C[0] = c[0]; C[1] = c[1]; C[2] = c[2]; }
                  size_t index () const { return (((size_t) this)/sizeof(Point)); }

                  bool operator< (const Point& b) { return ((pos[0]-b.pos[0])*normal[0] + (pos[1]-b.pos[1])*normal[1] + (pos[2]-b.pos[2])*normal[2] < 0.0); }
                  static MR::Point normal;
              };


              Track (Allocator& alloc, uint num);

              size_t          size () const               { return (num_p); }
              const Point&   operator[] (int n) const    { return (data[n]); }
              Point&         operator[] (int n)          { return (data[n]); }

              static const int stride = sizeof (Point);

            protected:
              Point* data;
              uint  num_p;
          };



          class Allocator {
            public:
              Allocator () : next (NULL), end (NULL) { }
              ~Allocator () { clear(); }

              Track::Point* operator () (uint size) { 
                size *= sizeof(Track::Point);
                assert (size <= TRACK_ALLOCATOR_SLAB_SIZE);
                if (next + size > end) new_block();
                Track::Point* p = (Track::Point*) next;
                next += size;
                return (p);
              }

              void clear () { for (std::list<uint8_t*>::iterator i = blocks.begin(); i != blocks.end(); ++i) delete [] *i; blocks.clear(); next = end = NULL; }

            private:
              std::list<uint8_t*> blocks;
              uint8_t* next;
              uint8_t* end;

              void new_block () {
                next = new uint8_t [TRACK_ALLOCATOR_SLAB_SIZE];
                end = next + TRACK_ALLOCATOR_SLAB_SIZE;
                blocks.push_back (next); 
                if (uint rem = (next - (uint8_t*) NULL ) % sizeof(Track::Point)) next += sizeof(Track::Point)-rem;
              }
          };


          TrackListItem () { colour_by_dir = true; alpha = 1.0; }


          std::string file;
          time_t mtime;
          std::list<Track>  tracks;
          DWI::Tractography::Properties properties;
          GLubyte colour[3];
          bool  colour_by_dir, colour_by_dir_previous;
          float alpha, alpha_previous;

          void update_RGBA (bool force = false) 
          {
            if (force || colour_by_dir != colour_by_dir_previous || alpha != alpha_previous) {
              GLubyte A = (GLubyte) (255.0*get_alpha());
              for (std::list<Track>::iterator i = tracks.begin(); i != tracks.end(); ++i) {
                Track& tck (*i);

                if (colour_by_dir) {
                  GLubyte default_colour[] = { 255, 255, 255 };
                  tck[0].set_colour (default_colour);
                  tck[0].C[3] = A;
                  if (tck.size() > 1) {
                    size_t n;
                    Point dir (tck[1].pos[0]- tck[0].pos[0], tck[1].pos[1]- tck[0].pos[1], tck[1].pos[2]- tck[0].pos[2]);
                    tck[0].set_colour (dir.normalise());
                    for (n = 1; n < tck.size()-1; n++) {
                      dir.set (tck[n+1].pos[0]- tck[n-1].pos[0], tck[n+1].pos[1]- tck[n-1].pos[1], tck[n+1].pos[2]- tck[n-1].pos[2]);
                      tck[n].set_colour (dir.normalise());
                      tck[n].C[3] = A;
                    }
                    dir.set (tck[n].pos[0]- tck[n-1].pos[0], tck[n].pos[1]- tck[n-1].pos[1], tck[n].pos[2]- tck[n-1].pos[2]);
                    tck[n].set_colour (dir.normalise());
                    tck[n].C[3] = A;
                  }
                }
                else {
                  for (size_t n = 0; n < tck.size(); n++) {
                    tck[n].set_colour (colour);
                    tck[n].C[3] = A;
                  }
                }
              }
              colour_by_dir_previous = colour_by_dir;
              alpha_previous = alpha;
            }
          }

          GLclampf get_alpha () const { return (exp (TRANSPARENCY_EXPONENT * ( alpha - 1.0))); }

          void load (const std::string& filename);
          bool refresh ()
          {
            struct stat S;
            if (g_stat (file.c_str(), &S)) throw Exception ("error accessing tracks file \"" + file + "\": " + Glib::strerror (errno));
            if (mtime != S.st_mtime) { load (file); return (true); }
            return (false);
          }

          void draw ();

          uint count () const
          {
            uint n = 0;
            for (std::list<Track>::const_iterator i = tracks.begin(); i != tracks.end(); ++i) n += i->size();
            return (n);
          }


          void add (std::vector<uint>& vertices, float min_dist, float max_dist)
          {
            for (std::list<Track>::iterator i = tracks.begin(); i != tracks.end(); ++i) {
              for (uint n = 0; n < i->size(); n++) {
                Track::Point& P ((*i)[n]);
                float Z = Track::Point::normal.dot (Point (P.pos));
                if (Z > min_dist && Z < max_dist) vertices.push_back (P.index());
              }
            }
          }

        protected:
          Allocator alloc;
      };






      inline TrackListItem::Track::Track (Allocator& alloc, uint num) : num_p (num)       { data = alloc (num_p); }

    }
  }
}

#endif

