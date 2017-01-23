/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "command.h"
#include "memory.h"
#include "dwi/fmls.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/SIFT/model_base.h"


using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "obtain an estimate of fibre connectivity between two regions using AFD and streamlines tractography"

  + "This estimate is obtained by determining a fibre volume (AFD) occupied by the pathway "
    "of interest, and dividing by the streamline length."

  + "If only the streamlines belonging to the pathway of interest are provided, then "
    "ALL of the fibre volume within each fixel selected will contribute to the result. "
    "If the -wbft option is used to provide whole-brain fibre-tracking (of which the pathway of "
    "interest should contain a subset), only the fraction of the fibre volume in each fixel "
    "estimated to belong to the pathway of interest will contribute to the result."

  + "Use -quiet to suppress progress messages and output fibre connectivity value only."

  + "For valid comparisons of AFD connectivity across scans, images MUST be intensity "
    "normalised and bias field corrected, and a common response function for all subjects "
    "must be used."

  + "Note that the sum of the AFD is normalised by streamline length to "
    "account for subject differences in fibre bundle length. This normalisation results in a measure "
    "that is more related to the cross-sectional volume of the tract (and therefore 'connectivity'). "
    "Note that SIFT-ed tract count is a superior measure because it is unaffected by tangential yet unrelated "
    "fibres. However, AFD connectivity may be used as a substitute when Anatomically Constrained Tractography "
    "is not possible due to uncorrectable EPI distortions, and SIFT may therefore not be as effective.";



  ARGUMENTS
  + Argument ("image", "the input FOD image.").type_image_in()

  + Argument ("tracks", "the input track file defining the bundle of interest.").type_tracks_in();

  OPTIONS
  + Option ("wbft", "provide a whole-brain fibre-tracking data set (of which the input track file "
                    "should be a subset), to improve the estimate of fibre bundle volume in the "
                    "presence of partial volume")
    + Argument ("tracks").type_tracks_in()

  + Option ("afd_map", "output a 3D image containing the AFD estimated for each voxel.")
    + Argument ("image").type_image_out()

  + Option ("all_fixels", "if whole-brain fibre-tracking is NOT provided, then if multiple fixels within "
                          "a voxel are traversed by the pathway of interest, by default the fixel with the "
                          "greatest streamlines density is selected to contribute to the AFD in that voxel. "
                          "If this option is provided, then ALL fixels with non-zero streamlines density "
                          "will contribute to the result, even if multiple fixels per voxel are selected.");

}


typedef float value_type;
typedef DWI::Tractography::Mapping::SetDixel SetDixel;
typedef DWI::Tractography::SIFT::FixelBase FixelBase;



class Fixel : public FixelBase
{ MEMALIGN(Fixel)
  public:
    Fixel () : FixelBase (), length (0.0) { }
    Fixel (const FMLS::FOD_lobe& lobe) : FixelBase (lobe), length (0.0) { }
    Fixel (const Fixel& that) : FixelBase (that), length (that.length) { }

    void       add_to_selection (const value_type l) { length += l; }
    value_type get_selected_volume (const value_type l) const { return get_TD() ? (get_FOD() * (l / get_TD())) : 0.0; }
    value_type get_selected_volume ()                   const { return get_TD() ? (get_FOD() * (length / get_TD())) : 0.0; }
    value_type get_selected_length() const { return length; }
    bool       is_selected()         const { return length; }

  private:
    value_type length;

};




class AFDConnectivity : public DWI::Tractography::SIFT::ModelBase<Fixel>
{ MEMALIGN(AFDConnectivity)
  public:
    AFDConnectivity (Image<value_type>& fod_buffer, const DWI::Directions::FastLookupSet& dirs, const std::string& tck_path, const std::string& wbft_path) :
        DWI::Tractography::SIFT::ModelBase<Fixel> (fod_buffer, dirs),
        have_wbft (wbft_path.size()),
        all_fixels (false),
        mapper (fod_buffer, dirs),
        v_fod (fod_buffer)
    {
      if (have_wbft) {
        perform_FOD_segmentation (fod_buffer);
        map_streamlines (wbft_path);
      } else {
        fmls.reset (new DWI::FMLS::Segmenter (dirs, Math::SH::LforN (fod_buffer.size (3))));
      }
      mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (fod_buffer, tck_path, 0.1));
      mapper.set_use_precise_mapping (true);
    }


    void set_all_fixels (const bool i) { all_fixels = i; }


    value_type get  (const std::string& path);
    void       save (const std::string& path);


  private:
    const bool have_wbft;
    bool all_fixels;
    DWI::Tractography::Mapping::TrackMapperBase mapper;
    Image<value_type> v_fod;
    copy_ptr<DWI::FMLS::Segmenter> fmls;

    using Fixel_map<Fixel>::accessor;

};



