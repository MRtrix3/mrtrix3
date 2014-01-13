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


#include "gui/dialog/list.h"
#include "gui/dialog/opengl.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      OpenGL::OpenGL (QWidget* parent, const QGLFormat& format) : QDialog (parent)
      {
        TreeModel* model = new TreeModel (this);

        TreeItem* root = model->rootItem;

        GLint i;
        glGetIntegerv (GL_MAJOR_VERSION, &i);
        std::string text = str(i) + ".";
        glGetIntegerv (GL_MINOR_VERSION, &i);
        text += str(i);

        root->appendChild (new TreeItem ("API version", text, root));
        root->appendChild (new TreeItem ("Renderer", (const char*) glGetString (GL_RENDERER), root));
        root->appendChild (new TreeItem ("Vendor", (const char*) glGetString (GL_VENDOR), root));
        root->appendChild (new TreeItem ("Version", (const char*) glGetString (GL_VERSION), root));

        TreeItem* bit_depths = new TreeItem ("Bit depths", std::string(), root);
        root->appendChild (bit_depths);

        bit_depths->appendChild (new TreeItem ("red", str (format.redBufferSize()), bit_depths));
        bit_depths->appendChild (new TreeItem ("green", str (format.greenBufferSize()), bit_depths));
        bit_depths->appendChild (new TreeItem ("blue", str (format.blueBufferSize()), bit_depths));
        bit_depths->appendChild (new TreeItem ("alpha", str (format.alphaBufferSize()), bit_depths));
        bit_depths->appendChild (new TreeItem ("depth", str (format.depthBufferSize()), bit_depths));
        bit_depths->appendChild (new TreeItem ("stencil", str (format.stencilBufferSize()), bit_depths));

        root->appendChild (new TreeItem ("Double buffering", format.doubleBuffer() ? "on" : "off", root));
        root->appendChild (new TreeItem ("VSync", format.swapInterval() ? "on" : "off", root));
        root->appendChild (new TreeItem ("Multisample anti-aliasing", format.sampleBuffers() ? str(format.samples()).c_str() : "off", root));

        glGetIntegerv (GL_MAX_TEXTURE_SIZE, &i);
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

