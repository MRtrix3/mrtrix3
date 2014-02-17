/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#include "file/config.h"
#include "math/vector.h"
#include "gui/opengl/lighting.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      namespace
      {
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
        load_default_color ("BackgroundColor", background_color, 1.0, 1.0, 1.0);
        load_default_color ("ObjectColor", object_color, 1.0, 1.0, 0.0);
        load_default_color ("LightPosition", lightpos, 1.0, 1.0, 3.0);

        Math::normalise (lightpos, 3);

        ambient = File::Config::get_float ("AmbientIntensity", 0.4);
        diffuse = File::Config::get_float ("DiffuseIntensity", 0.7);
        specular = File::Config::get_float ("SpecularIntensity", 0.4);
        shine = File::Config::get_float ("SpecularExponent", 8.0);
      }



    }
  }
}

