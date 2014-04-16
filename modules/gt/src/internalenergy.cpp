/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 19/12/13.
    
    This file is part of the Global Tractography module for MRtrix.
    
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

#include "internalenergy.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        
        
//        double InternalEnergyComputer::stageConnect(const Particle *par, const int ep1, Particle *par2, int &ep2)
//        {
//          // new
//          scanNeighbourhood(par, ep1, 0.01);     // TODO Current temperature
//          ParticleEnd pe = pickNeighbour();
//          par2 = pe.par;
//          VAR(par2);
//          ep2 = pe.alpha;
//          double dE = pe.e_conn;
//          // old
//          Particle* par0 = (ep1 == -1) ? par->getPredecessor() : par->getSuccessor();
//          if (par0) {
//            int a = (par0->getPredecessor() == par) ? -1 : +1;
//            dE -= calcEnergy(par, ep1, par0, a);
//          }
//          return dE;
//        }
        
        
        double InternalEnergyComputer::stageConnect(const ParticleEnd& pe1, ParticleEnd &pe2)
        {
          // new
          scanNeighbourhood(pe1.par, pe1.alpha, 0.01);     // TODO Current temperature
          pe2 = pickNeighbour();
          double dE = pe2.e_conn;
          // old
          Particle* par0 = (pe1.alpha == -1) ? pe1.par->getPredecessor() : pe1.par->getSuccessor();
          if (par0) {
            int a = (par0->getPredecessor() == pe1.par) ? -1 : +1;
            dE -= calcEnergy(pe1.par, pe1.alpha, par0, a);
          }
          return dE;
        }
        
        
        
        void InternalEnergyComputer::scanNeighbourhood(const Particle* p, const int alpha0, const double currTemp)
        {
          neighbourhood.resize(1);
          normalization = 1.0;
          
          Point_t ep = p->getEndPoint(alpha0);
          size_t x, y, z;
          pGrid.pos2xyz(ep, x, y, z);
          
          float tolerance2 = Particle::L * Particle::L;   // distance threshold (particle length), hard coded
          float costheta = M_SQRT1_2;                     // angular threshold (45 degrees), hard coded
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
                  d1 = dist2(ep, pe.par->getEndPoint(-1));
                  d2 = dist2(ep, pe.par->getEndPoint(1));
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
          double t = rng.uniform() * normalization;
          ParticleEnd pe;
          for (std::vector<ParticleEnd>::iterator it = neighbourhood.begin(); it != neighbourhood.end(); ++it)
          {
            sum += it->p_suc;
            if (sum >= t) {
              return *it;
            }
          }
          return pe;
        }
        
        
      }
    }
  }
}

