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

      OpenGL::OpenGL (QWidget* parent) : QDialog (parent)
      {
        TreeModel* model = new TreeModel (this);

        TreeItem* root = model->rootItem;

        std::string text;
        if (GLEE_VERSION_3_0) text = "3.0";
        else if (GLEE_VERSION_2_1) text = "2.1";
        else if (GLEE_VERSION_2_0) text = "2.0";
        else if (GLEE_VERSION_1_5) text = "1.5";
        else if (GLEE_VERSION_1_4) text = "1.4";
        else if (GLEE_VERSION_1_3) text = "1.3";
        else if (GLEE_VERSION_1_2) text = "1.2";
        else text = "1.1";

        root->appendChild (new TreeItem ("API version", text, root));
        root->appendChild (new TreeItem ("Renderer", (const char*) glGetString (GL_RENDERER), root));
        root->appendChild (new TreeItem ("Vendor", (const char*) glGetString (GL_VENDOR), root));
        root->appendChild (new TreeItem ("Version", (const char*) glGetString (GL_VERSION), root));

        TreeItem* extensions = new TreeItem ("Extensions", std::string(), root);
        root->appendChild (extensions);

        std::vector<std::string> ext = split ( (const char*) glGetString (GL_EXTENSIONS));
        for (size_t n = 0; n < ext.size(); ++n)
          extensions->appendChild (new TreeItem (std::string(), ext[n], extensions));

        TreeItem* bit_depths = new TreeItem ("Bit depths", std::string(), root);
        root->appendChild (bit_depths);

        GLint i;
        glGetIntegerv (GL_RED_BITS, &i);
        bit_depths->appendChild (new TreeItem ("red", str (i), bit_depths));
        glGetIntegerv (GL_GREEN_BITS, &i);
        bit_depths->appendChild (new TreeItem ("green", str (i), bit_depths));
        glGetIntegerv (GL_BLUE_BITS, &i);
        bit_depths->appendChild (new TreeItem ("blue", str (i), bit_depths));
        glGetIntegerv (GL_ALPHA_BITS, &i);
        bit_depths->appendChild (new TreeItem ("alpha", str (i), bit_depths));
        glGetIntegerv (GL_DEPTH_BITS, &i);
        bit_depths->appendChild (new TreeItem ("depth", str (i), bit_depths));
        glGetIntegerv (GL_STENCIL_BITS, &i);
        bit_depths->appendChild (new TreeItem ("stencil", str (i), bit_depths));

        TreeItem* buffers = new TreeItem ("Buffers", std::string(), root);
        root->appendChild (buffers);
        glGetIntegerv (GL_DOUBLEBUFFER, &i);
        buffers->appendChild (new TreeItem ("Double buffering", i ? "on" : "off", buffers));
        glGetIntegerv (GL_STEREO, &i);
        buffers->appendChild (new TreeItem ("Stereo buffering", i ? "on" : "off", buffers));
        glGetIntegerv (GL_AUX_BUFFERS, &i);
        buffers->appendChild (new TreeItem ("Auxilliary buffers", str (i), buffers));


        glGetIntegerv (GL_MAX_TEXTURE_SIZE, &i);
        root->appendChild (new TreeItem ("Maximum texture size", str (i), root));
        glGetIntegerv (GL_MAX_LIGHTS, &i);
        root->appendChild (new TreeItem ("Maximum number of lights", str (i), root));
        glGetIntegerv (GL_MAX_CLIP_PLANES, &i);
        root->appendChild (new TreeItem ("Maximum number of clip planes", str (i), root));


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

