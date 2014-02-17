/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#include "image/header.h"
#include "gui/dialog/file.h"
#include "gui/dialog/list.h"
#include "gui/dialog/image_properties.h"


namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      ImageProperties::ImageProperties (QWidget* parent, const Image::Header& header) :
        QDialog (parent), H (header), save_target (NULL)
      {
        model = new TreeModel (this);

        TreeItem* root = model->rootItem;

        root->appendChild (new TreeItem ("File", H.name(), root));
        root->appendChild (new TreeItem ("Format", H.format(), root));

        if (H.comments().size()) {
          TreeItem* comments = new TreeItem ("Comments", std::string(), root);
          root->appendChild (comments);
          for (size_t n = 0; n < H.comments().size(); ++n)
            comments->appendChild (new TreeItem (std::string(), H.comments() [n], comments));
        }

        std::string text;
        text = str (H.dim (0));
        for (size_t n = 1; n < H.ndim(); ++n)
          text += " x " + str (H.dim (n));
        root->appendChild (new TreeItem ("Dimensions", text, root));

        text = str (H.vox (0));
        for (size_t n = 1; n < H.ndim(); ++n)
          text += " x " + str (H.vox (n));
        root->appendChild (new TreeItem ("Voxel size", text, root));

        root->appendChild (new TreeItem ("Data type", H.datatype().description(), root));

        text = str (H.stride (0));
        for (size_t n = 1; n < H.ndim(); ++n)
          text += ", " + str (H.stride (n));
        root->appendChild (new TreeItem ("Strides", text, root));

        root->appendChild (new TreeItem ("Data scaling",
                                         "offset: " + str (H.intensity_offset()) + ", multiplier = " + str (H.intensity_scale()), root));

        if (H.transform().rows() != 4 || H.transform().columns() != 4) {
          root->appendChild (new TreeItem ("Transform", "(invalid)", root));
        }
        else {
          TreeItem* transform = new TreeItem ("Transform", std::string(), root);
          root->appendChild (transform);
          for (size_t n = 0; n < 4; ++n)
            transform->appendChild (new TreeItem (std::string(),
                                                  str (H.transform() (n,0)) + ", " +
                                                  str (H.transform() (n,1)) + ", " +
                                                  str (H.transform() (n,2)) + ", " +
                                                  str (H.transform() (n,3)),
                                                  transform));
        }

        if (H.DW_scheme().is_set()) {
          if (H.DW_scheme().rows() == 0 || H.DW_scheme().columns() != 4) {
            root->appendChild (new TreeItem ("Diffusion scheme", "(invalid)", root));
          }
          else {
            TreeItem* scheme = new TreeItem ("Diffusion scheme", std::string(), root);
            root->appendChild (scheme);
            for (size_t n = 0; n < H.DW_scheme().rows(); ++n)
              scheme->appendChild (new TreeItem (std::string(),
                                                 str (H.DW_scheme() (n,0)) + ", " +
                                                 str (H.DW_scheme() (n,1)) + ", " +
                                                 str (H.DW_scheme() (n,2)) + ", " +
                                                 str (H.DW_scheme() (n,3)),
                                                 scheme));
          }
        }


        view = new QTreeView;
        view->setModel (model);
        view->resizeColumnToContents (0);
        view->resizeColumnToContents (1);
        view->setMinimumSize (500, 200);
        view->setContextMenuPolicy (Qt::CustomContextMenu);
        connect (view, SIGNAL (customContextMenuRequested (const QPoint&)),
                 this, SLOT (context_menu (const QPoint&)));

        QDialogButtonBox* buttonBox = new QDialogButtonBox (QDialogButtonBox::Ok);
        connect (buttonBox, SIGNAL (accepted()), this, SLOT (accept()));

        QVBoxLayout* layout = new QVBoxLayout (this);
        layout->addWidget (view);
        layout->addWidget (buttonBox);
        setLayout (layout);

        setWindowTitle (tr ("Image Properties"));
        setSizeGripEnabled (true);
        adjustSize();
      }

      void ImageProperties::context_menu (const QPoint& point)
      {
        QModelIndex k = view->indexAt (point);
        if (!k.isValid()) return;
        k = k.sibling (k.row(), 0);

        while (k.parent().isValid()) k = k.parent();
        std::string text = k.data().toString().toUtf8().constData();

        if (text == "Transform") save_target = &H.transform();
        else if (text == "Diffusion scheme") save_target = &H.DW_scheme();
        else {
          save_target = NULL;
          return;
        }

        QAction* save_action = new QAction (tr ("&Save as..."), this);
        connect (save_action, SIGNAL (triggered()), this, SLOT (write_to_file()));

        QMenu menu (this);
        menu.addAction (save_action);
        menu.exec (view->viewport()->mapToGlobal (point));
      }




      void ImageProperties::write_to_file ()
      {
        assert (save_target);
        std::string name = File::get_save_name (this, "Save as...");
        if (name.size())
          save_target->save (name);
      }

    }
  }
}


