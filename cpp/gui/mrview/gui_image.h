/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "image.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "mrview/volume.h"
#include "opengl/glutils.h"
#include "types.h"
#include <unordered_map>

namespace MR::GUI {

class Projection;

namespace MRView {

class Window;

namespace Tool {
class ODF;
}

class ImageBase : public Volume {
public:
  ImageBase(MR::Header &&);
  virtual ~ImageBase();

  void render2D(Displayable::Shader &shader_program, const Projection &projection, const int plane, const int slice);
  void render3D(Displayable::Shader &shader_program, const Projection &projection, const float depth);

  virtual void update_texture2D(const int plane, const int slice) = 0;
  virtual void update_texture3D() = 0;

  void get_axes(const int plane, int &x, int &y) const;

protected:
  GL::Texture texture2D[3];
  std::vector<ssize_t> tex_positions;
};

class Image : public ImageBase {
public:
  Image(MR::Header &&);

  void update_texture2D(const int plane, const int slice) override;
  void update_texture3D() override;

  void request_render_colourbar(DisplayableVisitor &visitor) override {
    if (show_colour_bar)
      visitor.render_image_colourbar(*this);
  }

  MR::Image<cfloat> image;

  cfloat trilinear_value(const Eigen::Vector3f &) const;
  cfloat nearest_neighbour_value(const Eigen::Vector3f &) const;
  Eigen::VectorXcf trilinear_values(const Eigen::Vector3f &, const size_t axis = 3) const;
  Eigen::VectorXcf nearest_neighbour_values(const Eigen::Vector3f &, const size_t axis = 3) const;

  const transform_type &transform() const { return image.transform(); }
  const std::vector<std::string> &comments() const { return _comments; }
  std::string describe_value(const Eigen::Vector3f &focus) const;

  void reset_windowing(const int, const bool);

protected:
  struct CachedTexture {
    GL::Texture tex;
    float value_min, value_max;
  };

  std::array<float, 3> slice_min, slice_max;
  std::unordered_map<size_t, CachedTexture> tex_4d_cache;

private:
  bool volume_unchanged();
  bool format_unchanged();
  size_t guess_colourmap() const;

  template <typename T> void copy_texture_3D();
  void copy_texture_3D_complex();
  void lookup_texture_4D_cache();
  void update_texture_4D_cache();

  std::vector<std::string> _comments;
};

} // namespace MRView
} // namespace MR::GUI
