/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#ifndef __gui_mrview_mode_list_h__
# define __gui_mrview_mode_list_h__

# ifdef MODE
#  error MODE is already defined!
# endif

// place #include files in here:
#include "gui/mrview/mode/mode2d.h"
#include "gui/mrview/mode/mode3d.h"

#else

# ifndef __viewer_mode_list_read_once_h__
#  define __viewer_mode_list_read_once_h__
#  define MODE(index, classname, guiname, tooltip_text) case index: return (new classname (parent))
# else
#  ifndef __viewer_mode_list_read_twice_h__
#   define __viewer_mode_list_read_twice_h__
#   define MODE(index, classname, guiname, tooltip_text) case index: return (#guiname)
#  else
#   define MODE(index, classname, guiname, tooltip_text) case index: return (#tooltip_text)
#  endif
# endif

/* Provide a corresponding line for each mode here:
The first argument is the index of the mode, which should increment from zero.
The second argument is the name of the mode as displayed to the user. 
The third argument is the name of the corresponding Mode class. */

MODE (0, Mode2D, 2D, display slices aligned with image axes);
MODE (1, Mode3D, 3D reslice, display slices at an arbitrary angle);

# undef MODE
#endif


