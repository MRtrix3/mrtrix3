/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __gui_mrview_displayable_h__
#define __gui_mrview_displayable_h__

#include "math/math.h"
#include "gui/opengl/gl.h"
#include "gui/opengl/shader.h"
#include "gui/projection.h"
#include "gui/mrview/colourmap.h"


namespace MR
{
  class ProgressBar;

  namespace GUI
  {

    namespace MRView
    {

      class Window;

      const uint32_t InvertScale = 0x08000000;
      const uint32_t DiscardLower = 0x20000000;
      const uint32_t DiscardUpper = 0x40000000;
      const uint32_t Transparency = 0x80000000;
      const uint32_t Lighting = 0x01000000;
      const uint32_t DiscardLowerEnabled = 0x00100000;
      const uint32_t DiscardUpperEnabled = 0x00200000;
      const uint32_t TransparencyEnabled = 0x00400000;
      const uint32_t LightingEnabled = 0x00800000;

      class Image;
      namespace Tool { class BaseFixel; }
      namespace Tool { class Connectome; }
      namespace Tool { class Tractogram; }
      class DisplayableVisitor
      {
        public:
          virtual void render_image_colourbar (const Image&) {}
          virtual void render_fixel_colourbar (const Tool::BaseFixel&) {}
          virtual void render_tractogram_colourbar (const Tool::Tractogram&) {}
      };

      class Displayable : public QAction
      {
        Q_OBJECT

        public:
          Displayable (const std::string& filename);

          virtual ~Displayable ();

          virtual void request_render_colourbar(DisplayableVisitor&) {}

          const std::string& get_filename () const {
            return filename;
          }

          float scaling_min () const {
            return display_midpoint - 0.5f * display_range;
          }

          float scaling_max () const {
            return display_midpoint + 0.5f * display_range;
          }

          float scaling_min_thresholded () const {
            return std::max(scaling_min(), lessthan);
          }

          float scaling_max_thresholded () const {
            return std::min(scaling_max(), greaterthan);
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
            display_range *= std::exp (-0.002f * contrast);
            emit scalingChanged();
          }

          uint32_t flags () const { return flags_; }

          void set_allowed_features (bool thresholding, bool transparency, bool lighting) {
            uint32_t cmap = flags_;
            set_bit (cmap, DiscardLowerEnabled, thresholding);
            set_bit (cmap, DiscardUpperEnabled, thresholding);
            set_bit (cmap, TransparencyEnabled, transparency);
            set_bit (cmap, LightingEnabled, lighting);
            flags_ = cmap;
          }

          void set_colour (std::array<GLubyte,3> &c) {
            colour = c;
          }

          void set_use_discard_lower (bool yesno) {
            if (!discard_lower_enabled()) return;
            set_bit (DiscardLower, yesno);
          }

          void set_use_discard_upper (bool yesno) {
            if (!discard_upper_enabled()) return;
            set_bit (DiscardUpper, yesno);
          }

          void set_use_transparency (bool yesno) {
            if (!transparency_enabled()) return;
            set_bit (Transparency,  yesno);
          }

          void set_use_lighting (bool yesno) {
            if (!lighting_enabled()) return;
            set_bit (LightingEnabled, yesno);
          }

          void set_invert_scale (bool yesno) {
            set_bit (InvertScale, yesno);
          }

          bool scale_inverted () const {
            return flags_ & InvertScale;
          }

          bool discard_lower_enabled () const {
            return flags_ & DiscardLowerEnabled;
          }

          bool discard_upper_enabled () const {
            return flags_ & DiscardUpperEnabled;
          }

          bool transparency_enabled () const {
            return flags_ & TransparencyEnabled;
          }

          bool lighting_enabled () const {
            return flags_ & LightingEnabled;
          }

          bool use_discard_lower () const {
            return discard_lower_enabled() && ( flags_ & DiscardLower );
          }

          bool use_discard_upper () const {
            return discard_upper_enabled() && ( flags_ & DiscardUpper );
          }

          bool use_transparency () const {
            return transparency_enabled() && ( flags_ & Transparency );
          }

          bool use_lighting () const {
            return lighting_enabled() && ( flags_ & Lighting );
          }


          class Shader : public GL::Shader::Program {
            public:
              virtual std::string fragment_shader_source (const Displayable& object) = 0;
              virtual std::string geometry_shader_source (const Displayable&) { return std::string(); }
              virtual std::string vertex_shader_source (const Displayable& object) = 0;

              virtual bool need_update (const Displayable& object) const;
              virtual void update (const Displayable& object);

