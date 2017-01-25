/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "file/config.h"
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

        //CONF option: LightPosition
        //CONF default: 1,1,3
        //CONF The default position vector to use for the light in OpenGL
        //CONF renders.
        File::Config::get_RGB ("LightPosition", lightpos, 1.0, 1.0, 3.0);

        Eigen::Map<Eigen::Vector3f> (lightpos).normalize();

        //CONF option: AmbientIntensity
        //CONF default: 0.6
        //CONF The default intensity for the ambient light in OpenGL renders.
        ambient = File::Config::get_float ("AmbientIntensity", 0.6);
        //CONF option: DiffuseIntensity
        //CONF default: 0.3
        //CONF The default intensity for the diffuse light in OpenGL renders.
        diffuse = File::Config::get_float ("DiffuseIntensity", 0.3);
        //CONF option: SpecularIntensity
        //CONF default: 0.4
        //CONF The default intensity for the specular light in OpenGL renders.
        specular = File::Config::get_float ("SpecularIntensity", 0.4);
        //CONF option: SpecularExponent
        //CONF default: 1
        //CONF The default exponent for the specular light in OpenGL renders.
        shine = File::Config::get_float ("SpecularExponent", 1.0);
      }



    }
  }
}

