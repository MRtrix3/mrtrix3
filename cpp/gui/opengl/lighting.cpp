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

#include "opengl/lighting.h"

#include <array>

#include "file/config.h"

namespace MR::GUI::GL {
namespace {

constexpr float DefaultAmbient = 0.5F;
constexpr float DefaultDiffuse = 0.5F;
constexpr float DefaultSpecular = 0.5F;
constexpr float DefaultShine = 5.0F;
const Eigen::Array3f DefaultBackgroundColor(1.0F, 1.0F, 1.0F);
const Eigen::Array3f DefaultLightPosition(1.0F, 1.0F, 3.0F);

} // namespace

void Lighting::load_defaults() {
  // CONF option: BackgroundColor
  // CONF default: 1.0,1.0,1.0
  // CONF The default colour to use for the background in OpenGL panels, notably
  // CONF the SH viewer.
  background_color = File::Config::get_RGB("BackgroundColor", DefaultBackgroundColor);

  // CONF option: LightPosition
  // CONF default: 1.0,1.0,3.0
  // CONF The default position vector to use for the light in OpenGL
  // CONF renders.
  lightpos = File::Config::get_RGB("LightPosition", DefaultLightPosition).matrix().normalized();

  // CONF option: AmbientIntensity
  // CONF default: 0.5
  // CONF The default intensity for the ambient light in OpenGL renders.
  ambient = File::Config::get_float("AmbientIntensity", DefaultAmbient);
  // CONF option: DiffuseIntensity
  // CONF default: 0.5
  // CONF The default intensity for the diffuse light in OpenGL renders.
  diffuse = File::Config::get_float("DiffuseIntensity", DefaultDiffuse);
  // CONF option: SpecularIntensity
  // CONF default: 0.5
  // CONF The default intensity for the specular light in OpenGL renders.
  specular = File::Config::get_float("SpecularIntensity", DefaultSpecular);
  // CONF option: SpecularExponent
  // CONF default: 5.0
  // CONF The default exponent for the specular light in OpenGL renders.
  shine = File::Config::get_float("SpecularExponent", DefaultShine);
}

} // namespace MR::GUI::GL
