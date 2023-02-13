/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "progressbar.h"
#include "transform.h"
#include "types.h"

#include "file/path.h"
#include "fixel/helpers.h"
#include "interp/nearest.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/resampling/upsampler.h"


using namespace MR;
using namespace App;


enum class target_t { UNDEFINED, TSF, IMAGE, FIXEL };


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Map the minimal distance to a mask along streamlines trajectories";

  // TODO New capabilities:
  // - Allow ROI to be a fixel mask, rather than a voxel mask;
  //     just do in easiest way possible, deal with resolving against fixel TWI branch later

  DESCRIPTION
  + "This command aims to determine the spatial distance from any location to a user-defined 3D binary mask, "
    "but where those distances are computed based on the length along streamline trajectories rather "
    "than simple Euclidean distances."

  + "It is not necssary for the input ROI to be a single spatially contiguous cluster. For every streamline vertex, the "
    "distance to the nearest vertex that intersects the ROI is quantified; this could be in either direction along the "
    "trajectory of that particular streamline."

  + "Any streamline that does not intersect the input ROI will not contribute in any way to the resulting distance maps."

  + "The command can be used in one of three ways:"

  + "- By providing an input image via the -template option, the output image (defined on the same image grid) will contain, "
      "for every voxel, a value of minimum distance to the ROI based on the mean across those streamlines that intersect both "
      "that voxel and the ROI."

  + "- By providing either a fixel directory or fixel data file via the -template option, the output will be a fixel data file "
      "that contains, for every fixel, a value of minimum distance to the ROI based on the mean across those streamlines that "
      "intersect both that fixel and the ROI."

  + "- By not providing the -template option, the output will be a Track Scalar File (.tsf) that contains, for every streamline vertex, "
      "the minimal distance to the ROI along the trajectory of that streamline. Any streamline that does not intersect the ROI at any point "
      "will contain a value of -1.0 at every vertex.";

  ARGUMENTS
  + Argument ("roi",    "the region of interest mask").type_image_in()
  + Argument ("tracks", "the input track file").type_tracks_in()
  + Argument ("output", "the output path (either an image, a fixel data file, or a Track Scalar File (.tsf)").type_various();

  OPTIONS
  + Option ("template", "template on which an output image will be based; this can be either a >=3D image, or a fixel directory / file within such")
    + Argument ("image").type_various();

}


using value_type = float;
using roi_type = Interp::Nearest<Image<bool>>;
// TODO ScalarWriter should be compatible with Arrays
// TODO Cancel that; shoudl instead be explicitly using TrackScalar
// Well actually, some places it feels wrong, since it doesn't involve a unique streamline index
using vector_value_type = Eigen::Matrix<value_type, Eigen::Dynamic, 1>;
using vector_int_type = Eigen::Matrix<uint32_t, Eigen::Dynamic, 1>;


value_type angular_threshold_dp()
{
  return std::cos (45.0 * (Math::pi / 180.0));
}


// TODO If Fixel TWI code is updated to support incorporation of a TSF with streamline mapping
//   (replacing what was previously only used to support Gaussian track-wise statistic),
//   functionality of this command could instead be achieved by generating a TSF from this command,
//   and feeding that result to tckmap with a fixel image as target.




class TckVisitation;

class TargetBase
{
  public:
    TargetBase() { }
    virtual ~TargetBase() { }

    virtual const Header& header() const = 0;
    virtual size_t nfixels() const = 0;

    virtual void map (const Eigen::Vector3f& pos, const Eigen::Vector3f& dir, const value_type dist, TckVisitation& tck) const = 0;
    virtual void add (TckVisitation& vis) = 0;

    virtual void save (const std::string&) = 0;
};

class TargetImage;
class TargetFixel;



// TODO Need SourceBase, SourceImage, SourceFixel classes to deal with 3D / fixel ROIs






