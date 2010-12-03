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

#ifndef __viewer_shader_h__
#define __viewer_shader_h__

#include <QAction>
#include <QActionGroup>
#include <QMenu>

#include "opengl/gl.h"
#include "opengl/shader.h"

#ifdef Complex
# undef Complex
#endif

namespace MR {
  namespace Viewer {

    const uint32_t Texture2D = 0x00000000;
    const uint32_t Texture3D = 0x80000000;

    const uint32_t Invert = 0x10000000;
    const uint32_t DiscardLower = 0x20000000;
    const uint32_t DiscardUpper = 0x40000000;

    namespace ColourMap {
      const uint32_t Mask = 0x000000FF;
      const uint32_t MaskNonScalar = 0x00000080;

      const size_t NumScalar = 3;
      const uint32_t Gray = 0x00000000;
      const uint32_t Hot = 0x00000001;
      const uint32_t Jet = 0x00000002;

      const size_t NumSpecial = 2;
      const uint32_t Special = 0x00000080;
      const uint32_t RGB = Special;
      const uint32_t Complex = Special+1;

      inline void init (QObject* window, QActionGroup*& group, QMenu* menu, QAction**& actions) 
      {
        group = new QActionGroup (window);
        group->setExclusive (true);
        actions = new QAction* [NumScalar+NumSpecial];

        size_t n = 0;
        actions[n++] = new QAction ("Gray", window);
        actions[n++] = new QAction ("Hot", window);
        actions[n++] = new QAction ("Jet", window);

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
        bool operator! () const { return !shader_program; }
        void set (uint32_t flags);

        void start () { shader_program.start(); }
        void stop () { shader_program.stop(); }

        GL::Shader::Uniform get_uniform (const std::string& name) { return shader_program.get_uniform (name); }

      protected:
        GL::Shader::Fragment fragment_shader;
        GL::Shader::Program shader_program;

        static GL::Shader::Vertex vertex_shader;
        static const char* vertex_shader_source;
    };

  }
}

#endif



