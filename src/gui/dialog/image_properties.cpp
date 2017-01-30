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


#include "header.h"
#include "stride.h"
#include "math/math.h"
#include "dwi/gradient.h"
#include "gui/dialog/file.h"
#include "gui/dialog/list.h"
#include "gui/dialog/image_properties.h"


namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      ImageProperties::ImageProperties (QWidget* parent, const MR::Header& header) :
        QDialog (parent), H (header), save_data (0, 0)
      {
        model = new TreeModel (this);

        TreeItem* root = model->rootItem;

        root->appendChild (new TreeItem ("File", H.name(), root));
        assert (H.format());
        root->appendChild (new TreeItem ("Format", H.format(), root));

        if (H.keyval().size()) {
          TreeItem* keyvals = new TreeItem ("Key/value pairs", std::string(), root);
          root->appendChild (keyvals);
          for (auto n : H.keyval()) {
            if (n.first != "dw_scheme") {
              if (n.second.find ('\n') == n.second.npos) {
                keyvals->appendChild (new TreeItem (n.first, n.second, keyvals));
              } else {
                const auto lines = split_lines (n.second);
                TreeItem* multi_line_keyval = new TreeItem (n.first, std::string(), keyvals);
                keyvals->appendChild (multi_line_keyval);
                for (auto l : lines)
                  multi_line_keyval->appendChild (new TreeItem (std::string(), l, multi_line_keyval));
              }
            }
          }
        }

        std::string text;
        text = str (H.size (0));
        for (size_t n = 1; n < H.ndim(); ++n)
          text += " x " + str (H.size (n));
        root->appendChild (new TreeItem ("Dimensions", text, root));

        text = str (H.spacing (0));
        for (size_t n = 1; n < H.ndim(); ++n)
          text += " x " + str (H.spacing (n));
        root->appendChild (new TreeItem ("Voxel size", text, root));

        root->appendChild (new TreeItem ("Data type", H.datatype().description(), root));

        MR::Stride::List strides = MR::Stride::get_symbolic (H);
        text = str (strides[0]);
        for (size_t n = 1; n != strides.size(); ++n)
          text += ", " + str (strides[n]);
        root->appendChild (new TreeItem ("Strides", text, root));

        root->appendChild (new TreeItem ("Data scaling",
                                         "offset: " + str (H.intensity_offset()) + ", multiplier = " + str (H.intensity_scale()), root));

        Eigen::IOFormat Fmt (6, 0, ", ", "\n", "[", "]");
        TreeItem* transform = new TreeItem ("Transform", std::string(), root);
        root->appendChild (transform);
        for (size_t n = 0; n < 3; ++n) {
          std::stringstream ss;
          ss << H.transform().matrix().row (n).format (Fmt);
          transform->appendChild (new TreeItem (std::string(), ss.str(), transform));
        }

        auto DW_scheme = DWI::parse_DW_scheme (H);
        if (DW_scheme.rows()) {
          if (DW_scheme.cols() < 4) {
            root->appendChild (new TreeItem ("Diffusion scheme", "(invalid)", root));
          }
          else {
            TreeItem* scheme = new TreeItem ("Diffusion scheme", std::string(), root);
            root->appendChild (scheme);
            for (size_t n = 0; n < size_t(DW_scheme.rows()); ++n) {
              std::stringstream ss;
              ss << DW_scheme.row (n).format (Fmt);
              scheme->appendChild (new TreeItem (std::string(), ss.str(), scheme));
            }
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

        if (text == "Transform") save_data = H.transform().matrix();
        else if (text == "Diffusion scheme") save_data = DWI::parse_DW_scheme (H);
        else {
          save_data.resize (0, 0);
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
        assert (save_data.rows());
        std::string name = File::get_save_name (this, "Save as...", "dwgrad.txt");
        if (name.size())
          MR::save_matrix (save_data, name);
      }

    }
  }
}


