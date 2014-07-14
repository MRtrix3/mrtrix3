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
      {
        Q_OBJECT

        public:
          ComboBoxWithErrorMsg (QWidget* parent, const QString& msg);

          void setError ();


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

