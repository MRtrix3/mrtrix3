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


#ifndef __gui_mrview_tool_tractogram_h__
#define __gui_mrview_tool_tractogram_h__

#include "gui/mrview/displayable.h"
#include "dwi/tractography/properties.h"
#include "gui/mrview/tool/tractography/tractography.h"
#include "dwi/tractography/streamline.h"
#include "gui/mrview/colourmap.h"


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

        enum ColourType { Direction, Ends, Colour };

        class Tractogram : public Displayable
        {
          Q_OBJECT

          public:
            Tractogram (Window& parent, Tractography& tool, const std::string& filename);

            ~Tractogram ();

            void render (const Projection& transform);

            void load_tracks();

            void load_end_colours();
            void erase_nontrack_data();

            void set_colour (float c[3])
            {
              colour[0] = c[0];
              colour[1] = c[1];
              colour[2] = c[2];
            }

            ColourType color_type;
            float colour[3];

            class Shader : public Displayable::Shader {
              public:
                Shader () : do_crop_to_slab (false), use_lighting (false), color_type (Direction) { }
                virtual std::string vertex_shader_source (const Displayable& tractogram);
                virtual std::string fragment_shader_source (const Displayable& tractogram);
                virtual bool need_update (const Displayable& object) const;
                virtual void update (const Displayable& object);
              protected:
                bool do_crop_to_slab, use_lighting;
                ColourType color_type;

            } track_shader;

          signals:
            void scalingChanged ();

          private:
            Window& window;
            Tractography& tractography_tool;
            std::string filename;
            std::vector<GLuint> vertex_buffers;
            std::vector<GLuint> vertex_array_objects;
            std::vector<GLuint> colour_buffers;
            DWI::Tractography::Properties properties;
            std::vector<std::vector<GLint> > track_starts;
            std::vector<std::vector<GLint> > track_sizes;
            std::vector<size_t> num_tracks_per_buffer;


            void load_tracks_onto_GPU (DWI::Tractography::Streamline<float>& buffer,
                                              std::vector<GLint>& starts,
                                              std::vector<GLint>& sizes,
                                              size_t& tck_count);
                                              
            void load_end_colours_onto_GPU (DWI::Tractography::Streamline<float>& buffer);

        };
      }
    }
  }
}

#endif

