/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __gui_mrview_combo_box_error_h__
#define __gui_mrview_combo_box_error_h__

#include "mrtrix.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      class ComboBoxWithErrorMsg : public QComboBox
      { NOMEMALIGN
        Q_OBJECT

        public:
          ComboBoxWithErrorMsg (QWidget* parent, const QString& msg);

          void setError ();
          void clearError (int);
          void clearError() { clearError (-1); } // Guaranteed to clear the error entry

        protected slots:
          void onSetIndex (int);


        protected:
          const QString error_message;
          int error_index;

      };


    }
  }
}

#endif

