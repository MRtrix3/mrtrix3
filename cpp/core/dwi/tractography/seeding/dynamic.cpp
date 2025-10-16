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

#include "dwi/tractography/seeding/dynamic.h"

#include "app.h"
#include "dwi/fmls.h"
#include "dwi/tractography/rng.h"
#include "dwi/tractography/seeding/seeding.h"
#include "math/SH.h"

#ifdef DYNAMIC_SEEDING_DEBUGGING
#include "file/utils.h"
#endif
namespace MR::DWI::Tractography::Seeding {

bool Dynamic_ACT_additions::check_seed(Eigen::Vector3f &p) {

  // Needs to be thread-safe
  auto interp = interp_template;

  interp.scanner(p.cast<double>());
  const ACT::Tissues tissues(interp);

  if (tissues.get_csf() > tissues.get_wm() + tissues.get_gm())
    return false;

  if (tissues.get_wm() > tissues.get_gm())
    return true;

  // Detrimental to remove this in all cases tested
  return gmwmi_finder.find_interface(p, interp);
}

const default_type Dynamic::initial_td_sum = 1e-6;

Dynamic::Dynamic(std::string_view in,
                 Image<float> &fod_data,
                 const size_t num,
                 const DWI::Directions::FastLookupSet &dirs)
    : Base(in, "dynamic", attempts_per_seed.at(seed_attempt_t::DYNAMIC)),
      SIFT::ModelBase<Fixel_TD_seed>(fod_data, dirs),
      target_trackcount(num),
      track_count(0),
      attempts(0),
      seeds(0),
#ifdef DYNAMIC_SEED_DEBUGGING
      seed_output("seeds.tck", Tractography::Properties()),
      debugging_fixel_path("dynamic_seeding"),
      test_fixel(0),
#endif
      transform(fod_data) {
  auto opt = App::get_options("act");
  if (!opt.empty())
    act.reset(new Dynamic_ACT_additions(opt[0][0]));

  perform_FOD_segmentation(fod_data);

  // Have to set a volume so that Seeding::List works correctly
  for (const auto &i : fixels)
    volume += i.get_weight();
  volume *= fod_data.spacing(0) * fod_data.spacing(1) * fod_data.spacing(2);

  // Prevent divide-by-zero at commencement
  SIFT::ModelBase<Fixel_TD_seed>::TD_sum = initial_td_sum;

  // For small / unreliable fixels, don't modify the seeding probability during execution
  perform_fixel_masking();

#ifdef DYNAMIC_SEED_DEBUGGING
  File::mkdir(debugging_fixel_path);

  Header H_index(this->header());
  H_index.ndim() = 4;
  H_index.size(3) = 2;
  H_index.datatype() = DataType::native(DataType::UInt64);
  Header H_directions;
  H_directions.ndim() = 3;
  H_directions.size(0) = fixels.size();
  H_directions.size(1) = 3;
  H_directions.size(2) = 1;
  H_directions.stride(0) = 2;
  H_directions.stride(1) = 1;
  H_directions.stride(2) = 3;
  H_directions.spacing(0) = H_directions.spacing(1) = H_directions.spacing(2) = 1.0;
  H_directions.transform().setIdentity();
  H_directions.datatype() = DataType::native(DataType::from<float>());
  Image<uint64_t> index_image = Image<uint64_t>::create(Path::join(debugging_fixel_path, "index.mif"), H_index);
  Image<float> directions_image =
      Image<float>::create(Path::join(debugging_fixel_path, "directions.mif"), H_directions);
  VoxelAccessor v(accessor());
  for (auto l = Loop(v)(v, index_image); l; ++l) {
    if (v.value()) {
      index_image.index(3) = 0;
      index_image.value() = (*v.value()).num_fixels();
      index_image.index(3) = 1;
      index_image.value() = (*v.value()).first_index();
    }
  }
  for (size_t i = 0; i != fixels.size(); ++i) {
    directions_image.index(0) = i;
    directions_image.row(1) = fixels[i].get_dir();
  }

  // Pre-configure a Header instance to use in constructing output fixel data files
  H_fixeldata.ndim() = 3;
  H_fixeldata.size(0) = fixels.size();
  H_fixeldata.size(1) = 1;
  H_fixeldata.size(2) = 1;
  H_fixeldata.stride(0) = 1;
  H_fixeldata.stride(1) = 2;
  H_fixeldata.stride(2) = 3;
  H_fixeldata.spacing(0) = H_fixeldata.spacing(1) = H_fixeldata.spacing(2) = 1.0;
  H_fixeldata.transform().setIdentity();
  H_fixeldata.datatype() = DataType::native(DataType::from<float>());

  // Write the FD image only once
  Image<float> image_FD(Image<float>::create(Path::join(debugging_fixel_path, "FD.mif"), H_fixeldata));
  for (auto l = Loop(0)(image_FD); l; ++l)
    image_FD.value() = fixels[image_FD.index(0)].get_FOD();

  // Pick a good fixel to use for testing / debugging
  std::uniform_real_distribution<float> uniform;
  do {
    test_fixel = std::uniform_int_distribution<int>(1, fixels.size() - 1)(rng);
  } while (fixels[test_fixel].get_weight() < 1.0 || fixels[test_fixel].get_FOD() < 0.5);

  output_fixel_images("begin");
#endif
}

Dynamic::~Dynamic() {

  INFO("Dynamic seeeding required " + str(attempts) + " samples to draw " + str(seeds) + " seeds");

#ifdef DYNAMIC_SEED_DEBUGGING

  // Update all cumulative probabilities before output
  for (size_t fixel_index = 1; fixel_index != fixels.size(); ++fixel_index) {
    Fixel &fixel = fixels[fixel_index];
    const float ratio = fixel.get_ratio(mu());
    const bool force_seed = !fixel.get_TD();
    const size_t current_trackcount = track_count.load(std::memory_order_relaxed);
    const float cumulative_prob = fixel.get_cumulative_prob(current_trackcount);
    float seed_prob = cumulative_prob;
    if (!force_seed) {
      const uint64_t total_seeds = seeds.load(std::memory_order_relaxed);
      const uint64_t fixel_seeds = fixel.get_seed_count();
      seed_prob =
          cumulative_prob * (current_trackcount / static_cast<float>(target_trackcount - current_trackcount)) *
          (((static_cast<float>(total_seeds) / static_cast<float>(fixel_seeds)) *
            (static_cast<float>(target_trackcount) / static_cast<float>(current_trackcount)) *
            ((1.0F / ratio) - (static_cast<float>(total_seeds - fixel_seeds) / static_cast<float>(total_seeds)))) -
           1.0F);
      seed_prob = std::min(1.0F, seed_prob);
      seed_prob = std::max(0.0F, seed_prob);
    }
    fixel.update_prob(seed_prob, false);
  }

  output_fixel_images("end");
#endif
}

bool Dynamic::get_seed(Eigen::Vector3f &) const { return false; }

bool Dynamic::get_seed(Eigen::Vector3f &p, Eigen::Vector3f &d) {

  uint64_t this_attempts = 0;
  std::uniform_int_distribution<size_t> uniform_int(0, fixels.size() - 2);
  std::uniform_real_distribution<float> uniform_float(0.0F, 1.0F);

  while (1) {

    ++this_attempts;
    const size_t fixel_index = 1 + uniform_int(rng);
    Fixel &fixel = fixels[fixel_index];
    float seed_prob;
    if (fixel.can_update()) {

      // Derive the new seed probability
      // TODO Functionalise this?
      const float ratio = fixel.get_ratio(mu());
      const bool force_seed = !fixel.get_TD();
      const size_t current_trackcount = track_count.load(std::memory_order_relaxed);
      const float cumulative_prob = fixel.get_cumulative_prob(current_trackcount);
      seed_prob = cumulative_prob;
      if (!force_seed) {

        // Target track count is double the current track count, until this exceeds the actual target number
        // - try to modify the probabilities faster at earlier stages
        const size_t Szero = std::min(target_trackcount, 2 * current_trackcount);
        seed_prob =
            (ratio < 1.0)
                ? (cumulative_prob * (Szero - (current_trackcount * ratio)) / (ratio * (Szero - current_trackcount)))
                : 0.0;

#ifdef DYNAMIC_SEED_DEBUGGING
        // if (fixel_index == test_fixel)
        //   std::cerr << "Ratio " << ratio << ", tracks " << current_trackcount << "/" << target_trackcount << ", seeds
        //   " << fixel_seeds << "/" << total_seeds << ", prob " << cumulative_prob << " -> " << seed_prob << "\n";
#endif

        // These can occur fairly regularly, depending on the exact derivation
        seed_prob = std::min(1.0F, seed_prob);
        seed_prob = std::max(0.0F, seed_prob);
      }

    } else {
      seed_prob = fixel.get_old_prob();
    }

    if (seed_prob > uniform_float(rng)) {

      const Eigen::Vector3i &v(fixel.get_voxel());
      const Eigen::Vector3f vp(
          v[0] + uniform_float(rng) - 0.5, v[1] + uniform_float(rng) - 0.5, v[2] + uniform_float(rng) - 0.5);
      p = transform.voxel2scanner.cast<float>() * vp;

      bool good_seed = !act;
      if (!good_seed) {

        if (act->check_seed(p)) {
          // Make sure that the seed point has not left the intended voxel
          const Eigen::Vector3f new_v_float(transform.scanner2voxel.cast<float>() * p);
          const Eigen::Vector3i new_v(
              std::round(new_v_float[0]), std::round(new_v_float[1]), std::round(new_v_float[2]));
          good_seed = (new_v == v);
        }
      }
      if (good_seed) {
        d = fixel.get_dir().cast<float>();
#ifdef DYNAMIC_SEED_DEBUGGING
        write_seed(p);
#endif
        attempts.fetch_add(this_attempts, std::memory_order_relaxed);
        seeds.fetch_add(1, std::memory_order_relaxed);
        fixel.update_prob(seed_prob, true);
        return true;
      }
    }

    fixel.update_prob(seed_prob, false);
  }
  return false;
}

bool Dynamic::operator()(const FMLS::FOD_lobes &in) {
  if (!SIFT::ModelBase<Fixel_TD_seed>::operator()(in))
    return false;
  VoxelAccessor v(accessor());
  assign_pos_of(in.vox).to(v);
  if (v.value()) {
    for (DWI::Fixel_map<Fixel>::Iterator i = begin(v); i; ++i)
      i().set_voxel(in.vox);
  }
  return true;
}

#ifdef DYNAMIC_SEED_DEBUGGING
void Dynamic::write_seed(const Eigen::Vector3f &p) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);
  std::vector<Eigen::Vector3f> tck;
  tck.push_back(p);
  seed_output(tck);
}

