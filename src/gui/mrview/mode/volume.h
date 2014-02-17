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





