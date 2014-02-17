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

