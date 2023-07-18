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

#include "dwi/tractography/GT/internalenergy.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        
        double InternalEnergyComputer::stageConnect(const ParticleEnd& pe1, ParticleEnd &pe2)
        {
          // new
          scanNeighbourhood(pe1.par, pe1.alpha, stats.getTint());
          pe2 = pickNeighbour();
          dEint = pe2.e_conn;
          // old
          Particle* par0 = (pe1.alpha == -1) ? pe1.par->getPredecessor() : pe1.par->getSuccessor();
          if (par0) {
            int a = (par0->getPredecessor() == pe1.par) ? -1 : +1;
            dEint -= calcEnergy(pe1.par, pe1.alpha, par0, a);
          }
          return dEint / stats.getTint();
        }
        
        
        
        void InternalEnergyComputer::scanNeighbourhood(const Particle* p, const int alpha0, const double currTemp)
        {
          neighbourhood.resize(1);
          normalization = 1.0;
          
          Point_t ep = p->getEndPoint(alpha0);
          size_t x, y, z;
          pGrid.pos2xyz(ep, x, y, z);
          
          float tolerance2 = Particle::L * Particle::L;   // distance threshold (particle length), hard coded
          float costheta = Math::sqrt1_2;                 // angular threshold (45 degrees), hard coded
          ParticleEnd pe;
          float d1, d2, d, ct;
          
          for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
              for (int k = -1; k <= 1; k++) {
                const ParticleGrid::ParticleVectorType* pvec = pGrid.at(x+i, y+j, z+k);
                if (pvec == NULL)
                  continue;
                
                for (ParticleGrid::ParticleVectorType::const_iterator it = pvec->begin(); it != pvec->end(); ++it) 
                {
                  pe.par = *it;
                  if (pe.par == p)
                    continue;
                  d1 = (ep - pe.par->getEndPoint(-1)).squaredNorm();
                  d2 = (ep - pe.par->getEndPoint(+1)).squaredNorm();
                  d = (d1 < d2) ? d1 : d2;
                  pe.alpha = (d1 < d2) ? -1 : 1;
                  if ( (pe.alpha == -1) ? (pe.par->hasPredecessor() && pe.par->getPredecessor() != p) : (pe.par->hasSuccessor() && pe.par->getSuccessor() != p) )		// Exclude connected endpoints, unless they are connected to the current particle.
                    continue;
                  ct = (-alpha0*pe.alpha) * p->getDirection().dot(pe.par->getDirection());
                  if (d < tolerance2 && ct > costheta)
                  {
                    pe.e_conn = calcEnergy(p, alpha0, pe.par, pe.alpha);
                    pe.p_suc = exp(-pe.e_conn/currTemp);
                    normalization += pe.p_suc;
                    neighbourhood.push_back(pe);
                  }
                }
                
              }
            }
          }
          
        }
        
        
        ParticleEnd InternalEnergyComputer::pickNeighbour()
        {
          double sum = 0.0;
          double t = rng_uniform() * normalization;

          auto neighbour = std::find_if(neighbourhood.begin(), neighbourhood.end(), [&](const ParticleEnd& particle){
            sum += particle.p_suc;
            return sum >= t;
          });

          if(neighbour != neighbourhood.end()){
            return *neighbour;
          }
          else {
            throw Exception("Unable to pick neighbour!");
          }
        }
        
      }
    }
  }
}

