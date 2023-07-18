/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

      void ComboBoxWithErrorMsg::clearError (int new_index)
      {
        if (error_index == -1 || new_index == error_index)
          return;
        // Delete the extra element
        removeItem (error_index);
        error_index = -1;
      }


      void ComboBoxWithErrorMsg::onSetIndex (int new_index)
      {
        clearError (new_index);
      }


    }
  }
}