void Dynamic::output_fixel_images(std::string_view prefix) {
  Image<float> image_seedprobability(
      Image<float>::create(Path::join(debugging_fixel_path, prefix + "_seedprobability.mif"), H_fixeldata));
  Image<float> image_logseedprob(
      Image<float>::create(Path::join(debugging_fixel_path, prefix + "_seedlogprob.mif"), H_fixeldata));
  Image<float> image_TD(Image<float>::create(Path::join(debugging_fixel_path, prefix + "_TD.mif"), H_fixeldata));
  Image<float> image_reconratio(
      Image<float>::create(Path::join(debugging_fixel_path, prefix + "_reconratio.mif"), H_fixeldata));
  for (auto l = Loop(0)(image_seedprobability, image_logseedprob, image_TD, image_reconratio); l; ++l) {
    const size_t fixel_index = image_seedprobability.index(0);
    image_seedprobability.value() = fixels[fixel_index].get_old_prob();
    image_logseedprob.value() = std::log10(image_seedprobability.value());
    image_TD.value() = mu() * fixels[fixel_index].get_TD();
    image_reconratio.value() = image_TD.value() / fixels[fixel_index].get_FOD();
  }
  Header H_fixelmask(H_fixeldata);
  H_fixelmask.datatype() = DataType::Bit;
  Image<float> fixelmask_image(
      Image<float>::create(Path::join(debugging_fixel_path, prefix + "_fixelmask.mif"), H_fixelmask));
  for (auto l = Loop(0)(fixelmask_image); l; ++l)
    fixelmask_image.value() = fixels[fixelmask_image.index(0)].can_update();
}

