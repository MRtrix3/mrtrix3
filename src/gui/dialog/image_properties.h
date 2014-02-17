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

