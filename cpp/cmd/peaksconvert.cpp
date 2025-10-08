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

#include <string>

#include "adapter/base.h"
#include "algo/loop.h"
#include "command.h"
#include "header.h"
#include "image.h"
#include "math/sphere.h"
#include "transform.h"

using namespace MR;
using namespace App;

// TODO Do we need to support both mathematics and physics conventions for spherical coordinates?
// And if so, where do we do it?

const std::vector<std::string> formats = {"unitspherical", "spherical", "unit3vector", "3vector"};
enum class format_t { UNITSPHERICAL, SPHERICAL, UNITTHREEVECTOR, THREEVECTOR };
const std::vector<std::string> references = {"xyz", "ijk", "fsl"};
enum class reference_t { XYZ, IJK, FSL };

using transform_linear_type = Eigen::Matrix<default_type, 3, 3>;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Convert peak directions images between formats and/or conventions";

  DESCRIPTION
  + "Under default operation with no command-line options specified, "
    "the output image will be identical to the input image, "
    "as the MRtrix convention (3-vectors defined with respect to RAS scanner space axes) "
    "will be assumed to apply to both cases. "
    "This behaviour is only modulated by explicitly providing command-line options "
    "that give additional information about the format or reference"
    " of either input or output images."

  + "For -in_format and -out_format options,"
    " the choices are:"
    " - \"unitspherical\": Each orientation is represented using 2 sequential volumes"
      " encoded as azimuth and inclination angles in radians;"
    " - \"spherical\": Each orientation and associated value"
      " is represented using 3 sequential volumes,"
      " with associated value (\"radius\") first,"
      " followed by aximuth and inclination angles in radians;"
    " - \"unit3vector\": Each orientation is represented using 3 sequential volumes"
      " encoded as three dot products with respect to three orthogonal reference axes;"
    " - \"3vector\": Each orientation and associated non-negative value"
      " is represented using 3 sequential volumes,"
      " with the norm of that 3-vector encoding the associated value"
      " and the unit-normalised vector encoding the three dot products"
      " with respect to three orthogonal reference axes."
    " The default behaviour throughout MRtrix3"
    " is to interpret data as either \"unit3vector\" or \"3vector\""
    " depending upon the context and/or presence of non-unit norm vectors in the data."

  + "For -in_reference and -out_reference options,"
    " the choices are:"
    " - \"xyz\": Directions are defined with respect to \"real space\" / \"scanner space\","
      " which is independent of the transform stored within the image header,"
      " with the assumption that the positive direction of the first axis is that closest to anatomical right,"
      " the positive direction of the second axis is that closest to anatomical anterior,"
      " and the positive direction of the third axis is that closest to anatomical superior"
      " (so-called \"RAS+\");"
    " - \"ijk\": Directions are defined with respect to the image axes"
      " as represented on the file system;"
    " - \"fsl\": Directions are defined with respect to the internal convention adopted by the FSL software,"
      " which is equivalent to \"ijk\" for images with a negative header transform determinant"
      " (so-called \"left-handed\" coordinate systems)"
      " but for images with a positive header transform determinant"
      " (which is the case for the \"RAS+\" convention adopted for both NIfTI and MRtrix3)"
      " the interpretation is equivalent to being with respect to the image axes"
      " after flipping the first image axis."
    " The default interpretation in MRtrix3,"
    " including for this command in the absence of use of one of the command-line options,"
    " is \"xyz\".";

  ARGUMENTS
  + Argument ("input", "the input directions image").type_image_in()
  + Argument ("output", "the output directions image").type_image_out();

  OPTIONS
  + OptionGroup ("Options providing information about the input image")
  + Option ("in_format", "specify the format in which the input directions are specified"
                         " (see Description)")
    + Argument("choice").type_choice(formats)
  + Option ("in_reference", "specify the reference axes against which the input directions are specified"
                            " (see Description)")
    + Argument("choice").type_choice(references)
  // TODO Add option to import amplitudes to fuse with unit orientations / overwrite existing values

  + OptionGroup ("Options providing information about the output image")
  + Option ("out_format", "specify the format in which the output directions will be specified"
                          " (see Description)")
    + Argument("choice").type_choice(formats)
  + Option ("out_reference", "specify the reference axes against which the output directions will be specified"
                             " (see Description)")
    + Argument("choice").type_choice(references);
  // TODO Implement -fill
  //+ Option ("fill", "specify value to be inserted into output image in the absence of valid information")
  //  + Argument("value").type_float();

  // TODO Any additional options?
  // - Reduce maximal number of fixels per voxel

}
// clang-format on

