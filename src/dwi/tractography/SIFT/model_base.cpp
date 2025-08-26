/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "dwi/tractography/SIFT/model_base.h"

#include "adapter/reslice.h"
#include "interp/cubic.h"
#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/resample.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



        const App::OptionGroup SIFTModelWeightsOption = App::OptionGroup ("Options for setting the model weights for SIFT fixel-tractogram comparisons")

          + App::Option ("model_weights", "provide an image containing the model weights for the model; can be fixel-wise or voxel-wise data")
            + App::Argument ("image").type_image_in()

          + App::Option ("act", "use an ACT five-tissue-type segmented anatomical image to derive appropriate model weights")
            + App::Argument ("image").type_image_in();




        ModelBase::ModelBase (const std::string& fd_path) :
            MR::Fixel::Dataset (Path::dirname (fd_path)),
            // TODO More robust way to define initial number of columns
            fixels (nfixels(), 4),
            FD_sum (0.0),
            TD_sum (0.0),
            dummy_dixelmask (have_fixel_masks() ?
                             Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic>() :
                             Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic>::Zero (1, 1))
        {
          const std::string act_5tt_path = App::get_option_value<std::string>("act", "");
          if (act_5tt_path.size())
            load_5tt_image (act_5tt_path);

          const std::string weights_path = App::get_option_value<std::string>("model_weights", "");
          set_model_weights (weights_path);

          Image<float> fd_image (Image<float>::open (fd_path));
          MR::Fixel::check_data_file (fd_image, nfixels());
          if (fd_image.size(1) != 1)
            throw Exception ("Input fibre density fixel data file must be 1D");
          for (auto l = MR::Loop(0) (fd_image); l; ++l) {
            FixelBase fixel ((*this)[fd_image.index(0)]);
            fixel.set_fd(fd_image.value());
            FD_sum += fixel.fd() * fixel.weight();
          }
        }






        void ModelBase::map_streamlines (const std::string& path)
        {
          Tractography::Properties properties;
          Tractography::Reader<> file (path, properties);

          const track_t count = (properties.find ("count") == properties.end()) ? 0 : to<track_t>(properties["count"]);
          if (!count)
            throw Exception ("Cannot map streamlines: track file \"" + Path::basename(path) + "\" is empty");

          Mapping::TrackLoader loader (file, count);
          // The base class is used _both_:
          //   - To define the voxel grid as the target for mapping
          //   - To provide the target fixels
          Mapping::TrackMapperBase mapper (*this, *this);
          mapper.set_upsample_ratio (Mapping::determine_upsample_ratio (*this, properties, 0.1));
          mapper.set_use_precise_mapping (true);
          Thread::run_queue (
              loader,
              Thread::batch (Tractography::Streamline<float>()),
              Thread::multi (mapper),
              Thread::batch (Mapping::Set<Mapping::Fixel>()),
              *this);

          INFO ("Proportionality coefficient after streamline mapping is " + str (mu()));

          tractogram_path = path;
        }




        void ModelBase::scale_FDs_by_GM ()
        {

          if (!act_5tt.valid())
            throw Exception ("Cannot scale fibre densities according to GM fraction; no ACT image data provided");
          // Loop through voxels, getting total GM fraction for each, and scale all fixels in each voxel
          FD_sum = 0.0;
          for (auto loop_voxel = MR::Loop(*this) (*this, act_5tt); loop_voxel; ++loop_voxel) {
            Tractography::ACT::Tissues tissues (act_5tt);
            const value_type multiplier = 1.0 - tissues.get_cgm() - (0.5 * tissues.get_sgm()); // Heuristic
            for (auto f : value()) {
              FixelBase fixel ((*this)[f]);
              fixel.set_fd(fixel.fd() * multiplier);
              FD_sum += fixel.weight() * fixel.fd();
            }
          }
        }



        bool ModelBase::operator() (const Mapping::Set<Mapping::Fixel>& in)
        {
          value_type total_contribution = 0.0;
          for (const auto& i : in) {
            FixelBase fixel ((*this)[MR::Fixel::index_type(i)]);
            fixel += i.get_length();
            total_contribution += fixel.weight() * i.get_length();
          }
          TD_sum += total_contribution;
          return true;
        }



        value_type ModelBase::calc_cost_function() const
        {
          const value_type current_mu = mu();
          value_type cost = 0.0;
          for (size_t i = 0; i != nfixels(); ++i)
            cost += (*this)[i].get_cost (current_mu);
          return cost;
        }









        void ModelBase::load_5tt_image (const std::string& path)
        {
          auto H_in = Header::open (path);
          ACT::verify_5TT_image (H_in);

          if (voxel_grids_match_in_scanner_space (H_in, *this)) {
            INFO ("5TT image voxel grid matches fixel dataset; importing directly");
            act_5tt = H_in.get_image<float>();
          } else {
            INFO ("5TT image voxel grid does not match fixel dataset; regridding necessary");
            auto in_5tt = H_in.get_image<float>();
            Header H_5tt (*this);
            H_5tt.ndim() = 4;
            H_5tt.size(3) = 5;
            H_5tt.datatype() = DataType::Float32;
            H_5tt.datatype().set_byte_order_native();
            act_5tt = Image<float>::scratch (H_5tt, "5TT scratch buffer");
            auto threaded_loop = ThreadedLoop ("resampling ACT 5TT image to fixel dataset space", act_5tt, 0, 3);
            ACT::ResampleFunctor functor (in_5tt, act_5tt);
            threaded_loop.run (functor);
          }
        }





        void ModelBase::set_model_weights (const std::string& path)
        {
          // There are multiple ways in which the model weights could be defined:
          // - User provides a fixel data file
          //   - Make sure to verify the contents: values need to lie between 0 and 1 inclusive
          // - User provides a voxel image
          //   - If voxel grid matches, do a nested loop across voxels then fixels
          //     - Make sure to verify the contents
          //   - If voxel grid doesn't match, issue a warning, and resample, clamping to [0.0, 1.0]
          // - User does not provide weights data, but does provide 5TT image
          //   - Image will have already been regridded into member "act_5tt" if necessary
          //   - Calculate model weights for all voxels with at least one fixel
          // - User provides nothing
          //   - All fixels get a weight of 1.0 (at least initially)

          if (path.size()) {

            Header header (Header::open (path));
            if (MR::Fixel::is_data_file (header)) {
              MR::Fixel::check_data_file (header, nfixels());
              if (header.size(1) > 1)
                throw Exception ("Fixel data file containing model weights can only have one column");
              Image<float> image = header.get_image<float>();
              fixels.col(model_weight_column) = image.row(0);
              const value_type min_weight = fixels.col(model_weight_column).minCoeff();
              const value_type max_weight = fixels.col(model_weight_column).maxCoeff();
              if (min_weight < 0.0 || max_weight > 1.0)
                throw Exception ("Fixel-wise model weights must be within range [0.0, 1.0]; "
                                 "user-provided data \"" + path + "\" contains values "
                                 "[" + str(min_weight) + ", " + str(max_weight) + "]");
            } else {
              if (!(header.ndim() == 3 || (header.ndim() == 4 && header.size(3) == 1)))
                throw Exception ("Model weights provided as a volumetric image must be a 3D image");
              auto image = header.get_image<float>();
              if (voxel_grids_match_in_scanner_space (header, *this)) {
                INFO ("User-provided model weights image lies on same voxel grid as fixel dataset; "
                      "values will be imported directly");
                for (auto l = MR::Loop("Loading user-provided model weights image", *this) (*this, image); l; ++l) {
                  if (count()) {
                    const value_type weight = image.value();
                    if (!(weight >= 0.0 && weight <= 1.0))
                      throw Exception ("Invalid model weight of " + str(weight) + " observed in model weights image \"" + path + "\"; "
                                       "values must reside within range [0.0, 1.0]");
                    for (auto f : value())
                      fixels(f, model_weight_column) = weight;
                  }
                }
              } else {
                WARN ("User-provided model weights image \"" + path + "\" does not reside on same voxel grid as fixel dataset; "
                      "image will be explicitly interpolated and clamped to range [0.0, 1.0]");
                Adapter::Reslice<Interp::Cubic, Image<float>> reslice (header.get_image<float>(), *this);
                for (auto l = MR::Loop("Reslicing model weights image to fixel dataset voxel grid", *this) (*this, reslice); l; ++l) {
                  if (count()) {
                    const value_type weight = std::max(0.0, std::min(1.0, value_type(reslice.value())));
                    for (auto f : value())
                      fixels(f, model_weight_column) = weight;
                  }
                }
              }
            }

          } else if (act_5tt.valid()) {

            INFO ("User has not provided model weights data, but has provided an ACT 5TT image; "
                  "appropriate model weights will be derived from the 5TT image");
            act_5tt.index(3) = 2; // Access the WM fraction
            bool allzero = true;
            for (auto l = MR::Loop (act_5tt, 0, 3) (act_5tt, *this); l; ++l) {
              // Model weight is the square of the WM fraction
              value_type weight = Math::pow2<value_type> (act_5tt.value());
              if (weight > 0.0)
                allzero = false;
              else if (!std::isfinite (weight))
                weight = 0.0f;
              for (auto f : value())
                fixels(f, model_weight_column) = weight;
            }
            if (allzero)
              throw Exception ("Model weights from ACT 5TT image are all empty; check 5TT image / registration");

          } else {

            INFO ("User has not provided either model weights data or an ACT 5TT image; "
                  "all fixels will contribute equally to the model");
            fixels.col(model_weight_column).setOnes();

          }
        }





        void ModelBase::output_5tt_image (const std::string& path)
        {
          if (!have_act_data())
            throw Exception ("Cannot export 5TT image; no such data present");
          save (act_5tt, path);
        }

        void ModelBase::initialise_debug_image_output (const std::string& dirpath) const
        {
          Fixel::copy_all_integral_files (directory_path, dirpath);
          // These images do not change between before and after filtering
          output_target_voxel (Path::join (dirpath, "target_voxel.mif"));
#ifdef SIFT_MODEL_OUTPUT_SH_IMAGES
          output_target_sh (Path::join (dirpath, "target_sh.mif"));
#endif
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
          output_target_fixel (Path::join (dirpath, "target_fixel.mif"));
#endif
        }

        void ModelBase::output_all_debug_images (const std::string& dirpath, const std::string& prefix) const
        {
          output_tdi_voxel (Path::join (dirpath, prefix + "_tdi_voxel.mif"));
#ifdef SIFT_MODEL_OUTPUT_SH_IMAGES
          output_tdi_sh (Path::join (dirpath, prefix + "_tdi_sh.mif"));
#endif
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
          output_tdi_fixel (Path::join (dirpath, prefix + "_tdi_fixel.mif"));
#endif

          output_errors_voxel (dirpath, prefix + "_maxabsdiff_voxel.mif", prefix + "_diff_voxel.mif", prefix + "_cost_voxel.mif");
#ifdef SIFT_MODEL_OUTPUT_FIXEL_IMAGES
          output_errors_fixel (dirpath, prefix + "_diff_fixel.mif", prefix + "_cost_fixel.mif");
#endif
          output_scatterplot (Path::join (dirpath, prefix + "_scatterplot.csv"));
        }






      void ModelBase::output_target_voxel (const std::string& path) const
      {
        IndexImage index (*this);
        auto out = Image<float>::create (path, *this);
        for (auto l = Loop(out) (out, index); l; ++l) {
          if (index.count()) {
            value_type sum = 0.0;
            for (auto f : index.value())
              sum += (*this)[f].fd();
            out.value() = sum;
          } else {
            out.value() = NAN;
          }
        }
      }

      void ModelBase::output_target_sh (const std::string& path) const
      {
        const size_t L = 8;
        const size_t N = Math::Sphere::SH::NforL (L);
        Math::Sphere::SH::aPSF<value_type> aPSF (L);
        Header H_sh (*this);
        H_sh.ndim() = 4;
        H_sh.size(3) = N;
        H_sh.stride (3) = 0;
        auto out = Image<float>::create (path, H_sh);
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> sum = Eigen::Matrix<value_type, Eigen::Dynamic, 1>::Zero (N);
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> lobe;
        IndexImage index (*this);
        for (auto l = Loop (index) (out, index); l; ++l) {
          if (index.count()) {
            sum.setZero();
            for (auto f : index.value()) {
              const value_type fd = (*this)[f].fd();
              if (fd) {
                aPSF (lobe, dir(f));
                sum += lobe * fd;
              }
            }
            out.row(3) = sum;
          } else {
            out.row(3) = std::numeric_limits<float>::quiet_NaN();
          }
        }
      }

      void ModelBase::output_target_fixel (const std::string& path) const
      {
        Header H (MR::Fixel::data_header_from_nfixels (nfixels()));
        Image<float> image (Image<float>::create (path, H));
        for (auto l = Loop(0) (image); l; ++l)
          image.value() = (*this)[image.index(0)].fd();
      }

      void ModelBase::output_tdi_voxel (const std::string& path) const
      {
        const value_type current_mu = mu();
        auto out = Image<float>::create (path, *this);
        IndexImage index (*this);
        for (auto l = Loop (out) (out, index); l; ++l) {
          if (index.count()) {
            value_type sum = 0.0;
            for (auto i : index.value())
              sum += (*this)[i].td();
            out.value() = sum * current_mu;
          } else {
            out.value() = std::numeric_limits<float>::quiet_NaN();
          }
        }
      }

      void ModelBase::output_tdi_sh (const std::string& path) const
      {
        const value_type current_mu = mu();
        const size_t L = 8;
        const size_t N = Math::Sphere::SH::NforL (L);
        Math::Sphere::SH::aPSF<value_type> aPSF (L);
        Header H_sh (*this);
        H_sh.ndim() = 4;
        H_sh.size(3) = N;
        H_sh.stride (3) = 0;
        auto out = Image<float>::create (path, H_sh);
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> sum = Eigen::Matrix<value_type, Eigen::Dynamic, 1>::Zero (N);
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> lobe;
        IndexImage index (*this);
        for (auto l = Loop (index) (out, index); l; ++l) {
          if (index.count()) {
            sum.setZero();
            for (auto i : index.value()) {
              const value_type td = (*this)[i].td();
              if (td) {
                aPSF (lobe, dir(i));
                sum += lobe * td;
              }
              out.row(3) = sum * current_mu;
            }
          } else {
            out.row(3) = std::numeric_limits<float>::quiet_NaN();
          }
        }
      }

      void ModelBase::output_tdi_fixel (const std::string& path) const
      {
        Header H (MR::Fixel::data_header_from_nfixels (nfixels()));
        Image<float> image (Image<float>::create (path, H));
        for (auto l = Loop(0) (image); l; ++l)
          image.value() = (*this)[image.index(0)].td();
      }

      void ModelBase::output_errors_voxel (const std::string& dirpath, const std::string& max_abs_diff_path, const std::string& diff_path, const std::string& cost_path) const
      {
        const value_type current_mu = mu();
        auto out_max_abs_diff = Image<float>::create (Path::join (dirpath, max_abs_diff_path), *this);
        auto out_diff = Image<float>::create (Path::join (dirpath, diff_path), *this);
        auto out_cost = Image<float>::create (Path::join (dirpath, cost_path), *this);
        IndexImage index (*this);
        for (auto l = Loop (index) (index, out_max_abs_diff, out_diff, out_cost); l; ++l) {
          if (index.count()) {
            value_type max_abs_diff = 0.0, diff = 0.0, cost = 0.0;
            for (auto i : index.value()) {
              const auto fixel ((*this)[i]);
              const value_type this_diff = fixel.get_diff (current_mu);
              max_abs_diff = std::max (max_abs_diff, abs (this_diff));
              diff += this_diff;
              cost += fixel.get_cost (current_mu) * fixel.weight();
            }
            out_max_abs_diff.value() = max_abs_diff;
            out_diff.value() = diff;
            out_cost.value() = cost;
          } else {
            out_max_abs_diff.value() = std::numeric_limits<float>::quiet_NaN();
            out_diff.value() = std::numeric_limits<float>::quiet_NaN();
            out_cost.value() = std::numeric_limits<float>::quiet_NaN();
          }
        }
      }

      void ModelBase::output_errors_fixel (const std::string& dirpath, const std::string& diff_path, const std::string& cost_path) const
      {
        const value_type current_mu = mu();
        Header H (MR::Fixel::data_header_from_nfixels (nfixels()));
        Image<float> image_diff (Image<float>::create (Path::join (dirpath, diff_path), H));
        Image<float> image_cost (Image<float>::create (Path::join (dirpath, cost_path), H));
        for (auto l = Loop(0) (image_diff, image_cost); l; ++l) {
          ConstFixelBase fixel ((*this)[image_diff.index(0)]);
          image_diff.value() = fixel.get_diff (current_mu);
          image_cost.value() = fixel.get_cost (current_mu);
        }
      }

      void ModelBase::output_scatterplot (const std::string& path) const
      {
        File::OFStream out (path, std::ios_base::out | std::ios_base::trunc);
        out << "# " << App::command_history_string << "\n";
        const value_type current_mu = mu();
        out << "#Fibre density,Track density (unscaled),Track density (scaled),Weight,\n";
        for (size_t i = 0; i != nfixels(); ++i) {
          const auto fixel ((*this)[i]);
          out << str (fixel.fd()) << "," << str (fixel.td()) << "," << str (fixel.td() * current_mu) << "," << str (fixel.weight()) << ",\n";
        }
      }




      }
    }
  }
}
