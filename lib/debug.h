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

*/


#ifndef __debug_h__
#define __debug_h__

#include "mrtrix.h"
#include "app.h"

/** Prints the file and line number. Useful for debugging purposes. */
#define TEST std::cerr << MR::App::name() << ": line " << __LINE__ \
                       << " in " << __func__ << "() from file " << __FILE__ << "\n"


/** Prints a variable name and its value, followed by the file and line number. Useful for debugging purposes. */
#define VAR(variable) std::cerr << MR::App::name() << ": " << #variable << " = " << (variable) \
                                << " (in " << __func__ << "() from " << __FILE__  << ": " << __LINE__ << ")\n"

#endif