format_t format_from_option(const std::string &option_name) {
  auto opt = get_options(option_name);
  if (opt.empty())
    return format_t::THREEVECTOR;
  switch (static_cast<MR::App::ParsedArgument::IntType>(opt[0][0])) {
  case 0:
    return format_t::UNITSPHERICAL;
  case 1:
    return format_t::SPHERICAL;
  case 2:
    return format_t::UNITTHREEVECTOR;
  case 3:
    return format_t::THREEVECTOR;
  default:
    throw Exception("Unsupported input to option -" + option_name);
  }
}
reference_t reference_from_option(const std::string &option_name) {
  auto opt = get_options(option_name);
  if (opt.empty())
    return reference_t::XYZ;
  switch (static_cast<MR::App::ParsedArgument::IntType>(opt[0][0])) {
  case 0:
    return reference_t::XYZ;
  case 1:
    return reference_t::IJK;
  case 2:
    return reference_t::FSL;
  default:
    throw Exception("Unsupported input to option -" + option_name);
  }
}
size_t volumes_per_fixel(format_t format) { return format == format_t::UNITSPHERICAL ? 2 : 3; }

template <size_t NumElements> class FormatBase {
public:
  FormatBase(const Eigen::Matrix<default_type, NumElements, 1> & /* unused */) {}
  virtual ~FormatBase() = default;
  virtual Eigen::Matrix<default_type, NumElements, 1> operator()() const = 0;
  static size_t num_elements() { return NumElements; }
};

class UnitSpherical : public FormatBase<2> {
public:
  UnitSpherical(const Eigen::Matrix<default_type, 2, 1> &in) : FormatBase(in), azimuth(in[0]), inclination(in[1]) {}
  Eigen::Matrix<default_type, 2, 1> operator()() const override { return {azimuth, inclination}; }
  default_type azimuth, inclination;
  friend std::ostream &operator<<(std::ostream &stream, const UnitSpherical &in) {
    stream << "UnitSpherical(az=" << in.azimuth << ", in=" << in.inclination << ")";
    return stream;
  }
};

class Spherical : public FormatBase<3> {
public:
  Spherical(const Eigen::Matrix<default_type, 3, 1> &in)
      : FormatBase(in), radius(in[0]), azimuth(in[1]), inclination(in[2]) {}
  Eigen::Matrix<default_type, 3, 1> operator()() const override { return {radius, azimuth, inclination}; }
  default_type radius, azimuth, inclination;
  friend std::ostream &operator<<(std::ostream &stream, const Spherical &in) {
    stream << "Spherical(r=" << in.radius << ", az=" << in.azimuth << ", in=" << in.inclination << ")";
    return stream;
  }
};

class UnitThreeVector : public FormatBase<3> {
public:
  UnitThreeVector(const Eigen::Matrix<default_type, 3, 1> &in) : FormatBase(in), unitthreevector(in) {
    unitthreevector.normalize();
  }
  Eigen::Matrix<default_type, 3, 1> operator()() const override { return unitthreevector; }
  Eigen::Vector3d unitthreevector;
  friend std::ostream &operator<<(std::ostream &stream, const UnitThreeVector &in) {
    stream << "UnitThreeVector(" << in.unitthreevector.transpose() << ")";
    return stream;
  }
};

class ThreeVector : public FormatBase<3> {
public:
  ThreeVector(const Eigen::Matrix<default_type, 3, 1> &in) : FormatBase(in), threevector(in) {}
  Eigen::Matrix<default_type, 3, 1> operator()() const override { return threevector; }
  Eigen::Matrix<default_type, 3, 1> normalized() const { return threevector.normalized(); }
  default_type radius() const { return threevector.norm(); }
  Eigen::Vector3d threevector;
  friend std::ostream &operator<<(std::ostream &stream, const ThreeVector &in) {
    stream << "ThreeVector(" << in.threevector.transpose() << ")";
    return stream;
  }
};

