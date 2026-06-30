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

#pragma once

#include <QImage>

#include "opengl/glutils.h"

namespace MR::GUI::MRView {

//! Off-screen render target used by the screen-capture tool to produce super-resolution images.
/*! The visualisation window is rendered into this framebuffer at a multiple of its on-screen
 *  resolution, then read back to a QImage for export, leaving the visible window untouched.
 *
 *  The buffer is allocated lazily and re-used across successive captures (e.g. animation frames)
 *  whenever the requested dimensions and sample count are unchanged.
 *
 *  When \a samples exceeds one, a multi-sample colour+depth renderbuffer is rendered into and
 *  blit-resolved into a single-sample texture prior to read-back, yielding multi-sample
 *  anti-aliasing. With \a samples == 1 a single-sample colour texture is rendered into directly. */
class CaptureBuffer {
public:
  CaptureBuffer() = default;

  //! allocate (or re-use) the off-screen render target with the requested dimensions and sample count
  void ensure(GLsizei width, GLsizei height, GLsizei samples = 1);

  void bind() { framebuffer.bind(); }
  void unbind() { framebuffer.unbind(); }

  //! read the colour attachment back into a vertically-flipped RGBA image
  QImage read();

  GLsizei width() const { return width_; }
  GLsizei height() const { return height_; }

private:
  GLsizei width_{0};
  GLsizei height_{0};
  GLsizei samples_{0};

  // Render target: single-sample colour texture (samples == 1), or multi-sample colour renderbuffer
  // resolved into a single-sample texture on a separate framebuffer (samples > 1).
  GL::FrameBuffer framebuffer;
  GL::RenderBuffer depth;
  GL::Texture colour;
  GL::RenderBuffer colour_multisample;
  GL::FrameBuffer resolve_framebuffer;
  GL::Texture resolve_colour;

  void allocate();
};

} // namespace MR::GUI::MRView
