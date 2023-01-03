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