// Common intermediary format to be used regardless of input / output image details
// - ALWAYS in XYZ space
// - ALWAYS with a unit 3-vector
// - ALWAYS with a radius term present, even if it might be filled with unity
class Fixel {
public:
  Fixel(const Eigen::Vector3d &unit_threevector_xyz, default_type radius)
      : unit_threevector_xyz(unit_threevector_xyz), radius(radius) {}
  Fixel(const Eigen::Vector3d &unit_threevector_xyz)
      : unit_threevector_xyz(unit_threevector_xyz), radius(default_type(1)) {}

  template <class T, reference_t ref> static Fixel from(const T &);
  template <class T, reference_t ref> T to() const;

  static void set_input_transforms(const Header &H);
  static void set_output_transforms(const Header &H);

  friend std::ostream &operator<<(std::ostream &stream, const Fixel &in) {
    stream << "Fixel([" << in.unit_threevector_xyz.transpose() << "]: " << in.radius << ")";
    return stream;
  }

private:
  Eigen::Vector3d unit_threevector_xyz;
  default_type radius;

  static transform_linear_type in_ijk2xyz;
  static bool in_fsl_flipi;
  static default_type in_fsl_imultiplier;
  static Eigen::Vector3d in_fsl2ijk;

  static transform_linear_type out_ijk2xyz;
  static transform_linear_type out_xyz2ijk;
  static bool out_fsl_flipi;
  static default_type out_fsl_imultiplier;
  static Eigen::Vector3d out_ijk2fsl;
};

transform_linear_type Fixel::in_ijk2xyz =
    transform_linear_type::Constant(std::numeric_limits<default_type>::signaling_NaN());
bool Fixel::in_fsl_flipi = false;
default_type Fixel::in_fsl_imultiplier = std::numeric_limits<default_type>::signaling_NaN();
Eigen::Vector3d Fixel::in_fsl2ijk = Eigen::Vector3d::Constant(std::numeric_limits<default_type>::signaling_NaN());
transform_linear_type Fixel::out_ijk2xyz =
    transform_linear_type::Constant(std::numeric_limits<default_type>::signaling_NaN());
transform_linear_type Fixel::out_xyz2ijk =
    transform_linear_type::Constant(std::numeric_limits<default_type>::signaling_NaN());
bool Fixel::out_fsl_flipi = false;
default_type Fixel::out_fsl_imultiplier = std::numeric_limits<default_type>::signaling_NaN();
Eigen::Vector3d Fixel::out_ijk2fsl = Eigen::Vector3d::Constant(std::numeric_limits<default_type>::signaling_NaN());

void Fixel::set_input_transforms(const Header &H) {
  in_ijk2xyz = H.realignment().orig_transform().linear();
  in_fsl_flipi = in_ijk2xyz.determinant() > 0.0;
  in_fsl_imultiplier = in_fsl_flipi ? -1.0 : 1.0;
  in_fsl2ijk = {in_fsl_imultiplier, 1.0, 1.0};
  DEBUG("Input transform configured based on image \"" + H.name() + "\":");
  DEBUG("IJK-to-XYZ transform:\n" + str(in_ijk2xyz));
  DEBUG("FSL: flip " + str(in_fsl_flipi) + ", "                       //
        + "i component multiplier " + str(in_fsl_imultiplier) + ", "  //
        + "vector multiplier [" + str(in_fsl2ijk.transpose()) + "]"); //
}

