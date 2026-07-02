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

#include "mrview/capture_buffer.h"

#include <cassert>

#include <GL/gl.h>
#include <QImage>
#include <qglobal.h>

#include "opengl/glutils.h"

namespace MR::GUI::MRView {

void CaptureBuffer::ensure(GLsizei width, GLsizei height, GLsizei samples) {
  if (static_cast<GLuint>(framebuffer) != 0 && width == width_ && height == height_ && samples == samples_)
    return;
  width_ = width;
  height_ = height;
  samples_ = samples;
  allocate();
}

void CaptureBuffer::allocate() {
  assert(samples_ >= 1);

  // RenderBuffer::set_storage*()/Texture::gen() are no-ops on the object handle once it exists, so a
  // re-allocation at a different size or sample count simply re-specifies the storage of existing objects.
  framebuffer.gen();
  depth.gen();

  if (samples_ > 1) {
    // Multi-sample render target: colour + depth renderbuffers, resolved into a single-sample texture.
    colour_multisample.gen();
    colour_multisample.set_storage_multisample(samples_, gl::RGBA8, width_, height_);
    depth.set_storage_multisample(samples_, gl::DEPTH_COMPONENT24, width_, height_);
    framebuffer.attach_color(colour_multisample, 0);
    framebuffer.attach_depth(depth);
    framebuffer.draw_buffers(0);
    framebuffer.check();
    framebuffer.unbind();

    resolve_framebuffer.gen();
    resolve_colour.gen(gl::TEXTURE_2D, gl::NEAREST);
    resolve_colour.bind();
    gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA8, width_, height_, 0, gl::RGBA, gl::UNSIGNED_BYTE, nullptr);
    resolve_framebuffer.attach_color(resolve_colour, 0);
    resolve_framebuffer.draw_buffers(0);
    resolve_framebuffer.check();
    resolve_framebuffer.unbind();
  } else {
    // Single-sample render target: colour texture + depth renderbuffer.
    colour.gen(gl::TEXTURE_2D, gl::NEAREST);
    colour.bind();
    gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA8, width_, height_, 0, gl::RGBA, gl::UNSIGNED_BYTE, nullptr);
    depth.set_storage(gl::DEPTH_COMPONENT24, width_, height_);
    framebuffer.attach_color(colour, 0);
    framebuffer.attach_depth(depth);
    framebuffer.draw_buffers(0);
    framebuffer.check();
    framebuffer.unbind();
  }
}

QImage CaptureBuffer::read() {
  if (samples_ > 1) {
    // Resolve the multi-sample colour buffer into the single-sample texture (sizes equal => GL_NEAREST).
    framebuffer.bind_read();
    resolve_framebuffer.bind_draw();
    gl::BlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, gl::COLOR_BUFFER_BIT, gl::NEAREST);
    resolve_framebuffer.bind();
  } else {
    framebuffer.bind();
  }
  gl::ReadBuffer(gl::COLOR_ATTACHMENT0);
  QImage image(width_, height_, QImage::Format_RGBA8888);
  gl::PixelStorei(gl::PACK_ALIGNMENT, 1);
  gl::ReadPixels(0, 0, width_, height_, gl::RGBA, gl::UNSIGNED_BYTE, image.bits());
  // OpenGL's origin is bottom-left; flip to raster (top-left) orientation.
  // QImage::flipped() supersedes the deprecated QImage::mirrored() from Qt 6.9 onwards.
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
  return image.flipped(Qt::Vertical);
#else
  return image.mirrored(false, true);
#endif
}

} // namespace MR::GUI::MRView
