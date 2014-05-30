/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 2014

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

#include "dwi/sdeconv/response.h"


namespace MR {
  namespace DWI {

    const size_t default_response_coefficients_size[] = { 6, 8 };
    const float default_response_coefficients[] = { 
      0.0f, 3.5449f,     0.0f,    0.0f,     0.0f,    0.0f,     0.0f,   0.0f,
      1000.0f, 1.5616f, -0.5476f, 0.1002f, -0.0135f, 0.0024f,     0.0f,   0.0f,
      2000.0f, 1.0564f, -0.6033f, 0.1864f, -0.0418f, 0.0112f,     0.0f,   0.0f,
      3000.0f, 0.8857f, -0.5827f, 0.2261f, -0.0659f, 0.0207f, -0.0041f,   0.0f,
      4000.0f, 0.7480f, -0.5245f, 0.2263f, -0.0689f, 0.0246f, -0.0087f,   0.0f,
      5000.0f, 0.6329f, -0.4398f, 0.2003f, -0.0667f, 0.0248f, -0.0090f, 0.003f
    };

  }
}