void Fixel::set_output_transforms(const Header &H) {
  out_ijk2xyz = H.transform().linear();
  out_xyz2ijk = H.transform().inverse().linear();
  out_fsl_flipi = out_ijk2xyz.determinant() > 0.0;
  out_fsl_imultiplier = out_fsl_flipi ? -1.0 : 1.0;
  out_ijk2fsl = {out_fsl_imultiplier, 1.0, 1.0};
  DEBUG("Output transform configured based on image \"" + H.name() + "\":");
  DEBUG("IJK-to-XYZ transform:\n" + str(out_ijk2xyz));
  DEBUG("XYZ-to-IJK transform:\n" + str(out_xyz2ijk));
  DEBUG("FSL: flip " + str(out_fsl_flipi) + ", "                       //
        + "i component multiplier " + str(out_fsl_imultiplier) + ", "  //
        + "vector multiplier [" + str(out_ijk2fsl.transpose()) + "]"); //
}

template <> Fixel Fixel::from<UnitSpherical, reference_t::XYZ>(const UnitSpherical &in) {
  const Eigen::Matrix<default_type, 2, 1> az_in_xyz({in.azimuth, in.inclination});
  Eigen::Vector3d unit_threevector_xyz;
  Math::Sphere::spherical2cartesian(az_in_xyz, unit_threevector_xyz);
  return Fixel(unit_threevector_xyz);
}

template <> Fixel Fixel::from<UnitSpherical, reference_t::IJK>(const UnitSpherical &in) {
  const Eigen::Matrix<default_type, 2, 1> az_in_ijk({in.azimuth, in.inclination});
  Eigen::Vector3d unit_threevector_ijk;
  Math::Sphere::spherical2cartesian(az_in_ijk, unit_threevector_ijk);
  return Fixel(in_ijk2xyz * unit_threevector_ijk);
}

template <> Fixel Fixel::from<UnitSpherical, reference_t::FSL>(const UnitSpherical &in) {
  const Eigen::Matrix<default_type, 2, 1> az_in_fsl({in.azimuth, in.inclination});
  const Eigen::Matrix<default_type, 2, 1> az_in_ijk(
      {in_fsl_flipi ? Math::pi - az_in_fsl[0] : az_in_fsl[0], az_in_fsl[1]});
  Eigen::Vector3d unit_threevector_ijk;
  Math::Sphere::spherical2cartesian(az_in_ijk, unit_threevector_ijk);
  return Fixel(in_ijk2xyz * unit_threevector_ijk);
}

template <> Fixel Fixel::from<Spherical, reference_t::XYZ>(const Spherical &in) {
  const Eigen::Matrix<default_type, 3, 1> r_az_in_xyz({in.radius, in.azimuth, in.inclination});
  Eigen::Vector3d unit_threevector_xyz;
  Math::Sphere::spherical2cartesian(r_az_in_xyz.tail<2>(), unit_threevector_xyz);
  return Fixel(unit_threevector_xyz, r_az_in_xyz[0]);
}

template <> Fixel Fixel::from<Spherical, reference_t::IJK>(const Spherical &in) {
  const Eigen::Matrix<default_type, 3, 1> r_az_in_ijk({in.radius, in.azimuth, in.inclination});
  Eigen::Vector3d unit_threevector_ijk;
  Math::Sphere::spherical2cartesian(r_az_in_ijk.tail<2>(), unit_threevector_ijk);
  return Fixel(in_ijk2xyz * unit_threevector_ijk, r_az_in_ijk[0]);
}

template <> Fixel Fixel::from<Spherical, reference_t::FSL>(const Spherical &in) {
  const Eigen::Matrix<default_type, 3, 1> r_az_in_fsl({in.radius, in.azimuth, in.inclination});
  const Eigen::Matrix<default_type, 3, 1> r_az_in_ijk(
      {r_az_in_fsl[0], in_fsl_flipi ? Math::pi - r_az_in_fsl[1] : r_az_in_fsl[1], r_az_in_fsl[2]});
  Eigen::Vector3d unit_threevector_ijk;
  Math::Sphere::spherical2cartesian(r_az_in_ijk.tail<2>(), unit_threevector_ijk);
  return Fixel(in_ijk2xyz * unit_threevector_ijk, r_az_in_ijk[0]);
}

template <> Fixel Fixel::from<UnitThreeVector, reference_t::XYZ>(const UnitThreeVector &in) { return Fixel(in()); }

