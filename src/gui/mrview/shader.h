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

        inline void init (QObject* window, QActionGroup*& group, QMenu* menu, QAction** & actions)
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
          Shader () : flags_ (ColourMap::Mask) { }

          bool operator! () const {
            return !shader_program;
          }
          void set (uint32_t new_flags) {
            if (new_flags != flags_) {
              flags_ = new_flags;
              recompile();
            }
          }

          void start (float display_midpoint, float display_range, float lessthan, float greaterthan,
              float transparent_intensity, float opaque_intensity, float alpha) {
            shader_program.start();
            shader_program.get_uniform ("offset") = display_midpoint - 0.5f * display_range;
            shader_program.get_uniform ("scale") = 1.0f / display_range;
            if (flags_ & DiscardLower) shader_program.get_uniform ("lower") = lessthan;
            if (flags_ & DiscardUpper) shader_program.get_uniform ("upper") = greaterthan;
            if (flags_ & Transparency) {
              shader_program.get_uniform ("alpha_scale") = 1.0f / (opaque_intensity - transparent_intensity);
              shader_program.get_uniform ("alpha_offset") = transparent_intensity;
              shader_program.get_uniform ("alpha") = alpha;
            }
          }
          void stop () {
            shader_program.stop();
          }

          uint32_t flags () const { return flags_; }

          void set_colourmap (uint32_t index) {
            uint32_t cmap = flags_ & ~(ColourMap::Mask);
            cmap |= index;
            set (cmap);
          }

          void set_use_thresholds (bool lessthan, bool greaterthan) {
            uint32_t cmap = flags_;
            set_bit (cmap, DiscardLower, lessthan);
            set_bit (cmap, DiscardUpper, greaterthan);
            set (cmap);
          }

          void set_use_transparency (bool value) {
            uint32_t cmap = flags_;
            set_bit (cmap, Transparency, value);
            set (cmap);
          }

          void set_use_lighting (bool value) {
            uint32_t cmap = flags_;
            set_bit (cmap, Lighting, value);
            set (cmap);
          }

          void set_invert_map (bool value) {
            uint32_t cmap = flags_;
            set_bit (cmap, InvertMap, value);
            set (cmap);
          }

          void set_invert_scale (bool value) {
            uint32_t cmap = flags_;
            set_bit (cmap, InvertScale, value);
            set (cmap);
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

          void recompile ();
      };

    }
  }
}

#endif



