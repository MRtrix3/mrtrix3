/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __gui_mrview_tool_sphericalrois_h__
#define __gui_mrview_tool_sphericalrois_h__

#include <map>

#include "dwi/tractography/properties.h"
#include "gui/mrview/tool/tractography/tractography.h"
#include "gui/opengl/gl.h"
#include "gui/shapes/sphere.h"


namespace MR
{

  namespace GUI
  {
    class Projection;

    namespace MRView
    {
      class Window;

      namespace Tool
      {
        class SphericalROIs : public Displayable
        { MEMALIGN(SphericalROIs)
          Q_OBJECT

          public:

            enum class type_t { UNDEFINED, SEED, INCLUDE, EXCLUDE, MASK };

            SphericalROIs (const std::string&, const GUI::MRView::Tool::Tractography&);
            ~SphericalROIs ();

            void clear();
            void load (const DWI::Tractography::Properties&);
            void render (const Projection&);

            class Shader : public Displayable::Shader { MEMALIGN(Shader)
              public:
                Shader () :
                    is_3D (false),
                    use_lighting (false),
                    use_transparency (false) { }
                std::string vertex_shader_source   (const Displayable&) override;
                std::string geometry_shader_source (const Displayable&) override;
                std::string fragment_shader_source (const Displayable&) override;
                virtual bool need_update (const Displayable&) const override;
                virtual void update (const Displayable&) override;
              protected:
                bool is_3D, use_lighting, use_transparency;
            } shader;

          signals:
            void on_opacity_change();

          private:

            class SphereSpec { MEMALIGN(SphereSpec)
              public:
                SphereSpec (const type_t t, const Eigen::Vector3f& c, const float r) : type (t), centre (c), radius (r) { }
                type_t type;
                Eigen::Vector3f centre;
                float radius;
            };

            class Shared { MEMALIGN(Shared)
              public:
                Shared() { }
                void initialise();
                Shapes::Sphere sphere;
                GL::VertexArrayObject vao;
                std::map<type_t, Eigen::Vector3f> type2colour;
            };

            const GUI::MRView::Tool::Tractography& tractography_tool;
            vector<SphereSpec> data;
            GL::VertexBuffer vertex_buffer, radii_buffer, colour_buffer;
            GL::VertexArrayObject vertex_array_object;
            bool vao_dirty;

            static Shared shared;

        };



      }
    }
  }
}

#endif