class TckVisitation
{
  public:
    TckVisitation (const std::unique_ptr<TargetBase>& target, const bool is_fixel) :
        fixel_mindist_sum (is_fixel ? target->nfixels() : 0),
        fixel_vertexcount (is_fixel ? target->nfixels() : 0),
        image_mindist_sum (is_fixel ? Image<value_type>() : Image<value_type>::scratch (target->header(), "Streamline sum of minimum distances")),
        image_vertexcount (is_fixel ? Image<uint32_t>() : Image<uint32_t>::scratch (target->header(), "Streamline vertex count")) { }

    void zero()
    {
      fixel_mindist_sum.setZero();
      fixel_vertexcount.setZero();
      if (image_mindist_sum.valid()) {
        for (auto l = Loop(image_mindist_sum) (image_mindist_sum, image_vertexcount); l; ++l) {
          image_mindist_sum.value() = value_type(0);
          image_vertexcount.value() = uint32_t(0);
        }
      }
    }

    void add (const size_t fixel_index, const value_type distance)
    {
      assert (fixel_index < fixel_mindist_sum.size());
      fixel_mindist_sum[fixel_index] += distance;
      fixel_vertexcount[fixel_index]++;
    }

    void add (const Eigen::Array3i& voxel, const value_type distance)
    {
      assert (image_mindist_sum.valid());
      assign_pos_of (voxel).to (image_mindist_sum, image_vertexcount);
      image_mindist_sum.value() += distance;
      image_vertexcount.value() += 1;
    }

  private:
    vector_value_type fixel_mindist_sum;
    vector_int_type fixel_vertexcount;
    Image<value_type> image_mindist_sum;
    Image<uint32_t> image_vertexcount;

    friend class TargetBase;
    friend class TargetImage;
    friend class TargetFixel;

};





class TargetImage : public TargetBase
{
  public:
    TargetImage (const std::string& path) :
        H (Header::open (path)),
        scanner2voxel (Transform (H).scanner2voxel.cast<value_type>()),
        min_dist_sum (Image<value_type>::scratch (H, "Total sum of minimum distances")),
        num_tracks (Image<uint32_t>::scratch (H, "Total number of tracks"))
    {
      check_3D_nonunity (H);
    }

    const Header& header() const override { return H; }
    size_t nfixels() const override { return 0; }

    void map (const Eigen::Vector3f& pos, const Eigen::Vector3f& /*dir*/, const value_type dist, TckVisitation& tck) const override
    {
      const Eigen::Vector3f vox_float = scanner2voxel * pos;
      const Eigen::Array3i vox (int (std::round (vox_float[0])), int (std::round (vox_float[1])), int (std::round (vox_float[2])));
      if (!is_out_of_bounds (min_dist_sum, vox))
        tck.add (vox, dist);
    }

    void add (TckVisitation& tck) override
    {
      assert (tck.image_vertexcount.valid());
      for (auto l = Loop(H) (min_dist_sum, num_tracks, tck.image_mindist_sum, tck.image_vertexcount); l; ++l) {
        if (tck.image_vertexcount.value()) {
          min_dist_sum.value() += tck.image_mindist_sum.value() / value_type(tck.image_vertexcount.value());
          num_tracks.value() += 1;
        }
      }
    }

    void save (const std::string& path) override
    {
      Image<value_type> output = Image<value_type>::create (path, H);
      for (auto l = Loop (H) (min_dist_sum, num_tracks, output); l; ++l) {
        if (num_tracks.value())
          output.value() = min_dist_sum.value() / value_type(num_tracks.value());
        else
          output.value() = NaN;
      }
    }

  private:
    class HeaderHelper : public Header
    {
      public:
        HeaderHelper (const Header& h) :
            Header (h)
        {
          ndim() = 3;
          datatype() = DataType::Float32;
          datatype().set_byte_order_native();
        }
    } H;
    Eigen::Transform<value_type, 3, Eigen::AffineCompact> scanner2voxel;

    Image<value_type> min_dist_sum;
    Image<uint32_t> num_tracks;

};






