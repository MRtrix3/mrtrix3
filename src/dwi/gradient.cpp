/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#include "dwi/gradient.h"

namespace MR
{
  namespace DWI
  {

    using namespace App;

    const OptionGroup GradOption = OptionGroup ("DW gradient encoding options")
      + Option ("grad",
          "specify the diffusion-weighted gradient scheme used in the acquisition. "
          "The program will normally attempt to use the encoding stored in the image "
          "header. This should be supplied as a 4xN text file with each line is in "
          "the format [ X Y Z b ], where [ X Y Z ] describe the direction of the "
          "applied gradient, and b gives the b-value in units of s/mm^2.")
        + Argument ("encoding").type_file();









    void save_bvecs_bvals (const Image::Header& header, const std::string& path)
    {

      std::string bvecs_path, bvals_path;
      if (path.size() >= 5 && path.substr (path.size() - 5, path.size()) == "bvecs") {
        bvecs_path = path;
        bvals_path = path.substr (0, path.size() - 5) + "bvals";
      } else if (path.size() >= 5 && path.substr (path.size() - 5, path.size()) == "bvals") {
        bvecs_path = path.substr (0, path.size() - 5) + "bvecs";
        bvals_path = path;
      } else {
        bvecs_path = path + "bvecs";
        bvals_path = path + "bvals";
      }

      const Math::Matrix<float>& grad (header.DW_scheme());
      Math::Matrix<float> G (grad.rows(), 3);

      // rotate vectors from scanner space to image space
      Math::Matrix<float> D (header.transform());
      Math::Permutation p (4);
      int signum;
      Math::LU::decomp (D, p, signum);
      Math::Matrix<float> image2scanner (4,4);
      Math::LU::inv (image2scanner, D, p);
      Math::Matrix<float> rotation = image2scanner.sub (0,3,0,3);
      Math::Matrix<float> grad_G = grad.sub (0, grad.rows(), 0, 3);
      Math::mult (G, float(0.0), float(1.0), CblasNoTrans, grad_G, CblasTrans, rotation);

      // deal with FSL requiring gradient directions to coincide with data strides
      // also transpose matrices in preparation for file output
      std::vector<size_t> order = Image::Stride::order (header, 0, 3);
      Math::Matrix<float> bvecs (3, grad.rows());
      Math::Matrix<float> bvals (1, grad.rows());
      for (size_t n = 0; n < G.rows(); ++n) {
        bvecs(0,n) = header.stride(order[0]) > 0 ? G(n,order[0]) : -G(n,order[0]);
        bvecs(1,n) = header.stride(order[1]) > 0 ? G(n,order[1]) : -G(n,order[1]);
        bvecs(2,n) = header.stride(order[2]) > 0 ? G(n,order[2]) : -G(n,order[2]);
        bvals(0,n) = grad(n,3);
      }

      bvecs.save (bvecs_path);
      bvals.save (bvals_path);

    }





  }
}


