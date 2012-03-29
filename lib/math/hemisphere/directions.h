/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

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




#ifndef __math_hemisphere_directions_h__
#define __math_hemisphere_directions_h__



#include <stdint.h>
#include <vector>

#include "point.h"
#include "progressbar.h"

#include "math/hemisphere/predefined_dirs.h"
#include "math/matrix.h"




namespace MR {
namespace Math {
namespace Hemisphere {



class Directions {

  public:

    Directions (const std::string&);
    Directions (const Directions&);
    ~Directions ();

    size_t                     get_num_dirs      ()                                   const { return num_directions; }
    const Point<float>&        get_dir           (const size_t i)                     const { return unit_vectors[i]; }
    const std::vector<size_t>& get_adj_dirs      (const size_t i)                     const { return adj_dirs[i]; }
    bool                       dirs_are_adjacent (const size_t one, const size_t two) const;
    size_t                     get_min_linkage   (const size_t one, const size_t two) const;

    size_t size() const { return num_directions; }

    const std::vector< Point<float> >& get_dirs() const { return unit_vectors; }
    const Point<float>& operator[] (const size_t i) const { return unit_vectors[i]; }


  protected:

    size_t num_directions;
    Math::Matrix<float> az_el_pairs;
    std::vector< Point<float> > unit_vectors;
    std::vector<size_t>* adj_dirs; // Note: not self-inclusive

  private:

    size_t dir_mask_bytes, dir_mask_excess_bits;
    uint8_t dir_mask_excess_bits_mask;
    friend class Dir_mask;

    Directions ();
    void load_file (const std::string&);
    void load_predefined (const size_t);
    void initialise();

};




// When mapping each azimuth/elevation grid block to the possible nearest directions within that grid location,
//   oversample both azimuth & elevation by this amount within the grid, and perform a full check against
//   all possible directions to see which is nearest.
#define FINE_GRID_OVERSAMPLE_RATIO 8


class Directions_FastLookup : public Directions {

  public:

    Directions_FastLookup (const std::string&);
    Directions_FastLookup (const Directions_FastLookup&);
    ~Directions_FastLookup ();

    size_t select_direction (const Point<float>&) const;



  private:

    size_t** grid_near_dirs;
    unsigned int num_az_grids, num_el_grids, total_num_angle_grids;
    float az_grid_step, el_grid_step;
    float az_begin, el_begin;

    Directions_FastLookup ();
    size_t select_direction_slow (const Point<float>&) const;

};



}
}
}

#endif

