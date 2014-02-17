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


#ifndef __gui_mrview_mode_volume_h__
#define __gui_mrview_mode_volume_h__

#include "app.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool { class View; }



      namespace Mode
      {

        class Volume : public Base
        {
          public:
            Volume (Window& parent) :
              Base (parent, FocusContrast | MoveTarget | TiltRotate | ShaderTransparency | ShaderThreshold | ShaderClipping),
              volume_shader (*this) { 
              }

            virtual void paint (Projection& projection);
            virtual void slice_move_event (int x);
            virtual void pan_event ();
            virtual void panthrough_event ();
            virtual void tilt_event ();
            virtual void rotate_event ();

          protected:
            GL::VertexBuffer volume_VB, volume_VI;
            GL::VertexArrayObject volume_VAO;
            GL::Texture depth_texture;
            std::vector<GL::vec4> clip;

            class Shader : public Displayable::Shader {
              public:
                Shader (const Volume& mode) : mode (mode), active_clip_planes (0) { }
                virtual std::string vertex_shader_source (const Displayable& object);
                virtual std::string fragment_shader_source (const Displayable& object);
                virtual bool need_update (const Displayable& object) const;
                virtual void update (const Displayable& object);
                const Volume& mode;
                size_t active_clip_planes;
            } volume_shader;

            Tool::View* get_view_tool () const;
            std::vector< std::pair<GL::vec4,bool> > get_active_clip_planes () const;
            std::vector<GL::vec4*> get_clip_planes_to_be_edited () const;


            void move_clip_planes_in_out (std::vector<GL::vec4*>& clip, float distance);
            void rotate_clip_planes (std::vector<GL::vec4*>& clip, const Math::Versor<float>& rot);
        };

      }
    }
  }
}

#endif





