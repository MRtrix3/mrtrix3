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

        root->appendChild (new TreeItem ("Double buffering", format.doubleBuffer() ? "on" : "off", root));
        root->appendChild (new TreeItem ("VSync", format.swapInterval() ? "on" : "off", root));
        root->appendChild (new TreeItem ("Multisample anti-aliasing", format.sampleBuffers() ? str(format.samples()).c_str() : "off", root));

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