#endif

void Dynamic::perform_fixel_masking() {
  // IDEA Rather than a hard masking, could this be instead used to 'damp' how much the seeding
  //   probabilities can be perturbed? (This could perhaps be done by scaling R if less than zero)
  // IDEA Should the initial seeding probabilities be modulated according to the number of fixels
  //   in each voxel? (i.e. equal probability per voxel = WM seeding)
  // Although this may make it more homogeneous, it may actually be detrimental due to the
  //   biases in our particular tracking
  for (size_t i = 1; i != fixels.size(); ++i) {
    // Unfortunately this is going to be model-dependent...
    if (fixels[i].get_FOD() * fixels[i].get_weight() < 0.1)
      fixels[i].mask();
  }
}

bool WriteKernelDynamic::operator()(const Tracking::GeneratedTrack &in, Tractography::Streamline<> &out) {
  out.set_index(writer.count);
  out.weight = 1.0F;
  if (!WriteKernel::operator()(in)) {
    out.clear();
    // Flag to indicate that tracking has completed, and threads should therefore terminate
    out.weight = 0.0F;
    // Actually need to pass this down the queue so that the seeder thread receives it and knows to terminate
    return true;
  }
  out = in;
  return !out.empty(); // New pipe functor interpretation: Don't bother sending empty tracks
}

} // namespace MR::DWI::Tractography::Seeding