template <> Fixel Fixel::from<UnitThreeVector, reference_t::IJK>(const UnitThreeVector &in) {
  return Fixel(in_ijk2xyz * in());
}
template <> Fixel Fixel::from<UnitThreeVector, reference_t::FSL>(const UnitThreeVector &in) {
  return Fixel(in_ijk2xyz * (in().cwiseProduct(in_fsl2ijk)));
}

template <> Fixel Fixel::from<ThreeVector, reference_t::XYZ>(const ThreeVector &in) {
  return Fixel(in.normalized(), in.radius());
}

template <> Fixel Fixel::from<ThreeVector, reference_t::IJK>(const ThreeVector &in) {
  return Fixel(in_ijk2xyz * in.normalized(), in.radius());
}

template <> Fixel Fixel::from<ThreeVector, reference_t::FSL>(const ThreeVector &in) {
  return Fixel(in_ijk2xyz * (in.normalized().cwiseProduct(in_fsl2ijk)), in.radius());
}

template <> UnitSpherical Fixel::to<UnitSpherical, reference_t::XYZ>() const {
  Eigen::Matrix<default_type, 2, 1> az_in_xyz;
  Math::Sphere::cartesian2spherical(unit_threevector_xyz, az_in_xyz);
  return UnitSpherical(az_in_xyz);
}

template <> UnitSpherical Fixel::to<UnitSpherical, reference_t::IJK>() const {
  const default_type azimuth =
      std::atan2(unit_threevector_xyz.dot(out_ijk2xyz.col(1)), unit_threevector_xyz.dot(out_ijk2xyz.col(0)));
  const default_type inclination = std::acos(unit_threevector_xyz.dot(out_ijk2xyz.col(2)));
  return UnitSpherical({azimuth, inclination});
}

template <> UnitSpherical Fixel::to<UnitSpherical, reference_t::FSL>() const {
  default_type azimuth =
      std::atan2(unit_threevector_xyz.dot(out_ijk2xyz.col(1)), unit_threevector_xyz.dot(out_ijk2xyz.col(0)));
  if (out_fsl_flipi)
    azimuth = Math::pi - azimuth;
  const default_type inclination = std::acos(unit_threevector_xyz.dot(out_ijk2xyz.col(2)));
  return UnitSpherical({azimuth, inclination});
}

template <> Spherical Fixel::to<Spherical, reference_t::XYZ>() const {
  Eigen::Matrix<default_type, 3, 1> r_az_in_xyz;
  r_az_in_xyz[0] = radius;
  Math::Sphere::cartesian2spherical(unit_threevector_xyz, r_az_in_xyz.tail<2>());
  return Spherical(r_az_in_xyz);
}

template <> Spherical Fixel::to<Spherical, reference_t::IJK>() const {
  const default_type azimuth =
      std::atan2(unit_threevector_xyz.dot(out_ijk2xyz.col(1)), unit_threevector_xyz.dot(out_ijk2xyz.col(0)));
  const default_type inclination = std::acos(unit_threevector_xyz.dot(out_ijk2xyz.col(2)));
  return Spherical({radius, azimuth, inclination});
}

template <> Spherical Fixel::to<Spherical, reference_t::FSL>() const {
  default_type azimuth =
      std::atan2(unit_threevector_xyz.dot(out_ijk2xyz.col(1)), unit_threevector_xyz.dot(out_ijk2xyz.col(0)));
  if (out_fsl_flipi)
    azimuth = Math::pi - azimuth;
  const default_type inclination = std::acos(unit_threevector_xyz.dot(out_ijk2xyz.col(2)));
  return Spherical({radius, azimuth, inclination});
}

template <> UnitThreeVector Fixel::to<UnitThreeVector, reference_t::XYZ>() const {
  return UnitThreeVector(unit_threevector_xyz);
}

template <> UnitThreeVector Fixel::to<UnitThreeVector, reference_t::IJK>() const {
  return UnitThreeVector(out_xyz2ijk * unit_threevector_xyz);
}

template <> UnitThreeVector Fixel::to<UnitThreeVector, reference_t::FSL>() const {
  return UnitThreeVector((out_xyz2ijk * unit_threevector_xyz).cwiseProduct(out_ijk2fsl));
}

