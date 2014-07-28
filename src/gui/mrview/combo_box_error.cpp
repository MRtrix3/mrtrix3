/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

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

#include "gui/mrview/combo_box_error.h"
#include "math/math.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      ComboBoxWithErrorMsg::ComboBoxWithErrorMsg (QWidget* parent, const QString& msg) :
          QComboBox (parent),
          error_message (msg),
          error_index (-1)
      {
        connect (this, SIGNAL (currentIndexChanged(int)), SLOT (onSetIndex(int)));
      }


      void ComboBoxWithErrorMsg::setError()
      {
        if (error_index >= 0) {
          setCurrentIndex (error_index);
          return;
        }
        // Create the extra element, and set the current index to activate it
        error_index = count();
        addItem (error_message);
        setCurrentIndex (error_index);
      }


      void ComboBoxWithErrorMsg::onSetIndex (int new_index)
      {
        if (error_index == -1 || new_index == error_index)
          return;
        // Delete the extra element
        removeItem (error_index);
        error_index = -1;
      }


    }
  }
}

