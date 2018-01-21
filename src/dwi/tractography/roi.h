/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __dwi_tractography_roi_h__
#define __dwi_tractography_roi_h__

#include "app.h"
#include "image.h"
#include "image.h"
#include "interp/linear.h"
#include "math/rng.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      class Properties;


      extern const App::OptionGroup ROIOption;
      void load_rois (Properties& properties);


      class Mask : public Image<bool> { MEMALIGN(Mask)
        public:
          using transform_type = Eigen::Transform<float, 3, Eigen::AffineCompact>;
          Mask (const Mask&) = default;
          Mask (const std::string& name) :
              Image<bool> (__get_mask (name)),
              scanner2voxel (new transform_type (Transform(*this).scanner2voxel.cast<float>())),
              voxel2scanner (new transform_type (Transform(*this).voxel2scanner.cast<float>())) { }

          std::shared_ptr<transform_type> scanner2voxel, voxel2scanner; // Ptr to prevent unnecessary copy-construction

        private:
          static Image<bool> __get_mask (const std::string& name);
      };




      class ROI { MEMALIGN(ROI)
        public:
          ROI (const Eigen::Vector3f& sphere_pos, float sphere_radius) :
            pos (sphere_pos), radius (sphere_radius), radius2 (Math::pow2 (radius)) { }

          ROI (const std::string& spec) :
            radius (NaN), radius2 (NaN)
          {
            try {
              auto F = parse_floats (spec);
              if (F.size() != 4) 
                throw 1;
              pos[0] = F[0];
              pos[1] = F[1];
              pos[2] = F[2];
              radius = F[3];
              radius2 = Math::pow2 (radius);
            }
            catch (...) { 
              DEBUG ("could not parse spherical ROI specification \"" + spec + "\" - assuming mask image");
              mask.reset (new Mask (spec));
            }
          }

          std::string shape () const { return (mask ? "image" : "sphere"); }

          std::string parameters () const {
            return mask ? mask->name() : str(pos[0]) + "," + str(pos[1]) + "," + str(pos[2]) + "," + str(radius);
          }

          bool contains (const Eigen::Vector3f& p) const
          {

            if (mask) {
              Eigen::Vector3f v = *(mask->scanner2voxel) * p;
              Mask temp (*mask); // Required for thread-safety
              temp.index(0) = std::round (v[0]);
              temp.index(1) = std::round (v[1]);
              temp.index(2) = std::round (v[2]);
              if (is_out_of_bounds (temp))
                return false;
              return temp.value();
            }

            return (pos-p).squaredNorm() <= radius2;

          }

          friend inline std::ostream& operator<< (std::ostream& stream, const ROI& roi)
          {
            stream << roi.shape() << " (" << roi.parameters() << ")";
            return stream;
          }



        private:
          Eigen::Vector3f pos;
          float radius, radius2;
          std::shared_ptr<Mask> mask;

      };



      /**
      Contains a state that is passed into and out of ROISubSet::contains, allowing it to be used in an external loop for an array of coordinates
      */
      class ROISubSet_ContainsLoopState {
         MEMALIGN(ROISubSet_ContainsLoopState)


      public:
         ROISubSet_ContainsLoopState(size_t no_rois, bool ordered) {
            ordered_mode = ordered;
            entered_by_roi = vector<bool>();
            reset(no_rois);
            
         }

         /**
         Resets for re-use. ordered_mode and size remain unchanged
         */
         void reset()
         {
            reset(entered_by_roi.size());
         }

         /**
         Resets for re-use. ordered_mode remains unchanged
         */
         void reset(size_t no_rois) {
            if (ordered_mode) {
               last_entered_ROI_index = -1;
               next_legal_ROI_index = 0;
            }
            valid = true;


            entered_by_roi.assign(no_rois, false);
         }

         /**
         Returns true if all ROIs have been entered legally, or if there are no ROIs
         */
         bool all_entered() {
            if (ordered_mode) 
               return valid && (next_legal_ROI_index == entered_by_roi.size());
            else {
               for (size_t i = 0; i < entered_by_roi.size(); i++) {
                  if (!entered_by_roi[i]) 
                     return false;
               }
               return true;
            }
         }


         bool ordered_mode;//true if the parent ROISubSet is in ordered mode
         bool valid;//true if the order at which ROIs have been entered is legal
         size_t last_entered_ROI_index;//ordered only
         size_t next_legal_ROI_index;//ordered only
         vector<bool> entered_by_roi;//unordered only
      };

      

      /**
      Contains a state that is passed into and out of ROISet::contains, allowing it to be used in an external loop for an array of coordinates
      */
      class ROISet_ContainsLoopState {
         MEMALIGN(ROISet_ContainsLoopState)


      public:
         ROISet_ContainsLoopState(size_t no_rois_unordered, size_t no_rois_ordered):
            unordered(ROISubSet_ContainsLoopState(no_rois_unordered, false)),
            ordered(ROISubSet_ContainsLoopState(no_rois_ordered, true))
         {
            
         }

         /**
         Resets for re-use, retaining the same size
         */
         void reset() {
            unordered.reset();
            ordered.reset();
         }

         /**
         Resets for re-use.
         */
         void reset(size_t no_rois_unordered, size_t no_rois_ordered){
            unordered.reset(no_rois_unordered);
            ordered.reset(no_rois_ordered);
         }

         /**
         True if all ROIs are entered. True if there are no ROIs
         */
         bool all_entered() {
            return ordered.all_entered() && unordered.all_entered(); 
         }

         ROISubSet_ContainsLoopState unordered;
         ROISubSet_ContainsLoopState ordered;
      };



      /**
      Collection of ROIs. Can respond as to whether a coordinate is within any of these ROIs.
      If set to ordered mode, contains(ROISubSetLoopState) only responds true if the ROIs are passed through in order
      */
      class ROISubSet {
         MEMALIGN(ROISubSet)
      public:
         ROISubSet(bool ordered_arg) { ordered = ordered_arg; }

         void clear() { R.clear(); }
         size_t size() const { return (R.size()); }
         const ROI& operator[] (size_t i) const { return (R[i]); }
         void add(const ROI& roi) { R.push_back(roi); }

         /**
         Returns true if any ROI contains the specified coordinate.
         Ordering takes no effect
         */
         bool contains(const Eigen::Vector3f& p) const {
            for (size_t n = 0; n < R.size(); ++n)
               if (R[n].contains(p)) return (true);
            return false;
         }

         /**
         Fills retval with true/false as to whether each ROI within this subset contains the specified coordinate.
         Ordering takes no effect.
         */
         void contains(const Eigen::Vector3f& p, vector<bool>& retval) const {
            for (size_t n = 0; n < R.size(); ++n)
               if (R[n].contains(p))
                  retval[n] = true;
         }

         /**
         For use in an external loop. Provides a ROISubSet_ContainsLoopState which can specify whether each ROI within this subset has been entered.
         Ordering takes effect if switched on
         */
         void contains(const Eigen::Vector3f& p, ROISubSet_ContainsLoopState& loop_state) const {

            if (ordered) {
               if (loop_state.valid)//do nothing if the series of cooredinates have already performed something illegal
                  for (size_t n = 0; n < R.size(); ++n)
                     if (R[n].contains(p)) {
                        if (n == loop_state.next_legal_ROI_index) {
                           //entered the next ROI in the list. Legal.
                           loop_state.last_entered_ROI_index = n;
                           loop_state.next_legal_ROI_index = n + 1;
                        }
                        else if (n != loop_state.last_entered_ROI_index) {
                           //entered an ROI in the wrong order
                           //this may be due:
                           //a) the series of coordinates entering ROI X, entering ROI Y, and then re-entering ROI X, or
                           //b) entering ROI[n], when ROI[n-1] has not yet been entered
                           loop_state.valid = false;
                        }
                        break;
                     }

            }
            else {
               for (size_t n = 0; n < R.size(); ++n)
                  loop_state.entered_by_roi[n] = loop_state.entered_by_roi[n] || R[n].contains(p);
            }

         }

         friend inline std::ostream& operator<< (std::ostream& stream, const ROISubSet& R) {
            if (R.R.empty()) return (stream);
            vector<ROI>::const_iterator i = R.R.begin();
            stream << *i;
            ++i;
            for (; i != R.R.end(); ++i) stream << ", " << *i;
            return stream;
         }

      private:
         vector<ROI> R;
         bool ordered;
      };


      /**
      Collection of ROIs. Can respond as to whether a coordinate is within any of these ROIs.
      Internally, this contains two sub-collections of ROIs - one that is ordered and one that is not.
      */
      class ROISet { 
         MEMALIGN(ROISet)


        public:
          ROISet () :
            unordered ( ROISubSet(false)),
            ordered ( ROISubSet(true))
          {
             
          }

          void clear () { 
             unordered.clear();
             ordered.clear();
          }
          size_t size () const { 
             return unordered.size() + ordered.size();
          }
          size_t size_unordered() const {
             return unordered.size();
          }
          size_t size_ordered() const {
             return ordered.size();
          }
          const ROI& operator[] (size_t i) const { 
             if (i < unordered.size()) {
                return unordered[i];
             }
             else {
                return ordered[i - unordered.size()];
             }
          }
          /**
          Adds an ROI to the (internal) unordered list
          */
          void add (const ROI& roi) { unordered.add(roi); }
          /**
          Adds an ROI to the next position in the (internal) ordered list 
          */
          void add_ordered (const ROI& roi) { ordered.add(roi); }

          /**
          Returns true if any ROI contains the specified coordinate
          */
          bool contains (const Eigen::Vector3f& p) const {
             return unordered.contains(p) || ordered.contains(p);
          }

          /**
          For use in an external loop.
          OrderedROIs are tested but their order is ignored 
          */
          void contains (const Eigen::Vector3f& p, vector<bool>& retval) const {
             unordered.contains(p, retval);
             ordered.contains(p, retval);
          }

          /**
          For use in an external loop. Provides a ROISubSet_ContainsLoopState which can specify whether each ROI within this subset has been entered.
          Ordering takes effect for the ordered ROIs
          */
          void contains(const Eigen::Vector3f& p, ROISet_ContainsLoopState& state) const {
             unordered.contains(p, state.unordered);
             ordered.contains(p, state.ordered);
          }
          

          friend inline std::ostream& operator<< (std::ostream& stream, const ROISet& R) {
             stream << R.unordered;
             stream << R.ordered;
             return stream;
          }



      private:
         ROISubSet unordered;
         ROISubSet ordered;

      };



    }
  }
}

#endif


