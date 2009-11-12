/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/01/09.

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

#ifndef __dwi_render_window_h__
#define __dwi_render_window_h__

#include <QObject>
#include <QApplication>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QAction>

#include "file/path.h"
#include "math/SH.h"

namespace MR {
  namespace Dialog { class Lighting; }
  namespace DWI {

    class RenderFrame;

    class Window : public QMainWindow 
    {
      Q_OBJECT

      public:
        Window (const std::string& title, const Math::Matrix<float>& coefs);

      protected slots:
        void open_slot ();
        void close_slot ();
        void use_lighting_slot (bool is_checked);
        void show_axes_slot (bool is_checked);
        void hide_negative_lobes_slot (bool is_checked);
        void colour_by_direction_slot (bool is_checked);
        void normalise_slot (bool is_checked);
        void previous_slot ();
        void next_slot ();
        void previous_10_slot ();
        void next_10_slot ();
        void lmax_slot ();
        void lod_slot ();
        void screenshot_slot ();
        void lmax_inc_slot ();
        void lmax_dec_slot ();
        void advanced_lighting_slot ();

      protected:
        RenderFrame* render_frame;
        Dialog::Lighting* lighting_dialog;
        QActionGroup *lod_group, *lmax_group, *screenshot_OS_group;

        std::string name;
        int current;
        const Math::Matrix<float>& values;

        void set_values (int row);
    };


  }
}


#endif

