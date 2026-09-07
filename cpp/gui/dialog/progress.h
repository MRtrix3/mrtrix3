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

#include "opengl/glutils.h"
#include "progressbar.h"

namespace MR::GUI::Dialog::ProgressBar {

void display(const ::MR::ProgressBar &p);
void done(const ::MR::ProgressBar &p);

//! An MR::ProgressBar whose dialog offers a "Cancel" button.
/*! Constructing this type instead of a plain MR::ProgressBar is the only way to opt in to
 *  cancellation support: MR::ProgressBar itself declares no such concept anywhere in its
 *  interface. display() detects this type via dynamic_cast and, if the user clicks "Cancel",
 *  records that fact directly on this instance. Because the state lives on the object itself
 *  rather than in shared/static storage, it cannot leak onto an unrelated dialog regardless of
 *  whether this instance's own dialog is suppressed (e.g. under -quiet), never reaches the
 *  on-screen delay, or is destroyed having never displayed at all. */
class Cancellable : public ::MR::ProgressBar {
public:
  using ::MR::ProgressBar::ProgressBar;

  bool cancelled() const { return cancelled_; }

private:
  friend void display(const ::MR::ProgressBar &p);
  mutable bool cancelled_ = false;
};

} // namespace MR::GUI::Dialog::ProgressBar