value_type AFDConnectivity::get (const std::string& path)
{

  Tractography::Properties properties;
  Tractography::Reader<value_type> reader (path, properties);
  const size_t track_count = (properties.find ("count") == properties.end() ? 0 : to<size_t>(properties["count"]));
  DWI::Tractography::Mapping::TrackLoader loader (reader, track_count, "summing apparent fibre density within track");

  // If WBFT is provided, this is the sum of (volume/length) across streamlines
  // Otherwise, it's a sum of lengths of all streamlines (for later scaling by mean streamline length)
  double sum_contributions = 0.0;

  size_t count = 0;

  Tractography::Streamline<value_type> tck;
  while (loader (tck)) {
    ++count;

    SetDixel dixels;
    mapper (tck, dixels);
    double this_length = 0.0, this_volume = 0.0;

    for (SetDixel::const_iterator i = dixels.begin(); i != dixels.end(); ++i) {
      this_length += i->get_length();

      // If wbft has not been provided (i.e. FODs have not been pre-segmented), need to
      //   check to see if any data have been provided for this voxel; and if not yet,
      //   run the segmenter
      if (!have_wbft) {

        VoxelAccessor v (accessor());
        assign_pos_of (*i, 0, 3).to (v);
        if (!v.value()) {

          assign_pos_of (*i, 0, 3).to (v_fod);
          DWI::FMLS::SH_coefs fod_data;
          DWI::FMLS::FOD_lobes fod_lobes;

          fod_data.vox[0] = v_fod.index (0); fod_data.vox[1] = v_fod.index (1); fod_data.vox[2] = v_fod.index (2);
          fod_data.resize (v_fod.size (3));
          for (auto i = Loop(3) (v_fod); i; ++i)
            fod_data[v_fod.index (3)] = v_fod.value();

          (*fmls) (fod_data, fod_lobes);
          (*this) (fod_lobes);

        }
      }

      const size_t fixel_index = dixel2fixel (*i);
      Fixel& fixel = fixels[fixel_index];
      fixel.add_to_selection (i->get_length());
      if (have_wbft)
        this_volume += fixel.get_selected_volume (i->get_length());

    }

    if (have_wbft)
      sum_contributions += (this_volume / this_length);
    else
      sum_contributions += this_length;

  }

  if (!have_wbft) {

    // Streamlines define a fixel mask; go through and get all the fibre volumes
    double sum_volumes = 0.0;

    if (all_fixels) {

      // All fixels contribute to the result
      for (vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {
        if (i->is_selected())
          sum_volumes += i->get_FOD();
      }

    } else {

      // Only allow one fixel per voxel to contribute to the result
      VoxelAccessor v (accessor());
      for (auto l = Loop(v) (v); l; ++l) {
        if (v.value()) {
          value_type voxel_afd = 0.0, max_td = 0.0;
          for (Fixel_map<Fixel>::Iterator i = begin (v); i; ++i) {
            if (i().get_selected_length() > max_td) {
              max_td = i().get_selected_length();
              voxel_afd = i().get_FOD();
            }
          }
          sum_volumes += voxel_afd;
        }
      }

    }

    // sum_contributions currently stores sum of streamline lengths;
    //   turn into a mean length, then combine with volume to get a connectivity value
    const double mean_length = sum_contributions / double(count);
    sum_contributions = sum_volumes / mean_length;

  }

  return sum_contributions;

}



void AFDConnectivity::save (const std::string& path)
{
  auto out = Image<value_type>::create (path, Fixel_map<Fixel>::header());
  VoxelAccessor v (accessor());
  for (auto l = Loop(v) (v, out); l; ++l) {
    value_type value = 0.0;
    if (have_wbft) {
      for (Fixel_map<Fixel>::Iterator i = begin (v); i; ++i)
        value += i().get_selected_volume();
    } else if (all_fixels) {
      for (Fixel_map<Fixel>::Iterator i = begin (v); i; ++i)
        value += (i().is_selected() ? i().get_FOD() : 0.0);
    } else {
      value_type max_td = 0.0;
      for (Fixel_map<Fixel>::Iterator i = begin (v); i; ++i) {
        if (i().get_selected_length() > max_td) {
          max_td = i().get_selected_length();
          value = i().get_FOD();
        }
      }
    }
    out.value() = value;
  }
}



void run ()
{
  auto opt = get_options ("wbft");
  const std::string wbft_path = opt.size() ? str(opt[0][0]) : "";

  DWI::Directions::FastLookupSet dirs (1281);
  auto fod = Image<value_type>::open (argument[0]);
  Math::SH::check (fod);
  AFDConnectivity model (fod, dirs, argument[1], wbft_path);

  opt = get_options ("all_fixels");
  model.set_all_fixels (opt.size());

  const value_type connectivity_value = model.get (argument[1]);

  // output the AFD sum using std::cout. This enables output to be redirected to a file without the console output.
  std::cout << connectivity_value << std::endl;

  opt = get_options ("afd_map");
  if (opt.size())
    model.save (opt[0][0]);
}

