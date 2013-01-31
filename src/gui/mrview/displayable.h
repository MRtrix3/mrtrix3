/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier and David Raffelt, 07/12/12.

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

#ifndef __gui_mrview_displayable_h__
#define __gui_mrview_displayable_h__

#include <QAction>

#include "gui/mrview/shader.h"
#include "math/math.h"

class QAction;

namespace MR
{
  class ProgressBar;

  namespace GUI
  {
    class Projection;

    namespace MRView
    {

      class Window;

      class Displayable : public QAction
      {
        Q_OBJECT

        public:
          Displayable (const std::string& filename);
          Displayable (Window& window, const std::string& filename);

          virtual ~Displayable ();

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

          void set_colourmap (size_t index) {
            if (ColourMap::maps[index].special || ColourMap::maps[shader.colourmap()].special) {
              if (index != shader.colourmap()) {
//                position[0] = position[1] = position[2] = std::numeric_limits<ssize_t>::min(); TODO
                texture_mode_3D_unchanged = false;
              }
            }
            shader.set_colourmap (index);
          }
          
          size_t colourmap () const {
            return shader.colourmap();
          }


        signals:
          void scalingChanged ();

        public:
          Shader shader;


        protected:
          int interpolation;
          float value_min, value_max;
          bool texture_mode_3D_unchanged;
          const std::string filename;

      };

    }
  }
}

#endif

