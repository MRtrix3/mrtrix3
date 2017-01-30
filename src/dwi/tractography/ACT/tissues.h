/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __dwi_tractography_act_tissues_h__
#define __dwi_tractography_act_tissues_h__


// If the sum of tissue probabilities is below this threshold, the image is being exited, so a boolean flag is thrown
// The values will however still be accessible
#define TISSUE_SUM_THRESHOLD 0.5



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace ACT
      {


        class Tissues { MEMALIGN(Tissues)

          public:
            Tissues () : cgm (0.0), sgm (0.0), wm (0.0), csf (0.0), path (0.0), is_valid (false) { }

            Tissues (const float cg, const float sg, const float w, const float c, const float p) :
                cgm  (0.0),
                sgm  (0.0),
                wm   (0.0),
                csf  (0.0),
                path (0.0)
            {
              set (cg, sg, w, c, p);
            }

            template <class ImageType>
            Tissues (ImageType& data) :
                cgm  (0.0),
                sgm  (0.0),
                wm   (0.0),
                csf  (0.0),
                path (0.0),
                is_valid (false)
            {
              set<ImageType> (data);
            }


            Tissues (const Tissues& that) :
                cgm  (that.cgm),
                sgm  (that.sgm),
                wm   (that.wm),
                csf  (that.csf),
                path (that.path),
                is_valid (that.is_valid) { }

            Tissues (Tissues& that) :
                cgm  (that.cgm),
                sgm  (that.sgm),
                wm   (that.wm),
                csf  (that.csf),
                path (that.path),
                is_valid (that.is_valid) { }

            bool set (const float cg, const float sg, const float w, const float c, const float p) {
              if (std::isnan (cg) || std::isnan (sg) || std::isnan (w) || std::isnan (c) || std::isnan (p)) {
                cgm = sgm = wm = csf = path = 0.0;
                return ((is_valid = false));
              }
              cgm  = (cg < 0.0) ? 0.0 : ((cg > 1.0) ? 1.0 : cg);
              sgm  = (sg < 0.0) ? 0.0 : ((sg > 1.0) ? 1.0 : sg);
              wm   = (w  < 0.0) ? 0.0 : ((w  > 1.0) ? 1.0 : w );
              csf  = (c  < 0.0) ? 0.0 : ((c  > 1.0) ? 1.0 : c );
              path = (p  < 0.0) ? 0.0 : ((p  > 1.0) ? 1.0 : p );
              return ((is_valid = ((cgm + sgm + wm + csf + path) >= TISSUE_SUM_THRESHOLD)));
            }

            template <class ImageType>
            bool set (ImageType& data)
            {
              data.index(3) = 0; const float cg = data.value();
              data.index(3) = 1; const float sg = data.value();
              data.index(3) = 2; const float w  = data.value();
              data.index(3) = 3; const float c  = data.value();
              data.index(3) = 4; const float p  = data.value();
              return set (cg, sg, w, c, p);
            }

            void reset() {
              cgm = sgm = wm = csf = path = 0.0;
              is_valid = false;
            }

            bool valid() const { return is_valid; }

            float get_cgm()  const { return cgm; }
            float get_sgm()  const { return sgm; }
            float get_wm ()  const { return wm; }
            float get_csf()  const { return csf; }
            float get_path() const { return path; }

            float get_gm () const { return (cgm + sgm); }

            bool is_cgm()  const { return ((cgm  >= sgm) && (cgm  >= wm ) && (cgm  >  csf) && (cgm  >  path)); }
            bool is_sgm()  const { return ((sgm  >  cgm) && (sgm  >= wm ) && (sgm  >  csf) && (sgm  >  path)); }
            bool is_wm ()  const { return ((wm   >  cgm) && (wm   >  sgm) && (wm   >  csf) && (wm   >  path)); }
            bool is_csf()  const { return ((csf  >= cgm) && (csf  >= sgm) && (csf  >= wm ) && (csf  >= path)); }
            bool is_path() const { return ((path >= cgm) && (path >= sgm) && (path >= wm ) && (path >  csf )); }

            bool is_gm() const { return ((get_gm() >= wm) && (get_gm() > csf) && (get_gm() > path)); }

          private:
            float cgm, sgm, wm, csf, path;
            bool is_valid;

        };


        inline std::ostream& operator<< (std::ostream& stream, const Tissues& t)
        {
          stream << "[ " << t.get_cgm() << " " << t.get_sgm() << " " << t.get_wm() << " " << t.get_csf() << " " << t.get_path() <<" ]";
          return (stream);
        }


      }
    }
  }
}

#endif

