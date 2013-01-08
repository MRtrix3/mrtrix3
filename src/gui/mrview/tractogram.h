/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier & David Raffelt, 17/12/12.

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

#ifndef __gui_mrview_tool_tractogram_h__
#define __gui_mrview_tool_tractogram_h__

#include <QAction>
#include <string>

// necessary to avoid conflict with Qt4's foreach macro:
#ifdef foreach
# undef foreach
#endif

#include "gui/mrview/displayable.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "math/versor.h"
#include "image/interp/linear.h"
#include "gui/mrview/shader.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/file.h"


namespace MR
{
  class ProgressBar;

  namespace GUI
  {
    class Projection;

    namespace MRView
    {
      class Window;

      namespace Tool
      {

        class Tractogram : public Displayable
        {
          Q_OBJECT

          public:
            Tractogram (const std::string& filename);
            Tractogram (Window& parent, const std::string& filename);

            ~Tractogram ();

            std::string& get_filename () {
              return filename;
            }
            const std::string& get_filename () const {
              return filename;
            }

            float scaling_min () const {
              return shader.display_midpoint - 0.5f * shader.display_range;
            }
            float scaling_max () const {
              return shader.display_midpoint + 0.5f * shader.display_range;
            }

            float intensity_min () const {
              return value_min;
            }
            float intensity_max () const {
              return value_max;
            }

            void set_windowing (float min, float max) {
              shader.display_range = max - min;
              shader.display_midpoint = 0.5 * (min + max);
              emit scalingChanged();
            }
            void adjust_windowing (const QPoint& p) {
              adjust_windowing (p.x(), p.y());
            }
            void set_interpolate (bool linear) {
              interpolation = linear ? GL_LINEAR : GL_NEAREST;
            }
            bool interpolate () const {
              return interpolation == GL_LINEAR;
            }
            void reset_windowing () {
              set_windowing (value_min, value_max);
            }

            void adjust_windowing (float brightness, float contrast) {
              shader.display_midpoint -= 0.0005f * shader.display_range * brightness;
              shader.display_range *= Math::exp (-0.002f * contrast);
              emit scalingChanged();
            }


            void render ();


            void set_colourmap (uint32_t index) {
//              if (index >= ColourMap::Special || shader.colourmap() >= ColourMap::Special) {
//                if (index != shader.colourmap()) {
//                  position[0] = position[1] = position[2] = std::numeric_limits<ssize_t>::min();
//                  texture_mode_3D_unchanged = false;
//                }
//              }
//              shader.set_colourmap (index);
            }


            uint32_t colourmap_index () const {
              uint32_t cret = shader.colourmap();
              if (cret >= ColourMap::Special)
                cret -= ColourMap::Special - ColourMap::NumScalar;
              return cret;
            }

//            float scaling_rate () const {
//              return 1e-3 * (value_max - value_min);
//            }


          signals:
            void scalingChanged ();

          public:
            Shader shader;


          private:
            std::string filename;
            std::vector<GLuint> vertex_buffers;
            DWI::Tractography::Reader<float> file;
            DWI::Tractography::Properties properties;

            void set_color () {
            }

            void load_tracks();

        };

      }
    }
  }
}

#endif

