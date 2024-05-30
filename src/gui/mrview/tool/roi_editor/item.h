/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include <array>

#include "algo/loop.h"
#include "gui/mrview/tool/roi_editor/undoentry.h"
#include "gui/mrview/volume.h"
#include "header.h"
#include "types.h"

namespace MR::GUI::MRView::Tool {

namespace {
constexpr std::array<std::array<GLubyte, 3>, 6> preset_colours = {
    {{{255, 255, 0}}, {{255, 0, 255}}, {{0, 255, 255}}, {{255, 0, 0}}, {{0, 255, 255}}, {{0, 0, 255}}}};
}

class ROI_Item : public Volume {
public:
  ROI_Item(MR::Header &&);

  void zero();
  void load();

  template <class ImageType> void save(ImageType &&, GLubyte *);

  bool has_undo() { return current_undo >= 0; }
  bool has_redo() { return current_undo < int(undo_list.size() - 1); }

  ROI_UndoEntry &current() { return undo_list[current_undo]; }
  void start(ROI_UndoEntry &&entry);

  void undo();
  void redo();

  bool saved;
  float min_brush_size, max_brush_size, brush_size;

private:
  std::vector<ROI_UndoEntry> undo_list;
  int current_undo;

  static int number_of_undos;
  static int current_preset_colour;
  static int new_roi_counter;
};

template <class ImageType> void ROI_Item::save(ImageType &&out, GLubyte *data) {
  for (auto l = Loop(out)(out); l; ++l)
    out.value() = data[out.index(0) + out.size(0) * (out.index(1) + out.size(1) * out.index(2))];
  saved = true;
  filename = out.name();
}

} // namespace MR::GUI::MRView::Tool
