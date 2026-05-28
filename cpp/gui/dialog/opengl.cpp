/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "dialog/opengl.h"
#include "dialog/list.h"
#include "opengl/glutils.h"
#include <fmt/format.h>

namespace MR::GUI::Dialog {

OpenGL::OpenGL(QWidget *parent, const GL::Format &format) : QDialog(parent) {
  TreeModel *model = new TreeModel(this);

  TreeItem *root = model->rootItem;

  GLint major_version;
  gl::GetIntegerv(gl::MAJOR_VERSION, &major_version);
  GLint minor_version;
  gl::GetIntegerv(gl::MINOR_VERSION, &minor_version);

  root->appendChild(new TreeItem("API version", fmt::format("{}.{}", major_version, minor_version), root));
  root->appendChild(new TreeItem("Renderer", (const char *)gl::GetString(gl::RENDERER), root));
  root->appendChild(new TreeItem("Vendor", (const char *)gl::GetString(gl::VENDOR), root));
  root->appendChild(new TreeItem("Version", (const char *)gl::GetString(gl::VERSION), root));

  TreeItem *bit_depths = new TreeItem("Bit depths", std::string(), root);
  root->appendChild(bit_depths);

  bit_depths->appendChild(new TreeItem("red", fmt::format("{}", format.redBufferSize()), bit_depths));
  bit_depths->appendChild(new TreeItem("green", fmt::format("{}", format.greenBufferSize()), bit_depths));
  bit_depths->appendChild(new TreeItem("blue", fmt::format("{}", format.blueBufferSize()), bit_depths));
  bit_depths->appendChild(new TreeItem("alpha", fmt::format("{}", format.alphaBufferSize()), bit_depths));
  bit_depths->appendChild(new TreeItem("depth", fmt::format("{}", format.depthBufferSize()), bit_depths));
  bit_depths->appendChild(new TreeItem("stencil", fmt::format("{}", format.stencilBufferSize()), bit_depths));

  root->appendChild(new TreeItem("Buffering",
                                 format.swapBehavior() == QSurfaceFormat::SingleBuffer
                                     ? "single"
                                     : (format.swapBehavior() == QSurfaceFormat::DoubleBuffer ? "double" : "triple"),
                                 root));
  root->appendChild(new TreeItem("VSync", format.swapInterval() ? "on" : "off", root));
  root->appendChild(new TreeItem(
      "Multisample anti-aliasing", format.samples() ? fmt::format("{}", format.samples()).c_str() : "off", root));

  GLint max_2d_texture_size;
  gl::GetIntegerv(gl::MAX_TEXTURE_SIZE, &max_2d_texture_size);
  root->appendChild(new TreeItem("Maximum 2D texture size", fmt::format("{}", max_2d_texture_size), root));

  GLint max_3D_texture_size;
  gl::GetIntegerv(gl::MAX_3D_TEXTURE_SIZE, &max_3D_texture_size);
  root->appendChild(new TreeItem("Maximum 3D texture size", fmt::format("{}", max_3D_texture_size), root));

  QTreeView *view = new QTreeView;
  view->setModel(model);
  view->resizeColumnToContents(0);
  view->resizeColumnToContents(1);
  view->setMinimumSize(500, 200);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(view);
  layout->addWidget(buttonBox);
  setLayout(layout);

  setWindowTitle(tr("OpenGL information"));
  setSizeGripEnabled(true);
  adjustSize();
}

} // namespace MR::GUI::Dialog