template <> ThreeVector Fixel::to<ThreeVector, reference_t::XYZ>() const {
  return ThreeVector(unit_threevector_xyz * radius);
}

template <> ThreeVector Fixel::to<ThreeVector, reference_t::IJK>() const {
  return ThreeVector(out_xyz2ijk * unit_threevector_xyz * radius);
}

template <> ThreeVector Fixel::to<ThreeVector, reference_t::FSL>() const {
  return ThreeVector((out_xyz2ijk * unit_threevector_xyz).cwiseProduct(out_ijk2fsl) * radius);
}

template <class FixelType> class FixelImage : public Adapter::Base<FixelImage<FixelType>, Image<float>> {
public:
  using ImageType = Image<float>;
  using BaseType = Adapter::Base<FixelImage<FixelType>, ImageType>;
  using BaseType::parent;
  FixelImage(Image<float> &that) : BaseType(that), fixel_index(0) {}
  void reset() {
    parent().reset();
    fixel_index = 0;
  }
  ssize_t size(size_t axis) const {
    return axis == 3 ? (parent().size(3) / FixelType::num_elements()) : parent().size(axis);
  }
  ssize_t get_index(size_t axis) const { return (axis == 3) ? fixel_index : parent().get_index(axis); }
  void move_index(size_t axis, ssize_t increment) {
    if (axis != 3) {
      parent().move_index(axis, increment);
      return;
    }
    parent().move_index(3, FixelType::num_elements() * increment);
    fixel_index += increment;
  }
  FixelType get_value() {
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> data(
        Eigen::Matrix<default_type, Eigen::Dynamic, 1>::Zero(FixelType::num_elements()));
    for (size_t index = 0; index != FixelType::num_elements(); ++index) {
      data[index] = parent().get_value();
      parent().move_index(3, 1);
    }
    parent().move_index(3, -FixelType::num_elements());
    return FixelType(data);
  }
  void set_value(const FixelType &value) {
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> data(
        Eigen::Matrix<default_type, Eigen::Dynamic, 1>::Zero(FixelType::num_elements()));
    data = value();
    for (size_t index = 0; index != FixelType::num_elements(); ++index) {
      parent().set_value(data[index]);
      parent().move_index(3, 1);
    }
    parent().move_index(3, -FixelType::num_elements());
  }

private:
  ssize_t fixel_index;
};

template <reference_t in_reference, class InFixelType, reference_t out_reference, class OutFixelType>
void run(FixelImage<InFixelType> &in_fixel_image, FixelImage<OutFixelType> &out_fixel_image) {
  // TODO Multi-thread
  // TODO Test to see if this naturally works across bootstrap realisations
  for (auto l = Loop("Converting peaks orientations", in_fixel_image)(in_fixel_image, out_fixel_image); l; ++l) {
    const Fixel fixel(Fixel::from<InFixelType, in_reference>(in_fixel_image.get_value()));
    out_fixel_image.set_value(fixel.to<OutFixelType, out_reference>());
  }
}

template <class InFixelType, reference_t out_reference, class OutFixelType>
void run(reference_t in_reference, FixelImage<InFixelType> &in_fixel_image, FixelImage<OutFixelType> &out_fixel_image) {
  switch (in_reference) {
  case reference_t::XYZ:
    run<reference_t::XYZ, InFixelType, out_reference, OutFixelType>(in_fixel_image, out_fixel_image);
    return;
  case reference_t::IJK:
    run<reference_t::IJK, InFixelType, out_reference, OutFixelType>(in_fixel_image, out_fixel_image);
    return;
  case reference_t::FSL:
    run<reference_t::FSL, InFixelType, out_reference, OutFixelType>(in_fixel_image, out_fixel_image);
    return;
  }
}

