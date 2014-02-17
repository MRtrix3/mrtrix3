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

