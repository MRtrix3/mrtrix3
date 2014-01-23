/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __gui_dialog_image_properties_h__
#define __gui_dialog_image_properties_h__

#include "math/matrix.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace Image
  {
    class Header;
  }

  namespace GUI
  {
    namespace Dialog
    {
      class TreeModel;

      class ImageProperties : public QDialog
      {
          Q_OBJECT

        public:
          ImageProperties (QWidget* parent, const Image::Header& header);

        private slots:
          void context_menu (const QPoint& point);
          void write_to_file ();

        private:
          const Image::Header& H;
          QTreeView* view;
          TreeModel* model;
          const Math::Matrix<float>* save_target;
      };

    }
  }
}

#endif

