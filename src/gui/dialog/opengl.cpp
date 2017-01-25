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



#include "gui/dialog/list.h"
#include "gui/dialog/opengl.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      OpenGL::OpenGL (QWidget* parent, const GL::Format& format) : QDialog (parent)
      {
        TreeModel* model = new TreeModel (this);

        TreeItem* root = model->rootItem;

        GLint i;
        gl::GetIntegerv (gl::MAJOR_VERSION, &i);
        std::string text = str(i) + ".";
        gl::GetIntegerv (gl::MINOR_VERSION, &i);
        text += str(i);

        root->appendChild (new TreeItem ("API version", text, root));
        root->appendChild (new TreeItem ("Renderer", (const char*) gl::GetString (gl::RENDERER), root));
        root->appendChild (new TreeItem ("Vendor", (const char*) gl::GetString (gl::VENDOR), root));
        root->appendChild (new TreeItem ("Version", (const char*) gl::GetString (gl::VERSION), root));

        TreeItem* bit_depths = new TreeItem ("Bit depths", std::string(), root);
        root->appendChild (bit_depths);

        bit_depths->appendChild (new TreeItem ("red", str (format.redBufferSize()), bit_depths));
        bit_depths->appendChild (new TreeItem ("green", str (format.greenBufferSize()), bit_depths));
        bit_depths->appendChild (new TreeItem ("blue", str (format.blueBufferSize()), bit_depths));
        bit_depths->appendChild (new TreeItem ("alpha", str (format.alphaBufferSize()), bit_depths));
        bit_depths->appendChild (new TreeItem ("depth", str (format.depthBufferSize()), bit_depths));
        bit_depths->appendChild (new TreeItem ("stencil", str (format.stencilBufferSize()), bit_depths));

#if QT_VERSION >= 0x050400
        root->appendChild (new TreeItem ("Buffering", format.swapBehavior() == QSurfaceFormat::SingleBuffer ? "single" :
               ( format.swapBehavior() == QSurfaceFormat::DoubleBuffer ? "double" : "triple" ), root));
#else
        root->appendChild (new TreeItem ("Buffering", format.doubleBuffer() ? "double" : "single", root));
#endif
        root->appendChild (new TreeItem ("VSync", format.swapInterval() ? "on" : "off", root));
        root->appendChild (new TreeItem ("Multisample anti-aliasing", format.samples() ? str(format.samples()).c_str() : "off", root));

        gl::GetIntegerv (gl::MAX_TEXTURE_SIZE, &i);
        root->appendChild (new TreeItem ("Maximum texture size", str (i), root));

        QTreeView* view = new QTreeView;
        view->setModel (model);
        view->resizeColumnToContents (0);
        view->resizeColumnToContents (1);
        view->setMinimumSize (500, 200);

        QDialogButtonBox* buttonBox = new QDialogButtonBox (QDialogButtonBox::Ok);
        connect (buttonBox, SIGNAL (accepted()), this, SLOT (accept()));

        QVBoxLayout* layout = new QVBoxLayout (this);
        layout->addWidget (view);
        layout->addWidget (buttonBox);
        setLayout (layout);

        setWindowTitle (tr ("OpenGL information"));
        setSizeGripEnabled (true);
        adjustSize();
      }


    }
  }
}

