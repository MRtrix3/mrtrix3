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

#include "opengl/lighting.h"
#include "file/config.h"

namespace MR {
  namespace GL {

    namespace {
      void load_default_color (const std::string& entry, float* ret, float def_R, float def_G, float def_B)
      {
        std::string string;
        string = File::Config::get (entry);
        if (string.size()) { 
          try {
            std::vector<float> V (parse_floats (string));
            if (V.size() < 3) throw Exception ("invalid configuration key \"" + entry + "\" - ignored");
            ret[0] = V[0];
            ret[1] = V[1];
            ret[2] = V[2];
          }
          catch (Exception) { }
        }
        else { 
          ret[0] = def_R;
          ret[1] = def_G;
          ret[2] = def_B;
        }
      }

    }

    void Lighting::load_defaults ()
    {
      load_default_color ("OrientationPlot.BackgroundColor", background_color, 1.0, 1.0, 1.0);
      load_default_color ("OrientationPlot.AmbientColour", ambient_color, 1.0, 1.0, 1.0);
      load_default_color ("OrientationPlot.LightColour", light_color, 1.0, 1.0, 1.0);
      load_default_color ("OrientationPlot.ObjectColour", object_color, 1.0, 1.0, 1.0);
      load_default_color ("OrientationPlot.LightPosition", lightpos, 1.0, 1.0, 3.0);
      lightpos[3] = 0.0;

      ambient = File::Config::get_float ("OrientationPlot.AmbientIntensity", 0.4);
      diffuse = File::Config::get_float ("OrientationPlot.DiffuseIntensity", 0.7);
      specular = File::Config::get_float ("OrientationPlot.SpecularIntensity", 0.3);
      shine = File::Config::get_float ("OrientationPlot.SpecularExponent", 5.0);
    }




    void Lighting::set ()
    {
      if (set_background) glClearColor (background_color[0], background_color[1], background_color[2], 0.0);

      glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
      glShadeModel (GL_SMOOTH);
      glEnable (GL_LIGHT0);
      glEnable (GL_NORMALIZE);

      GLfloat v[] = { ambient_color[0]*ambient, ambient_color[1]*ambient, ambient_color[2]*ambient, 1.0 };
      glLightModelfv (GL_LIGHT_MODEL_AMBIENT, v);

      v[0] = 1.0;
      v[1] = 1.0;
      v[2] = 1.0;
      glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, v);
      glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, shine);

      glLightfv (GL_LIGHT0, GL_POSITION, lightpos);

      v[0] = light_color[0] * diffuse; 
      v[1] = light_color[1] * diffuse; 
      v[2] = light_color[2] * diffuse; 
      glLightfv (GL_LIGHT0, GL_DIFFUSE, v);

      v[0] = light_color[0] * specular; 
      v[1] = light_color[1] * specular; 
      v[2] = light_color[2] * specular; 
      glLightfv (GL_LIGHT0, GL_SPECULAR, v);

      v[0] = v[1] = v[2] = 0.0;
      glLightfv (GL_LIGHT0, GL_AMBIENT, v);

      v[0] = v[1] = v[2] = 0.9; 
      glMaterialfv (GL_BACK, GL_AMBIENT_AND_DIFFUSE, v);
    }
  }
}

