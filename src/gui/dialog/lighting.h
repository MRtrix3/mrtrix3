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


#ifndef __gui_dialog_lighting_h__
#define __gui_dialog_lighting_h__

#include "mrtrix.h"
#include "gui/opengl/lighting.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      class LightingSettings : public QFrame
      {
          Q_OBJECT

        public:
          LightingSettings (QWidget* parent, GL::Lighting& lighting, bool include_object_color);
          ~LightingSettings () { }

        protected:
          GL::Lighting&  info;
          QSlider* elevation_slider, *azimuth_slider;

        protected slots:
          void object_color_slot (const QColor& new_color);
          void ambient_intensity_slot (int value);
          void diffuse_intensity_slot (int value);
          void specular_intensity_slot (int value);
          void shine_slot (int value);
          void light_position_slot ();
      };






      class Lighting : public QDialog
      {
        public:
          Lighting (QWidget* parent, const std::string& message, GL::Lighting& lighting, bool include_object_color = true);

          LightingSettings* settings;
      };

    }
  }
}

#endif

