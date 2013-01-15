/*
   Copyright 2010 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#ifndef __gui_mrview_shader_h__
#define __gui_mrview_shader_h__

#include <QAction>
#include <QActionGroup>
#include <QMenu>

#include "gui/opengl/gl.h"
#include "gui/opengl/shader.h"

#ifdef Complex
# undef Complex
#endif

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

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

      namespace ColourMap
      {
        const uint32_t Mask = 0x000000FF;
        const uint32_t MaskNonScalar = 0x00000080;

        const size_t NumScalar = 4;
        const uint32_t Gray = 0x00000000;
        const uint32_t Hot = 0x00000001;
        const uint32_t Jet = 0x00000002;
        const uint32_t DWI = 0x00000003;

        const size_t NumSpecial = 2;
        const uint32_t Special = 0x00000080;
        const uint32_t RGB = Special;
        const uint32_t Complex = Special+1;

        inline void init (QWidget* window, QActionGroup*& group, QMenu* menu, QAction** & actions)
        {
          group = new QActionGroup (window);
          group->setExclusive (true);
          actions = new QAction* [NumScalar+NumSpecial];

          size_t n = 0;
          actions[n++] = new QAction ("Gray", window);
          actions[n++] = new QAction ("Hot", window);
          actions[n++] = new QAction ("Jet", window);
          actions[n++] = new QAction ("DWI", window);

          actions[n++] = new QAction ("RGB", window);
          actions[n++] = new QAction ("Complex", window);

          for (n = 0; n < NumScalar; ++n) {
            actions[n]->setCheckable (true);
            group->addAction (actions[n]);
            menu->addAction (actions[n]);
          }
          menu->addSeparator();
          for (; n < NumScalar+NumSpecial; ++n) {
            actions[n]->setCheckable (true);
            group->addAction (actions[n]);
            menu->addAction (actions[n]);
          }
          actions[0]->setChecked (true);

          for (n = 0; n < NumScalar+NumSpecial; ++n) {
            window->addAction (actions[n]);
            actions[n]->setShortcut (QObject::tr (std::string ("Ctrl+" + str (n+1)).c_str()));
          }

        }

        inline uint32_t from_menu (uint32_t num)
        {
          if (num < NumScalar) return num;
          return num-NumScalar+Special;
        }

      }

      class Shader
      {
        public:
          Shader () : 
            lessthan (NAN),
            greaterthan (NAN),
            display_midpoint (NAN),
            display_range (NAN),
            transparent_intensity (NAN),
            opaque_intensity (NAN),
            alpha (NAN),
            flags_ (ColourMap::Mask) { }

          virtual ~Shader() {}

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

          void start (float scaling = 1.0) {
            shader_program.start();
            shader_program.get_uniform ("offset") = (display_midpoint - 0.5f * display_range) / scaling;
            shader_program.get_uniform ("scale") = scaling / display_range;
            if (flags_ & DiscardLower) shader_program.get_uniform ("lower") = lessthan / scaling;
            if (flags_ & DiscardUpper) shader_program.get_uniform ("upper") = greaterthan / scaling;
            if (flags_ & Transparency) {
              shader_program.get_uniform ("alpha_scale") = scaling / (opaque_intensity - transparent_intensity);
              shader_program.get_uniform ("alpha_offset") = transparent_intensity / scaling;
              shader_program.get_uniform ("alpha") = alpha;
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

          void set_colourmap (uint32_t index) {
            uint32_t cmap = flags_ & ~(ColourMap::Mask);
            cmap |= index;
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

          uint32_t colourmap () const {
            return flags_ & ColourMap::Mask;
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

          uint32_t colourmap_index () const {
            uint32_t cret = flags_ & ColourMap::Mask;
            if (cret >= ColourMap::Special)
              cret -= ColourMap::Special - ColourMap::NumScalar;
            return cret;
          }


        protected:
          uint32_t flags_;

          GL::Shader::Fragment fragment_shader;
          GL::Shader::Vertex vertex_shader;
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

          virtual void recompile ();
      };

    }
  }
}

#endif



