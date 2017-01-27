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
      namespace {

        constexpr float DefaultAmbient = 0.5f;
        constexpr float DefaultDiffuse = 0.5f;
        constexpr float DefaultSpecular = 0.5f;
        constexpr float DefaultShine = 5.0f;
        constexpr float DefaultBackgroundColor[3] = { 1.0f, 1.0f, 1.0f };
        constexpr float DefaultLightPosition[3] = { 1.0f, 1.0f, 3.0f };

      }



      void Lighting::load_defaults ()
      {
        //CONF option: BackgroundColor
        //CONF The default colour to use for the background in OpenGL panels, notably
        //CONF the SH viewer.
        File::Config::get_RGB ("BackgroundColor", background_color, DefaultBackgroundColor[0], DefaultBackgroundColor[1], DefaultBackgroundColor[2]);

        //CONF option: LightPosition
        //CONF The default position vector to use for the light in OpenGL
        //CONF renders.
        File::Config::get_RGB ("LightPosition", lightpos, DefaultLightPosition[0], DefaultLightPosition[1], DefaultBackgroundColor[2]);

        Eigen::Map<Eigen::Vector3f> (lightpos).normalize();

        //CONF option: AmbientIntensity
        //CONF The default intensity for the ambient light in OpenGL renders.
        ambient = File::Config::get_float ("AmbientIntensity", DefaultAmbient);
        //CONF option: DiffuseIntensity
        //CONF The default intensity for the diffuse light in OpenGL renders.
        diffuse = File::Config::get_float ("DiffuseIntensity", DefaultDiffuse);
        //CONF option: SpecularIntensity
        //CONF The default intensity for the specular light in OpenGL renders.
        specular = File::Config::get_float ("SpecularIntensity", DefaultSpecular);
        //CONF option: SpecularExponent
        //CONF The default exponent for the specular light in OpenGL renders.
        shine = File::Config::get_float ("SpecularExponent", DefaultShine);
      }



    }
  }
}

