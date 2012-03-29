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




#include "math/hemisphere/directions.h"


namespace MR {
namespace Math {
namespace Hemisphere {



Directions::Directions (const std::string& path) :
    num_directions (0),
    adj_dirs (NULL),
    dir_mask_bytes (0),
    dir_mask_excess_bits (0),
    dir_mask_excess_bits_mask (0)
{
  try {
    load_file (path);
  } catch (...) {
    load_predefined (to<size_t>(path));
  }
  initialise();
}


Directions::Directions (const Directions& that) :
    num_directions (that.num_directions),
    az_el_pairs (that.az_el_pairs),
    unit_vectors (that.unit_vectors),
    adj_dirs (new std::vector<size_t>[num_directions])
{
  for (size_t i = 0; i != num_directions; ++i)
    adj_dirs[i] = that.adj_dirs[i];
}



Directions::~Directions()
{
  if (adj_dirs) {
    delete[] adj_dirs;
    adj_dirs = NULL;
  }
}





bool Directions::dirs_are_adjacent (const size_t one, const size_t two) const
{
  for (std::vector<size_t>::const_iterator i = adj_dirs[one].begin(); i != adj_dirs[one].end(); ++i) {
    if (*i == two)
      return true;
  }
  return false;
}



size_t Directions::get_min_linkage (const size_t one, const size_t two) const
{

  if (one == two)
    return 0;

  std::vector<bool> processed (num_directions, 0);
  std::vector<size_t> to_expand;
  processed[one] = true;
  to_expand.push_back (one);
  size_t min_linkage = 0;
  do {
    ++min_linkage;
    std::vector<size_t> next_to_expand;
    for (std::vector<size_t>::const_iterator i = to_expand.begin(); i != to_expand.end(); ++i) {
      for (std::vector<size_t>::const_iterator j = adj_dirs[*i].begin(); j != adj_dirs[*i].end(); ++j) {
        if (*j == two) {
          return min_linkage;
        } else if (!processed[*j]) {
          processed[*j] = true;
          next_to_expand.push_back (*j);
        }
      }
    }
    swap (to_expand, next_to_expand);
  } while (1);
  return std::numeric_limits<size_t>::max();

}


void Directions::load_file (const std::string& file_path)
{

  az_el_pairs = Math::Matrix<float> (file_path);
  if (az_el_pairs.columns() != 2)
    throw Exception ("Text file \"" + file_path + "\"does not contain directions as azimuth-elevation pairs");

}



void Directions::load_predefined (const size_t i)
{
  switch (i) {
    case 60:   directions_60   (az_el_pairs); break;
    case 129:  directions_129  (az_el_pairs); break;
    case 300:  directions_300  (az_el_pairs); break;
    case 457:  directions_457  (az_el_pairs); break;
    case 1281: directions_1281 (az_el_pairs); break;
    default: throw Exception ("No pre-defined data set of " + str (i) + " directions!");
  }
}



void Directions::initialise()
{

  num_directions = az_el_pairs.rows();
  unit_vectors.reserve (num_directions);
  for (size_t i = 0; i != num_directions; ++i) {
    const float azimuth   = az_el_pairs(i, 0);
    const float elevation = az_el_pairs(i, 1);
    const float sin_elevation = Math::sin (elevation);
    unit_vectors.push_back (Point<float> (Math::sin (azimuth) * sin_elevation, Math::cos (azimuth) * sin_elevation, Math::cos (elevation)));
  }

  adj_dirs = new std::vector<size_t>[num_directions];
  for (size_t i = 0; i != num_directions; ++i) {
    for (size_t j = 0; j != num_directions; ++j) {
      if (j != i) {

        Point<float> p;
        if (unit_vectors[i].dot (unit_vectors[j]) > 0.0)
          p = ((unit_vectors[i] + unit_vectors[j]).normalise());
        else
          p = ((unit_vectors[i] - unit_vectors[j]).normalise());
        const float dot_to_i = fabs (p.dot (unit_vectors[i]));
        const float dot_to_j = fabs (p.dot (unit_vectors[j]));
        const float this_dot_product = MAX(dot_to_i, dot_to_j);

        bool is_adjacent = true;
        for (size_t k = 0; (k != num_directions) && is_adjacent; ++k) {
          if ((k != i) && (k != j)) {
            if (fabs (p.dot (unit_vectors[k])) > this_dot_product)
              is_adjacent = false;
          }
        }

        if (is_adjacent)
          adj_dirs[i].push_back (j);

      }
    }
  }

  dir_mask_bytes = (num_directions + 7) / 8;
  dir_mask_excess_bits = (8 * dir_mask_bytes) - num_directions;
  dir_mask_excess_bits_mask = 0xFF >> dir_mask_excess_bits;

}




Directions_FastLookup::Directions_FastLookup (const std::string& path) :
    Directions (path)
{

  double adj_dot_product_sum = 0.0;
  size_t adj_dot_product_count = 0;
  for (size_t i = 0; i != num_directions; ++i) {
    for (std::vector<size_t>::const_iterator j = adj_dirs[i].begin(); j != adj_dirs[i].end(); ++j) {
      if (*j > i) {
        adj_dot_product_sum += Math::abs (unit_vectors[i].dot (unit_vectors[*j]));
        ++adj_dot_product_count;
      }
    }
  }

  const float min_dp = adj_dot_product_sum / double(adj_dot_product_count);
  const float max_angle_step = acos (min_dp);

  num_az_grids = ceil (2.0 * M_PI / max_angle_step) + 1;
  num_el_grids = ceil (      M_PI / max_angle_step) + 1;
  total_num_angle_grids = num_az_grids * num_el_grids;

  az_grid_step = 2.0 * M_PI / float(num_az_grids - 1);
  el_grid_step =       M_PI / float(num_el_grids - 1);

  az_begin = -M_PI;
  el_begin = 0.0;

  grid_near_dirs = new size_t*[total_num_angle_grids];

  unsigned int index = 0;

  for (unsigned int azimuth_grid = 0; azimuth_grid != num_az_grids; ++azimuth_grid) {
    const float azimuth = az_begin + (az_grid_step * (azimuth_grid - 0.5));

    for (unsigned int elevation_grid = 0; elevation_grid != num_el_grids; ++elevation_grid, ++index) {
      const float elevation = el_begin + (el_grid_step * (elevation_grid - 0.5));

      std::vector<size_t> this_grid_dirs_list;

      for (int azimuth_fine_grid = -2; azimuth_fine_grid != FINE_GRID_OVERSAMPLE_RATIO + 3; ++azimuth_fine_grid) {
        const float azimuth_fine = azimuth + (azimuth_fine_grid * az_grid_step / float(FINE_GRID_OVERSAMPLE_RATIO));

        for (int elevation_fine_grid = -2; elevation_fine_grid != FINE_GRID_OVERSAMPLE_RATIO + 3; ++elevation_fine_grid) {
          const float elevation_fine = elevation + (elevation_fine_grid * el_grid_step / float(FINE_GRID_OVERSAMPLE_RATIO));

          const Point<float> unit_vector (sin(elevation_fine) * cos(azimuth_fine), sin(elevation_fine) * sin(azimuth_fine), cos(elevation_fine));
          const size_t nearest_dir = select_direction_slow (unit_vector);
          bool value_present = false;
          for (std::vector<size_t>::const_iterator i = this_grid_dirs_list.begin(); i != this_grid_dirs_list.end(); ++i) {
            if (*i == nearest_dir) {
              value_present = true;
              break;
            }
          }
          if (!value_present)
            this_grid_dirs_list.push_back (nearest_dir);
        }
      }

      sort (this_grid_dirs_list.begin(), this_grid_dirs_list.end());

      size_t* this_array = new size_t[this_grid_dirs_list.size() + 1];
      this_array[0] = this_grid_dirs_list.size() + 1;
      for (unsigned int i = 0; i != this_grid_dirs_list.size(); ++i)
        this_array[i+1] = this_grid_dirs_list[i];
      grid_near_dirs[index] = this_array;

    }
  }

}


Directions_FastLookup::Directions_FastLookup (const Directions_FastLookup& that) :
    Directions (that),
    grid_near_dirs (new size_t*[total_num_angle_grids]),
    num_az_grids (that.num_az_grids),
    num_el_grids (that.num_el_grids),
    total_num_angle_grids (that.total_num_angle_grids),
    az_grid_step (that.az_grid_step),
    el_grid_step (that.el_grid_step),
    az_begin (that.az_begin),
    el_begin (that.el_begin)
{
  for (size_t i = 0; i != total_num_angle_grids; ++i) {
    const size_t array_size = that.grid_near_dirs[i][0];
    grid_near_dirs[i] = new size_t[array_size];
    memcpy (grid_near_dirs[i], that.grid_near_dirs[i], array_size * sizeof (size_t));
  }
}



Directions_FastLookup::~Directions_FastLookup ()
{
  if (grid_near_dirs) {
    for (size_t i = 0; i != total_num_angle_grids; ++i) {
      delete[] grid_near_dirs[i];
      grid_near_dirs[i] = NULL;
    }
    delete[] grid_near_dirs;
    grid_near_dirs = NULL;
  }
}



size_t Directions_FastLookup::select_direction (const Point<float>& p) const {

  float azimuth   = atan2(p[1], p[0]);
  float elevation = acos (p[2]);

  const unsigned int azimuth_grid   = round (( azimuth  - az_begin) / az_grid_step);
  const unsigned int elevation_grid = round ((elevation - el_begin) / el_grid_step);
  const unsigned int index = (azimuth_grid * num_el_grids) + elevation_grid;

  const size_t array_size = (grid_near_dirs[index])[0];
  size_t best_dir = (grid_near_dirs[index])[1];
  float max_dp = fabs (p.dot (unit_vectors[best_dir]));
  for (size_t i = 2; i != array_size; ++i) {
    const size_t this_dir = (grid_near_dirs[index])[i];
    const float this_dp = fabs (p.dot (unit_vectors[this_dir]));
    if (this_dp > max_dp) {
      max_dp = this_dp;
      best_dir = this_dir;
    }
  }

  return best_dir;

}



size_t Directions_FastLookup::select_direction_slow (const Point<float>& p) const {

  size_t dir = 0;
  float max_dot_product = fabs (p.dot (unit_vectors[0]));
  for (size_t i = 1; i != num_directions; ++i) {
    const float this_dot_product = fabs (p.dot (unit_vectors[i]));
    if (this_dot_product > max_dot_product) {
      max_dot_product = this_dot_product;
      dir = i;
    }
  }
  return dir;

}




}
}
}

