/*
   Copyright 2015 Brain Research Institute, Melbourne, Australia

   Written by Maximilian Pietsch, 2015-04-09

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

#ifndef __image_registration_transform_transform_utils_h__
#define __image_registration_transform_transform_utils_h__

#include <limits>
namespace MR
{
  namespace Image
  {
    namespace Registration
    {
      namespace Transform
      {
	  template <typename TransformMatrixType, typename StreamType, typename PrecisionType = int>
	  void write_affine (const TransformMatrixType& transform, StreamType& out, const PrecisionType precision = 0)
	  {   
		if (precision > 0)		
			out.precision(precision);
		else
			out.precision(std::numeric_limits<long double>::digits10 + 1);
		out << "mrtrix transformation affine matrix"; 
		out << "\nmatrix: " << transform (0,0) << "," <<  transform (0,1) << "," << transform (0,2) << "," << transform (0,3);
		out << "\nmatrix: " << transform (1,0) << "," <<  transform (1,1) << "," << transform (1,2) << "," << transform (1,3);
		out << "\nmatrix: " << transform (2,0) << "," <<  transform (2,1) << "," << transform (2,2) << "," << transform (2,3);
		out << "\nmatrix: " << transform (3,0) << "," <<  transform (3,1) << "," << transform (3,2) << "," << transform (3,3);
		out << "\n";
	  }

	  }
	}
  }
}
#endif
