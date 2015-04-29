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
        //CONF option: BackgroundColor
        //CONF default: 1,1,1 (white)
        //CONF The default colour to use for the background in OpenGL panels, notably
        //CONF the SH viewer.
        File::Config::get_RGB ("BackgroundColor", background_color, 1.0, 1.0, 1.0);
        //CONF option: ObjectColor
        //CONF default: 1,1,0 (yellow)
        //CONF The default colour to use for objects (i.e. SH glyphs) when not
        //CONF colouring by direction.
        File::Config::get_RGB ("ObjectColor", object_color, 1.0, 1.0, 0.0);

        //CONF option: LightPosition
        //CONF default: 1,1,3 
        //CONF The default position vector to use for the light in OpenGL
        //CONF renders
        File::Config::get_RGB ("LightPosition", lightpos, 1.0, 1.0, 3.0);

        Math::normalise (lightpos, 3);

        //CONF option: AmbientIntensity
        //CONF default: 0.6
        //CONF The default intensity for the ambient light in OpenGL renders
        ambient = File::Config::get_float ("AmbientIntensity", 0.6);
        //CONF option: DiffuseIntensity
        //CONF default: 0.3
        //CONF The default intensity for the diffuse light in OpenGL renders
        diffuse = File::Config::get_float ("DiffuseIntensity", 0.3);
        //CONF option: SpecularIntensity
        //CONF default: 0.4
        //CONF The default intensity for the specular light in OpenGL renders
        specular = File::Config::get_float ("SpecularIntensity", 0.4);
        //CONF option: SpecularExponent
        //CONF default: 1
        //CONF The default exponent for the specular light in OpenGL renders
        shine = File::Config::get_float ("SpecularExponent", 1.0);
      }



    }
  }
}

