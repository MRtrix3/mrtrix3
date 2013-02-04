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
#include <QActionGroup>
#include <QMenu>

#include "math/math.h"
#include "gui/opengl/gl.h"
#include "gui/opengl/shader.h"
#include "gui/projection.h"
#include "gui/mrview/colourmap.h"

#ifdef Complex
# undef Complex
#endif

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

      const uint32_t InvertScale = 0x08000000;
      const uint32_t InvertMap = 0x10000000;
      const uint32_t DiscardLower = 0x20000000;
      const uint32_t DiscardUpper = 0x40000000;
      const uint32_t Transparency = 0x80000000;
      const uint32_t Lighting = 0x01000000;
      const uint32_t DiscardLowerOn = 0x00100000;
      const uint32_t DiscardUpperOn = 0x00200000;
      const uint32_t TransparencyOn = 0x00400000;
      const uint32_t LightingOn = 0x00800000;

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
            return display_midpoint - 0.5f * display_range;
          }

          float scaling_max () const {
            return display_midpoint + 0.5f * display_range;
          }

          float scaling_rate () const {
            return 1e-3 * (value_max - value_min);
          }

          float intensity_min () const {
            return value_min;
          }

          float intensity_max () const {
            return value_max;
          }

          void set_windowing (float min, float max) {
            display_range = max - min;
            display_midpoint = 0.5 * (min + max);
            emit scalingChanged();
          }
          void adjust_windowing (const QPoint& p) {
            adjust_windowing (p.x(), p.y());
          }

          void reset_windowing () {
            set_windowing (value_min, value_max);
          }

          void adjust_windowing (float brightness, float contrast) {
            display_midpoint -= 0.0005f * display_range * brightness;
            display_range *= Math::exp (-0.002f * contrast);
            emit scalingChanged();
          }

          void set_colourmap (size_t index) {
            if (index != colourmap_index) {
              colourmap_index = index;
              recompile();
            }
          }
          
          float lessthan, greaterthan;
          float display_midpoint, display_range;
          float transparent_intensity, opaque_intensity, alpha;

          bool operator! () const {
            return !shader_program;
          }
          void set (uint32_t new_flags) {
            if (new_flags != flags_) {
              flags_ = new_flags;
              recompile();
            }
          }

          void start (const Projection& projection, float scaling = 1.0) {
            if (!shader_program)
              recompile();
            shader_program.start();

            glUniformMatrix4fv (glGetUniformLocation (shader_program, "MVP"), 1, GL_FALSE, projection.modelview_projection());
            glUniform1f (glGetUniformLocation (shader_program, "offset"), (display_midpoint - 0.5f * display_range) / scaling);
            glUniform1f (glGetUniformLocation (shader_program, "scale"), scaling / display_range);
            if (flags_ & DiscardLower)
              glUniform1f (glGetUniformLocation (shader_program, "lower"), lessthan / scaling);
            if (flags_ & DiscardUpper)
              glUniform1f (glGetUniformLocation (shader_program, "upper"), greaterthan / scaling);
            if (flags_ & Transparency) {
              glUniform1f (glGetUniformLocation (shader_program, "alpha_scale"), scaling / (opaque_intensity - transparent_intensity));
              glUniform1f (glGetUniformLocation (shader_program, "alpha_offset"), transparent_intensity / scaling);
              glUniform1f (glGetUniformLocation (shader_program, "alpha"), alpha);
            }
          }

          void stop () {
            shader_program.stop();
          }

          GLuint get_uniform (const std::string& name) {
            return glGetUniformLocation (shader_program, name.c_str());
          }

          uint32_t flags () const { return flags_; }

          void set_allowed_features (bool thresholding, bool transparency, bool lighting) {
            uint32_t cmap = flags_;
            set_bit (cmap, DiscardLower, ( thresholding && discard_lower_enabled() && finite (lessthan) ));
            set_bit (cmap, DiscardUpper, ( thresholding && discard_upper_enabled() && finite (greaterthan) ));
            set_bit (cmap, Transparency, (
                  transparency && transparency_enabled() &&
                  finite (transparent_intensity) && finite (opaque_intensity)
                  && finite (alpha)));
            set_bit (cmap, Lighting, ( lighting_enabled() && lighting ));
            set (cmap);
          }


          void set_use_discard_lower (bool yesno) {
            set_bit (DiscardLower | DiscardLowerOn, ( yesno && finite (lessthan) ));
          }

          void set_use_discard_upper (bool yesno) {
            set_bit (DiscardLower | DiscardLowerOn, ( yesno && finite (lessthan) ));
          }

          void set_use_thresholds (bool yesno) {
            uint32_t cmap = flags_;
            set_bit (cmap, DiscardLower | DiscardLowerOn, ( yesno && finite (lessthan) ));
            set_bit (cmap, DiscardLower | DiscardLowerOn, ( yesno && finite (lessthan) ));
            set (cmap);
          }

          void set_use_transparency (bool value) {
            set_bit (Transparency | TransparencyOn,
                ( value && finite (transparent_intensity) && finite (opaque_intensity) && finite (alpha) ));
          }

          void set_use_lighting (bool value) {
            set_bit (Lighting | LightingOn, value);
          }

          void set_invert_map (bool value) {
            set_bit (InvertMap, value);
          }

          void set_invert_scale (bool value) {
            set_bit (InvertScale, value);
          }

          void set_thresholds (float less_than_value = NAN, float greater_than_value = NAN) {
            lessthan = less_than_value;
            greaterthan = greater_than_value;
            set_use_thresholds (true);
          }

          void set_transparency (float transparent = NAN, float opaque = NAN, float alpha_value = 1.0) {
            transparent_intensity = transparent;
            opaque_intensity = opaque;
            alpha = alpha_value;
            set_use_transparency (true);
          }

          size_t colourmap () const {
            return colourmap_index;
          }

          bool scale_inverted () const {
            return flags_ & InvertScale;
          }

          bool colourmap_inverted () const {
            return flags_ & InvertMap;
          }

          bool discard_lower_enabled () const {
            return flags_ & DiscardLowerOn;
          }

          bool discard_upper_enabled () const {
            return flags_ & DiscardUpperOn;
          }

          bool transparency_enabled () const {
            return flags_ & TransparencyOn;
          }

          bool lighting_enabled () const {
            return flags_ & LightingOn;
          }

          bool use_discard_lower () const {
            return flags_ & DiscardLower;
          }

          bool use_discard_upper () const {
            return flags_ & DiscardUpper;
          }

          bool use_transparency () const {
            return flags_ & Transparency;
          }

          bool use_lighting () const {
            return flags_ & Lighting;
          }

          virtual void recompile ();


        signals:
          void scalingChanged ();


        protected:
          const std::string filename;
          float value_min, value_max;

          uint32_t flags_;
          size_t colourmap_index;

          GL::Shader::Program shader_program;

          static const char* vertex_shader_source;

          void set_bit (uint32_t& field, uint32_t bit, bool value) {
            if (value) field |= bit;
            else field &= ~bit;
          }

          void set_bit (uint32_t bit, bool value) {
            uint32_t cmap = flags_;
            set_bit (cmap, bit, value);
            set (cmap);
          }

      };

    }
  }
}

#endif

