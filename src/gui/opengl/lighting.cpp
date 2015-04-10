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

#include "file/config.h"
#include "math/vector.h"
#include "gui/opengl/lighting.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      void Lighting::load_defaults ()
      {
        File::Config::get_RGB ("BackgroundColor", background_color, 1.0, 1.0, 1.0);
        File::Config::get_RGB ("ObjectColor", object_color, 1.0, 1.0, 0.0);
        File::Config::get_RGB ("LightPosition", lightpos, 2.0, 0.0, 0.5);

        Math::normalise (lightpos, 3);

        ambient = File::Config::get_float ("AmbientIntensity", 0.5);
        diffuse = File::Config::get_float ("DiffuseIntensity", 0.4);
        specular = File::Config::get_float ("SpecularIntensity", 0.4);
        shine = File::Config::get_float ("SpecularExponent", 2.0);
      }



    }
  }
}

