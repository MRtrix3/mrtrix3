/*
    Copyright 2010 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 15/01/10.

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

#ifdef EXT
# undef EXT
#endif 

#ifdef GL_GLEXT_PROTOTYPES
# define EXT(type, name)
#else 

# ifdef __DECLARE__
#  define EXT(type, name) extern PFNGL##type##PROC gl##name
# endif

# ifdef __DEFINE__
#  define EXT(type, name) PFNGL##type##PROC gl##name = NULL
# endif

# ifdef __LINK__
#  define EXT(type, name) gl##name = (PFNGL##type##PROC) glXGetProcAddress ((const GLubyte*) ("gl"#name))
# endif

#endif