class TargetFixel : public TargetBase
{
  public:
    TargetFixel (const std::string& path) :
        index_header (Fixel::find_index_header (path)),
        scanner2voxel (Transform (index_header).scanner2voxel.cast<value_type>()),
        total_nfixels (Fixel::get_number_of_fixels (index_header)),
        index_image (index_header.get_image<uint32_t>()),
        directions_image (Fixel::find_directions_header (path).get_image<value_type>()),
        min_dist_sum (vector_value_type::Zero (total_nfixels)),
        num_tracks (vector_int_type::Zero (total_nfixels)) { }

    const Header& header() const override { return index_header; }
    size_t nfixels() const override { return total_nfixels; }

    void map (const Eigen::Vector3f& pos, const Eigen::Vector3f& dir, const value_type dist, TckVisitation& tck) const override
    {
      const Eigen::Vector3f vox = scanner2voxel * pos;
      for (size_t axis = 0; axis != 3; ++axis)
        index_image.index (axis) = ssize_t (std::round (vox[axis]));
      if (!is_out_of_bounds (index_image)) {
        index_image.index (3) = 0; const size_t count  = index_image.value();
        index_image.index (3) = 1; const size_t offset = index_image.value();
        value_type max_dp = angular_threshold_dp();
        size_t selected_index = count + offset;
        for (size_t f = offset; f != count + offset; ++f) {
          directions_image.index(0) = f;
          const Eigen::Vector3f fixel_dir = directions_image.row (1);
          const value_type dp = std::abs (fixel_dir.dot (dir));
          if (dp > max_dp) {
            max_dp = dp;
            selected_index = f;
          }
        }
        if (selected_index != count + offset)
          tck.add (selected_index, dist);
      }
    }

    void add (TckVisitation& tck) override
    {
      assert (tck.fixel_vertexcount.size());
      for (size_t f = 0; f != nfixels(); ++f) {
        if (tck.fixel_vertexcount[f]) {
          min_dist_sum[f] += tck.fixel_mindist_sum[f] / value_type(tck.fixel_vertexcount[f]);
          num_tracks[f]++;
        }
      }
    }

    void save (const std::string& path) override
    {
      Image<value_type> output = Image<value_type>::create (path, Fixel::data_header_from_index (index_header));
      for (auto l = Loop (output) (output); l; ++l) {
        if (num_tracks[output.index(0)])
          output.value() = min_dist_sum[output.index(0)] / value_type(num_tracks[output.index(0)]);
        else
          output.value() = NaN;
      }
    }

  private:
    Header index_header;
    Eigen::Transform<value_type, 3, Eigen::AffineCompact> scanner2voxel;
    const uint32_t total_nfixels;
    mutable Image<uint32_t> index_image;
    mutable Image<float> directions_image;

    vector_value_type min_dist_sum;
    vector_int_type num_tracks;

};







DWI::Tractography::TrackScalar<> vertex_distances (const DWI::Tractography::Streamline<> tck, roi_type& roi)
{
  const size_t num_vertices = tck.size();
  DWI::Tractography::TrackScalar<> result (num_vertices, NaN);
  bool roi_visited = false;
  for (size_t v = 0; v != num_vertices; ++v) {
    roi.scanner (tck[v]);
    if (roi.value()) {
      result[v] = 0.0f;
      roi_visited = true;
    }
  }
  if (!roi_visited) {
    result.resize (0);
    return result;
  }
  // Rather than a bubble expansion, just do one pass in either direction, and take the minimum value of the two
  value_type distance = result[0];
  for (size_t v = 1; v != num_vertices; ++v) {
    if (std::isfinite (result[v]))
      distance = 0.0;
    else if (std::isfinite (distance))
      result[v] = (distance += (tck[v] - tck[v-1]).norm());
  }
  // All values now finite; need to start accumulating distance after the first zero is encountered
  // Also only over-write non-zero values if the distance in this direction is smaller
  distance = result[num_vertices-1] ? NaN : 0.0;
  for (ssize_t v = num_vertices-2; v >= 0; --v) {
    if (result[v] == 0.0) {
      distance = 0.0;
    } else if (std::isfinite (distance)) {
      if (std::isfinite (result[v]))
        result[v] = std::min (result[v], (distance += (tck[v] - tck[v+1]).norm()));
      else
        result[v] = (distance += (tck[v] - tck[v+1]).norm());
    }
  }
  assert (result.allFinite());
  return result;
}