              void start (const Displayable& object) {
                if (*this  == 0 || need_update (object)) 
                  recompile (object);
                GL::Shader::Program::start();
              }
            protected:
              uint32_t flags;
              size_t colourmap;

              void recompile (const Displayable& object) {
                if (*this != 0) 
                  clear();

                update (object);

                GL::Shader::Vertex vertex_shader (vertex_shader_source (object));
                GL::Shader::Geometry geometry_shader (geometry_shader_source (object));
                GL::Shader::Fragment fragment_shader (fragment_shader_source (object));

                attach (vertex_shader);
                if((GLuint)geometry_shader)
                    attach (geometry_shader);
                attach (fragment_shader);
                link();
              }
          };


          std::string declare_shader_variables (const std::string& with_prefix = "") const {
            std::string source =
              "uniform float " + with_prefix+"offset;\n"
              "uniform float " + with_prefix+"scale;\n";
            if (use_discard_lower())
              source += "uniform float " + with_prefix+"lower;\n";
            if (use_discard_upper())
              source += "uniform float " + with_prefix+"upper;\n";
            if (use_transparency()) {
              source += 
                "uniform float " + with_prefix+"alpha_scale;\n"
                "uniform float " + with_prefix+"alpha_offset;\n"
                "uniform float " + with_prefix+"alpha;\n";
            }
            if (ColourMap::maps[colourmap].is_colour)
              source += "uniform vec3 " + with_prefix + "colourmap_colour;\n";
            return source;
          }

          void start (Shader& shader_program, float scaling = 1.0, const std::string& with_prefix = "") {
            shader_program.start (*this);
            set_shader_variables (shader_program, scaling, with_prefix);
          }

          void set_shader_variables (Shader& shader_program, float scaling = 1.0, const std::string& with_prefix = "") {
            gl::Uniform1f (gl::GetUniformLocation (shader_program, (with_prefix+"offset").c_str()), (display_midpoint - 0.5f * display_range) / scaling);
            gl::Uniform1f (gl::GetUniformLocation (shader_program, (with_prefix+"scale").c_str()), scaling / display_range);
            if (use_discard_lower())
              gl::Uniform1f (gl::GetUniformLocation (shader_program, (with_prefix+"lower").c_str()), lessthan / scaling);
            if (use_discard_upper())
              gl::Uniform1f (gl::GetUniformLocation (shader_program, (with_prefix+"upper").c_str()), greaterthan / scaling);
            if (use_transparency()) {
              gl::Uniform1f (gl::GetUniformLocation (shader_program, (with_prefix+"alpha_scale").c_str()), scaling / (opaque_intensity - transparent_intensity));
              gl::Uniform1f (gl::GetUniformLocation (shader_program, (with_prefix+"alpha_offset").c_str()), transparent_intensity / scaling);
              gl::Uniform1f (gl::GetUniformLocation (shader_program, (with_prefix+"alpha").c_str()), alpha);
            }
            if (ColourMap::maps[colourmap].is_colour)
              gl::Uniform3f (gl::GetUniformLocation (shader_program, (with_prefix+"colourmap_colour").c_str()), 
                  colour[0]/255.0, colour[1]/255.0, colour[2]/255.0);
          }

          void stop (Shader& shader_program) {
            shader_program.stop();
          }

          float lessthan, greaterthan;
          float display_midpoint, display_range;
          float transparent_intensity, opaque_intensity, alpha;
          std::array<GLubyte,3> colour;
          size_t colourmap;
          bool show;
          bool show_colour_bar;


        signals:
          void scalingChanged ();


        protected:
          std::string filename;
          float value_min, value_max;
          uint32_t flags_;

          void set_bit (uint32_t& field, uint32_t bit, bool value) {
            if (value) field |= bit;
            else field &= ~bit;
          }

          void set_bit (uint32_t bit, bool value) {
            uint32_t cmap = flags_;
            set_bit (cmap, bit, value);
            flags_ = cmap;
          }

          void update_levels () {
            assert (std::isfinite (value_min));
            assert (std::isfinite (value_max));
            if (!std::isfinite (transparent_intensity)) 
              transparent_intensity = value_min + 0.1 * (value_max - value_min);
            if (!std::isfinite (opaque_intensity)) 
              opaque_intensity = value_min + 0.5 * (value_max - value_min);
            if (!std::isfinite (alpha)) 
              alpha = 0.5;
            if (!std::isfinite (lessthan))
              lessthan = value_min;
            if (!std::isfinite (greaterthan))
              greaterthan = value_max;
          }

      };




    }
  }
}

#endif

