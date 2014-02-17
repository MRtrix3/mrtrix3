#include "file/config.h"
#include "gui/opengl/font.h"
#include "gui/mrview/colourmap.h"
#include "gui/mrview/displayable.h"
#include "gui/projection.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace ColourMap
      {

        const char* Entry::default_amplitude = "color.r";



        const Entry maps[] = {
          Entry ("Gray", 
              "color.rgb = vec3 (amplitude);\n"),

          Entry ("Hot", 
              "color.rgb = vec3 (2.7213 * amplitude, 2.7213 * amplitude - 1.0, 3.7727 * amplitude - 2.7727);\n"),

          Entry ("Cool",
              "color.rgb = 1.0 - (vec3 (2.7213 * (1.0 - amplitude), 2.7213 * (1.0 - amplitude) - 1.0, 3.7727 * (1.0 - amplitude) - 2.7727));\n"),

          Entry ("Jet", 
              "color.rgb = 1.5 - 4.0 * abs (1.0 - amplitude - vec3(0.25, 0.5, 0.75));\n"),

          Entry ("RGB",
              "color.rgb = scale * (abs(color.rgb) - offset);\n",
              "length (color.rgb)",
              true),

          Entry ("Complex",
              "float phase = atan (color.r, color.g) * 0.954929658551372;\n"
              "color.rgb = phase + vec3 (-2.0, 0.0, 2.0);\n"
              "if (phase > 2.0) color.b -= 6.0;\n"
              "if (phase < -2.0) color.r += 6.0;\n"
              "color.rgb = clamp (scale * (amplitude - offset), 0.0, 1.0) * (2.0 - abs (color.rgb));\n",
              "length (color.rg)",
              true),

          Entry (NULL, NULL, NULL, true)
        };





        void create_menu (QWidget* parent, QActionGroup*& group, QMenu* menu, QAction** & actions, bool create_shortcuts, bool use_special)
        {
          group = new QActionGroup (parent);
          group->setExclusive (true);
          actions = new QAction* [num()];
          bool in_scalar_section = true;

          for (size_t n = 0; maps[n].name; ++n) {
            if (maps[n].special && !use_special)
              continue;
            QAction* action = new QAction (maps[n].name, parent);
            action->setCheckable (true);
            group->addAction (action);

            if (maps[n].special && in_scalar_section) {
              menu->addSeparator();
              in_scalar_section = false;
            }

            menu->addAction (action);
            parent->addAction (action);

            if (create_shortcuts)
              action->setShortcut (QObject::tr (std::string ("Ctrl+" + str (n+1)).c_str()));

            actions[n] = action;
          }

          actions[0]->setChecked (true);
        }




        Renderer::Renderer () : 
          current_index (0),
          current_inverted (false),
          width (MR::File::Config::get_float ("MRViewColourBarWidth", 20.0f)), 
          height (MR::File::Config::get_float ("MRViewColourBarHeight", 100.0f)), 
          inset (MR::File::Config::get_float ("MRViewColourBarInset", 20.0f)), 
          text_offset (MR::File::Config::get_float ("MRViewColourBarTextOffset", 10.0f)) { } 








        void Renderer::setup (size_t index, bool inverted)
        {
          program.clear();
          frame_program.clear();

          std::string source =
            "layout(location=0) in vec3 data;\n"
            "uniform float scale_x, scale_y;\n"
            "out float amplitude;\n"
            "void main () {\n"
            "  gl_Position = vec4 (data.x*scale_x-1.0, data.y*scale_y-1.0, 0.0, 1.0);\n"
            "  amplitude = ";
            if (inverted)
              source += "1.0 - ";
            source += "data.z;\n"
            "}\n";

          GL::Shader::Vertex vertex_shader (source);

          GL::Shader::Fragment fragment_shader (
              "in float amplitude;\n"
              "out vec3 color;\n"
              "void main () {\n"
              "  " + std::string(maps[index].mapping) +
              "}\n");

          program.attach (vertex_shader);
          program.attach (fragment_shader);
          program.link();

          GL::Shader::Fragment frame_fragment_shader (
              "out vec3 color;\n"
              "void main () {\n"
              "  color = vec3(1.0, 1.0, 0.0);\n"
              "}\n");

          frame_program.attach (vertex_shader);
          frame_program.attach (frame_fragment_shader);
          frame_program.link();
          
          current_index = index;
          current_inverted = inverted;
        }












        void Renderer::render (const Projection& projection, const Displayable& object, int position, bool inverted)
        {
          if (!position) return;
          if (maps[object.colourmap].special) return;
          
          if (!program || !frame_program || object.colourmap != current_index || current_inverted != inverted)
            setup (object.colourmap, inverted);

          if (!VB || !VAO) {
            VB.gen();
            VAO.gen();

            VB.bind (gl::ARRAY_BUFFER);
            VAO.bind();

            gl::EnableVertexAttribArray (0);
            gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);
          }
          else {
            VB.bind (gl::ARRAY_BUFFER);
            VAO.bind();
          }


          GLfloat data[] = {
            0.0f,   0.0f,  0.0f,
            0.0f,   height, 1.0f,
            width, height, 1.0f,
            width, 0.0f,  0.0f
          };
          float x_offset = inset;
          float y_offset = inset;
          int halign = -1;
          if (position == 2 || position == 4) {
            x_offset = projection.width() - width - inset;
            halign = 1;
          }
          if (position == 3 || position == 4) 
            y_offset = projection.height() - height - inset;

          data[0] += x_offset; data[1] += y_offset;
          data[3] += x_offset; data[4] += y_offset;
          data[6] += x_offset; data[7] += y_offset;
          data[9] += x_offset; data[10] += y_offset;

          gl::BufferData (gl::ARRAY_BUFFER, sizeof(data), data, gl::STREAM_DRAW);

          gl::DepthMask (gl::FALSE_);
          gl::LineWidth (1.0);
          gl::Disable (gl::BLEND);
          gl::Disable (gl::DEPTH_TEST);

          program.start();
          gl::Uniform1f (gl::GetUniformLocation (program, "scale_x"), 2.0f / projection.width());
          gl::Uniform1f (gl::GetUniformLocation (program, "scale_y"), 2.0f / projection.height());
          gl::DrawArrays (gl::TRIANGLE_FAN, 0, 4);
          program.stop();

          frame_program.start();
          gl::Uniform1f (gl::GetUniformLocation (program, "scale_x"), 2.0f / projection.width());
          gl::Uniform1f (gl::GetUniformLocation (program, "scale_y"), 2.0f / projection.height());
          gl::DrawArrays (gl::LINE_LOOP, 0, 4);
          frame_program.stop();

          projection.setup_render_text();
          int x = halign > 0 ? data[0] - text_offset : data[6] + text_offset;
          projection.render_text_align (x, data[1], str(object.scaling_min()), halign, 0);
          projection.render_text_align (x, data[4], str(object.scaling_max()), halign, 0);
          projection.done_render_text();

          gl::DepthMask (gl::TRUE_);

        }



      }
    }
  }
}