template <reference_t out_reference, class OutFixelType>
void run(format_t in_format,
         reference_t in_reference,
         Image<float> &input_image,
         FixelImage<OutFixelType> &out_fixel_image) {
  switch (in_format) {
  case format_t::UNITSPHERICAL: {
    FixelImage<UnitSpherical> in_fixel_image(input_image);
    run<UnitSpherical, out_reference, OutFixelType>(in_reference, in_fixel_image, out_fixel_image);
    return;
  }
  case format_t::SPHERICAL: {
    FixelImage<Spherical> in_fixel_image(input_image);
    run<Spherical, out_reference, OutFixelType>(in_reference, in_fixel_image, out_fixel_image);
    return;
  }
  case format_t::UNITTHREEVECTOR: {
    FixelImage<UnitThreeVector> in_fixel_image(input_image);
    run<UnitThreeVector, out_reference, OutFixelType>(in_reference, in_fixel_image, out_fixel_image);
    return;
  }
  case format_t::THREEVECTOR: {
    FixelImage<ThreeVector> in_fixel_image(input_image);
    run<ThreeVector, out_reference, OutFixelType>(in_reference, in_fixel_image, out_fixel_image);
    return;
  }
  default:
    assert(false);
  }
}

template <class OutFixelType>
void run(format_t in_format,
         reference_t in_reference,
         Image<float> &input_image,
         reference_t out_reference,
         FixelImage<OutFixelType> &out_fixel_image) {
  switch (out_reference) {
  case reference_t::XYZ:
    run<reference_t::XYZ, OutFixelType>(in_format, in_reference, input_image, out_fixel_image);
    return;
  case reference_t::IJK:
    run<reference_t::IJK, OutFixelType>(in_format, in_reference, input_image, out_fixel_image);
    return;
  case reference_t::FSL:
    run<reference_t::FSL, OutFixelType>(in_format, in_reference, input_image, out_fixel_image);
    return;
  }
}

void run(format_t in_format,
         reference_t in_reference,
         Image<float> &input_image,
         format_t out_format,
         reference_t out_reference,
         Image<float> &output_image) {
  switch (out_format) {
  case format_t::UNITSPHERICAL: {
    FixelImage<UnitSpherical> out_fixel_image(output_image);
    run(in_format, in_reference, input_image, out_reference, out_fixel_image);
    return;
  }
  case format_t::SPHERICAL: {
    FixelImage<Spherical> out_fixel_image(output_image);
    run(in_format, in_reference, input_image, out_reference, out_fixel_image);
    return;
  }
  case format_t::UNITTHREEVECTOR: {
    FixelImage<UnitThreeVector> out_fixel_image(output_image);
    run(in_format, in_reference, input_image, out_reference, out_fixel_image);
    return;
  }
  case format_t::THREEVECTOR: {
    FixelImage<ThreeVector> out_fixel_image(output_image);
    run(in_format, in_reference, input_image, out_reference, out_fixel_image);
    return;
  }
  default:
    assert(false);
  }
}

void run() {

  Header H_in = Header::open(argument[0]);
  if (H_in.ndim() != 4)
    throw Exception("Input image must be 4D");

  const format_t in_format(format_from_option("in_format"));
  const size_t in_volumes_per_fixel(volumes_per_fixel(in_format));
  const size_t num_fixels = H_in.size(3) / in_volumes_per_fixel;
  if (num_fixels * in_volumes_per_fixel != H_in.size(3))
    throw Exception("Number of volumes in input image (" + str(H_in.size(3)) + ")" + " incompatible with " +
                    str(volumes_per_fixel) + " volumes per orientation");
  const reference_t in_reference(reference_from_option("in_reference"));

  const format_t out_format(format_from_option("out_format"));
  if ((in_format == format_t::SPHERICAL || in_format == format_t::THREEVECTOR) &&
      (out_format == format_t::UNITSPHERICAL || out_format == format_t::UNITTHREEVECTOR)) {
    WARN("Output image will not include amplitudes that may be present in input image due to chosen format");
  }
  const reference_t out_reference(reference_from_option("out_reference"));

  Header H_out(H_in);
  H_out.name() = std::string(argument[1]);
  H_out.size(3) = num_fixels * volumes_per_fixel(out_format);
  Stride::set_from_command_line(H_out);

  Fixel::set_input_transforms(H_in);
  Fixel::set_output_transforms(H_out);

  auto input = H_in.get_image<float>();
  auto output = Image<float>::create(argument[1], H_out);
  run(in_format, in_reference, input, out_format, out_reference, output);
}