void run ()
{
  // ROI
  roi_type roi (Image<bool>::open (argument[0]));

  // Streamlines
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<> reader (argument[1], properties);
  DWI::Tractography::Streamline<> tck_in;

  // Target image
  std::unique_ptr<TargetBase> target;
  const std::string output_path = argument[2];
  target_t target_type (target_t::UNDEFINED);
  if (Path::has_suffix (output_path, "tsf")) {

    target_type = target_t::TSF;
    DWI::Tractography::ScalarWriter<> out (output_path, properties);

    // Branch operation completely since here we don't perform streamline upsampling in the case of a TSF
    ProgressBar progress ("Mapping distance from ROI along streamlines",
                          properties.find ("count") == properties.end() ? 0 : to<size_t>(properties["count"]));
    while (reader (tck_in)) {
      DWI::Tractography::TrackScalar<> vertex_roi_dist = vertex_distances (tck_in, roi);
      // Can't use non-finite values in track scalar file due to their use as delimiters
      if (!vertex_roi_dist.size())
        vertex_roi_dist.resize (tck_in.size(), -1.0f);
      out (vertex_roi_dist);
      ++progress;
    }
    return;
  }

  // Code proceeds from this point for either 3D image or fixel output targets
  auto opt = get_options ("template");
  if (!opt.size())
    throw Exception ("Output is not a TSF file; -template option must be provided");
  const std::string template_path = opt[0][0];
  try {
    target.reset (new TargetImage (template_path));
    target_type = target_t::IMAGE;
  } catch (...) {
    try {
      target.reset (new TargetFixel (template_path));
    } catch (...) {
      throw Exception ("Cannot determine appropriate image from input to -template option");
    }
  }

  DWI::Tractography::Resampling::Upsampler upsampler (DWI::Tractography::Mapping::determine_upsample_ratio (target->header(), properties, 0.2));

  TckVisitation tck_visitation (target, target_type == target_t::FIXEL);

  ProgressBar progress (std::string ("Mapping distance from ROI along streamlines to ") + (target_type == target_t::IMAGE ? "image" : "fixels"),
                        properties.find ("count") == properties.end() ? 0 : to<size_t>(properties["count"]));
  while (reader (tck_in)) {
    DWI::Tractography::Streamline<> tck_upsampled;
    upsampler (tck_in, tck_upsampled);
    const size_t num_vertices = tck_upsampled.size();
    const DWI::Tractography::TrackScalar<> vertex_roi_dist = vertex_distances (tck_upsampled, roi);
    if (vertex_roi_dist.size()) {
      // We now have, for each streamline vertex, the minimum distance along the streamline to the ROI
      // The task now is to map each vertex to a fixel
      // For each fixel, count the number of vertices within the fixel, and sum the distances of each vertex to the ROI
      // Once this is done, for fixels traversed, get the mean of these distances, and contribute this result
      //   to the min_dist_sum and num_tracks arrays
      tck_visitation.zero();
      for (size_t v = 0; v != num_vertices; ++v) {
        Eigen::Vector3f tck_dir;
        if (v == 0)
          tck_dir = (tck_upsampled[1] - tck_upsampled[0]).normalized();
        else if (v == num_vertices-1)
          tck_dir = (tck_upsampled[v] - tck_upsampled[v-1]).normalized();
        else
          tck_dir = (tck_upsampled[v+1] - tck_upsampled[v-1]).normalized();
        target->map (tck_upsampled[v], tck_dir, vertex_roi_dist[v], tck_visitation);
      }
      target->add (tck_visitation);
    }
    ++progress;
  }

  target->save (output_path);
}
