#ifndef __gui_opengl_lighting_h__
#define __gui_opengl_lighting_h__

#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      class Lighting : public QObject
      {
          Q_OBJECT

        public:
          Lighting (QObject* parent) : 
            QObject (parent), 
            ambient (0.4), 
            diffuse (0.7), 
            specular (0.3), 
            shine (5.0), 
            set_background (false) {
            load_defaults();
          }

          float ambient, diffuse, specular, shine;
          float light_color[3], object_color[3], lightpos[3], background_color[3];
          bool set_background;

          void  set () const;
          void  load_defaults ();
          void  update () {
            emit changed();
          }

        signals:
          void changed ();
      };

    }
  }
}

#endif



