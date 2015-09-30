/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2013.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/



#include <limits>
#include <vector>

#include "exception.h"
#include "image.h"
#include "memory.h"
#include "thread.h"

#include "algo/iterator.h"

#include "file/ofstream.h"

#include "math/rng.h"
#include "math/SH.h"

#include "dwi/fmls.h"
#include "dwi/gradient.h"
#include "dwi/sdeconv/constrained.h"



namespace MR {
namespace DWI {
namespace RF {



class SFThresholds;
class FODSegResult;



class SFThresholds
{
  public:
    SFThresholds (const default_type volume_ratio) :
        volume_ratio   (volume_ratio),
        min_integral   (0.0),
        max_integral   (std::numeric_limits<default_type>::max()),
        max_dispersion (std::numeric_limits<default_type>::max()) { }

    default_type get_volume_ratio  () const { return volume_ratio; }
    default_type get_min_integral  () const { return min_integral; }
    default_type get_max_integral  () const { return max_integral; }
    default_type get_max_dispersion() const { return max_dispersion; }

    void update (const std::vector<FODSegResult>&, const default_type, const default_type, const size_t);

  private:
    default_type volume_ratio, min_integral, max_integral, max_dispersion;
};





class FODSegResult
{
  public:
    FODSegResult (const DWI::FMLS::FOD_lobes& lobes) :
        vox (lobes.vox),
        peak_dir (lobes[0].get_peak_dir()),
        integral (lobes[0].get_integral()),
        dispersion (integral / lobes[0].get_peak_value())
    {
      assert (lobes.size());
      default_type sum_integrals = 0.0;
      for (size_t i = 1; i != lobes.size(); ++i)
        sum_integrals += lobes[i].get_integral();
      volume_ratio = sum_integrals / lobes[0].get_integral();
    }

    FODSegResult() : vox (), peak_dir (), integral (NAN), dispersion (NAN), volume_ratio (NAN) { }

    bool is_sf (const SFThresholds& thresholds) const;

    const Eigen::Array3i&  get_vox()      const { return vox; }
    const Eigen::Vector3f& get_peak_dir() const { return peak_dir; }

    default_type get_integral()     const { return integral; }
    default_type get_dispersion()   const { return dispersion; }
    default_type get_volume_ratio() const { return volume_ratio; }


  private:
    Eigen::Array3i vox;
    Eigen::Vector3f peak_dir;
    default_type integral, dispersion, volume_ratio;

};





class FODCalcAndSeg
{
  public:
    FODCalcAndSeg (Image<float>& dwi,
                   Image<bool>& mask,
                   const DWI::CSDeconv::Shared& csd_shared,
                   const DWI::Directions::Set& dirs,
                   const size_t lmax,
                   std::vector<FODSegResult>& output) :
        in (dwi),
        mask (mask),
        csd (csd_shared),
        fmls (new DWI::FMLS::Segmenter (dirs, lmax)),
        lmax (lmax),
        output (output),
        mutex (new std::mutex)
    {
      // TODO Still unhappy with the segmentation of small FOD lobes in this context
      // One possibility would be to NOT throw out negative lobes, use no thresholds, and instead
      //   quantify the volume ratio using the sum of both positive and negative lobes
      //   other than the dominant lobe
      //fmls->set_ratio_to_negative_lobe_integral (0.0);
      //fmls->set_ratio_to_negative_lobe_mean_peak (0.0);
      //fmls->set_peak_value_threshold (0.0);
      fmls->set_ratio_of_peak_value_to_merge (1.0); // NEVER merge lobes with distinct peaks
      fmls->set_create_null_lobe (false);
      fmls->set_create_lookup_table (false); // No need for it
    }


    FODCalcAndSeg (const FODCalcAndSeg& that) :
        in     (that.in),
        mask   (that.mask),
        csd    (that.csd),
        fmls   (that.fmls),
        lmax   (that.lmax),
        output (that.output),
        mutex  (that.mutex) { }

    ~FODCalcAndSeg() { }


    bool operator() (const Iterator& pos);


  private:
    Image<float> in;
    Image<bool> mask;
    DWI::CSDeconv csd;
    std::shared_ptr<DWI::FMLS::Segmenter> fmls;
    const size_t lmax;
    std::vector<FODSegResult>& output;

    std::shared_ptr<std::mutex> mutex;

};




class SFSelector
{

  public:
    SFSelector (const std::vector<FODSegResult>& results,
                const SFThresholds& thresholds,
                Image<bool>& output_mask) :
        input      (results),
        thresholds (thresholds),
        it         (input.begin()),
        output     (output_mask) { }

    SFSelector (const SFSelector& that) :
        input      (that.input),
        thresholds (that.thresholds),
        it         (that.it),
        output     (that.output)
    {
      throw Exception ("Do not instantiate copy constructor of SFSelector class!");
    }

    bool operator() (FODSegResult& out);

  private:
    const std::vector<FODSegResult>& input;
    const SFThresholds& thresholds;

    std::vector<FODSegResult>::const_iterator it;
    Image<bool> output;

};






// TODO If implementing the (bias-corrected) gradient descent version of response function determination,
//   this may exist as an alternative Response class that stores all individual voxel SH data using the += operator,
//   and performs the final estimation in the result() function; ResponseEstimator then becomes a templated class
class Response
{
  public:

    typedef Eigen::Matrix<default_type, Eigen::Dynamic, 1> vector_t;

    Response (const size_t lmax) :
        data (vector_t::Zero (lmax/2+1)),
        count (0) { }

    Response& operator+= (const vector_t& i)
    {
      assert (i.size() == data.size());
      data += i;
      ++count;
      return *this;
    }

    vector_t result() const
    {
      assert (count);
      vector_t result (data);
      result /= default_type(count);
      return result;
    }

    size_t get_count() const { return count; }

  private:
    vector_t data;
    size_t count;
};






class ResponseEstimator
{

  public:
    ResponseEstimator (Image<float>& dwi_data,
                       const DWI::CSDeconv::Shared& csd_shared,
                       const size_t lmax,
                       Response& output) :
        dwi    (dwi_data),
        shared (csd_shared),
        lmax   (lmax),
        output (output),
        mutex  (new std::mutex()) { }

    ResponseEstimator (const ResponseEstimator& that) :
        dwi    (that.dwi),
        shared (that.shared),
        lmax   (that.lmax),
        output (that.output),
        rng    (that.rng),
        mutex  (that.mutex) { }


    bool operator() (const FODSegResult&);


  private:
    Image<float> dwi;
    const DWI::CSDeconv::Shared& shared;
    const size_t lmax;
    Response& output;

    mutable Math::RNG::Uniform<default_type> rng;

    std::shared_ptr<std::mutex> mutex;

    Eigen::Matrix<default_type, 3, 3> gen_rotation_matrix (const Eigen::Vector3&) const;

};




}
}
}


