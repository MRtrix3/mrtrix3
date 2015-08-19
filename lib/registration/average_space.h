// #include "image/buffer_scratch.h"
#include "eigen/matrix.h"
#include "eigen/gsl_eigen.h"
#include <Eigen/Geometry> // Eigen::Translation

#ifndef __image_average_space_h__
#define __image_average_space_h__

namespace MR
{ 
  template<class ComputeType>
  Eigen::Matrix<ComputeType, 8, 4> get_bounding_box(const Eigen::Matrix<ComputeType, 4, 1>& width){
    if (width.size() != 4)
      throw Exception("get_bounding_box: width has to be 4-dimensional");
    Eigen::Matrix<ComputeType, 8, 4>  corners;
    // MatrixType faces = MatrixType(8,4);
    // faces << 1, 2, 3, 4,   2, 6, 7, 3,   4, 3, 7, 8,   1, 5, 8, 4,   1, 2, 6, 5,   5, 6, 7, 8;
    corners << 0.0, 0.0, 0.0, 1.0,
               0.0, 1.0, 0.0, 1.0,
               1.0, 1.0, 0.0, 1.0, 
               1.0, 0.0, 0.0, 1.0, 
               0.0, 0.0, 1.0, 1.0, 
               0.0, 1.0, 1.0, 1.0, 
               1.0, 1.0, 1.0, 1.0, 
               1.0, 0.0, 1.0, 1.0;
    corners *= width.asDiagonal();  
    return corners;
  }

  template<class ComputeType, class TransformType>
  Eigen::Matrix<ComputeType, 8, 4> get_bounding_box(const Eigen::Matrix<ComputeType, 4, 1>& width, const TransformType& transformation){
    // get image boundary box corners in voxels
    Eigen::Matrix<ComputeType, 8, 4> corners = get_bounding_box<ComputeType>(width);
    // transform voxels into scanner coordinate system
    for (int i=0; i<corners.rows();i++){
      corners.transpose().col(i) = (transformation * corners.transpose().col(i));
    }
    return corners;
  }

  template<class ComputeType, class TransformType>
  Eigen::Matrix<ComputeType, 8, 4> get_bounding_box(const Image::Info& info, const TransformType& voxel2scanner){
    if (info.ndim() < 3)
      throw Exception("get_bounding_box: image dimension has to be >= 3");
    Eigen::Matrix<ComputeType, 4, 1> width = Eigen::Matrix<ComputeType, 4, 1>::Ones(4);
    // width in voxels
    for (size_t i=0; i<3; i++){
      width(i) = info.dim(i) - 1.0;
    }
    // get image boundary box corners in voxels
    Eigen::Matrix<ComputeType, 8, 4>  corners = get_bounding_box<ComputeType, TransformType>(width, voxel2scanner);
    return corners;
  }

  template<class ComputeType, class ImagePtrType>
  Image::Info  compute_minimum_average_info(
      std::vector<ImagePtrType>& input_images, 
      ComputeType voxel_subsampling, 
      Eigen::Matrix<ComputeType, 4, 1>  padding,
      std::vector<Eigen::Transform< ComputeType, 3, Eigen::Projective>>& transform_header_with){
    typedef Eigen::Transform< ComputeType, 3, Eigen::Projective> TransformType;
    typedef Eigen::Matrix<ComputeType, Eigen::Dynamic, Eigen::Dynamic> MatrixType;
    typedef Eigen::Matrix<ComputeType, Eigen::Dynamic, 1> VectorType;
    typedef Eigen::Matrix<ComputeType, 4, 4> MatrixType4;
    typedef std::vector<MatrixType> MatrixArrayType;


    size_t num_images = input_images.size();
    MatrixArrayType transformation_matrices;
    MatrixType bounding_box_corners = MatrixType::Zero(8*num_images,4);
    for (size_t iFile=0; iFile<num_images; iFile++) {
      // Image::ConstHeader header_in  (*input_images[iFile]);

      // get v2s transformation
      Image::Transform   trafo_im (input_images[iFile]->voxel());
      Math::Matrix<float> trafo_m;
      trafo_im.voxel2scanner_matrix (trafo_m);
      TransformType v2s_trafo = TransformType::Identity();
      Eigen::gslMatrix2eigenMatrix<Math::Matrix<float>,decltype(v2s_trafo.matrix())>(trafo_m,v2s_trafo.matrix());

      if (transform_header_with.size()>iFile)
        v2s_trafo = transform_header_with[iFile] * v2s_trafo;

      transformation_matrices.push_back(v2s_trafo.matrix());

      // Image::ConstHeader header (*input_images[iFile]);
      bounding_box_corners.template block<8,4>(iFile*8,0) = get_bounding_box<ComputeType, TransformType>(input_images[iFile]->info(), v2s_trafo);
    }

    // create average space header
    // Image::ConstHeader header_in  (*input_images[0]);
    Image::Info info_out (input_images[0]->info());
    info_out.set_ndim (3); // TODO: do we want to allow 4D output?
    info_out.datatype() = DataType::Float32; 

    VectorType vox_scaling = VectorType(4);
    // set header voxel size: smallest voxel spacing in any template * voxel_subsampling
    for (size_t i=0;i<3;i++){
      for (auto const& in : input_images){
        if ((*in).ndim() >=i)
          info_out.vox(i) = std::min(double(info_out.vox(i)), (*in).vox(i) * voxel_subsampling);
      }
      vox_scaling(i) = 1.0/info_out.vox(i);
    }
    vox_scaling(3) = 1.0;

    MatrixType4 mat_avg;
    Math::matrix_average<MatrixArrayType, MatrixType4>(transformation_matrices, mat_avg);

    TransformType average_v2s_trafo = TransformType::Identity();
    average_v2s_trafo.matrix() = mat_avg;
    TransformType average_v2s_trafo_inverse = average_v2s_trafo.inverse(Eigen::Projective);
    // DEBUG("inverse sanity: " + str( (average_v2s_trafo * average_v2s_trafo_inverse).matrix().isApprox(MatrixType::Identity(4,4)) ));

    // transform all image corners into inverse average space
    MatrixType bounding_box_corners_inv = bounding_box_corners;
    for (int i=0; i<bounding_box_corners_inv.rows(); i++){
      bounding_box_corners_inv.transpose().col(i) = (average_v2s_trafo_inverse * bounding_box_corners.transpose().col(i));
    } 
    // minimum axis-aligned corners in inverse average space 
    VectorType bounding_box_corners_inv_min = bounding_box_corners_inv.colwise().minCoeff();
    VectorType bounding_box_corners_inv_max = bounding_box_corners_inv.colwise().maxCoeff();
    VectorType bounding_box_corners_inv_width = bounding_box_corners_inv_max - bounding_box_corners_inv_min;
    bounding_box_corners_inv_width += 2.0 * padding;
    bounding_box_corners_inv_width(3) = 1.0;

    bounding_box_corners = get_bounding_box<ComputeType>(bounding_box_corners_inv_width);

    // transform boundary box corners back into scanner space
    for (int i=0; i<bounding_box_corners.rows();i++){
      bounding_box_corners.row(i) += bounding_box_corners_inv_min - padding; 
      bounding_box_corners.row(i)(3) = 1.0;
      bounding_box_corners.row(i) = ( average_v2s_trafo * bounding_box_corners.row(i).transpose()).transpose();
    } 

    VectorType bounding_box_corners_width = bounding_box_corners.colwise().maxCoeff() - bounding_box_corners.colwise().minCoeff();
    bounding_box_corners_width(3) = 1.0;
    // INFO("average space axis-aligned minimum bounding box:\n"+str( bounding_box_corners ));
    // DEBUG( str(bounding_box_corners.colwise().minCoeff()));
    // DEBUG( str(bounding_box_corners.colwise().maxCoeff()));
    // DEBUG( str(RowVectorType(bounding_box_corners_width)));

    // set translation to first corner (0, 0, 0, 1)
    average_v2s_trafo.matrix().col(3).template head<3>() = bounding_box_corners.row(0).template  head<3>();
    // average_v2s_trafo_inverse = average_v2s_trafo.inverse();

    // override header transformation
    TransformType average_i2s_trafo = TransformType::Identity();
    average_i2s_trafo.matrix() = average_v2s_trafo.matrix();
    average_i2s_trafo.matrix() *= vox_scaling.asDiagonal();
    // set header transformation
    for (size_t i = 0, nRows = 4, nCols = 4; i < nCols; ++i){
      for (size_t j = 0; j < nRows; ++j){
        info_out.transform()(i,j) = average_i2s_trafo.matrix()(i,j);
      }
    }
    info_out.transform()(3,0) = 0.0;
    info_out.transform()(3,1) = 0.0;
    info_out.transform()(3,2) = 0.0;

    // get scanner 2 voxel trafo
    Math::Matrix<float> s2v; 
    Image::Transform(info_out).scanner2voxel_matrix(s2v);
    MatrixType average_s2v = MatrixType(4,4);
    Eigen::gslMatrix2eigenMatrix<Math::Matrix<float>,MatrixType>(s2v,average_s2v);

    // set header dimensions according to element 7 in bounding_box_corners:
    VectorType ext = VectorType(4);
    ext = bounding_box_corners.row(6);
    ext(3) = 1.0;
    ext = average_s2v *ext;
    for (size_t i=0;i<3;i++){
      info_out.dim(i) = std::ceil(ext(i)) ; 
    }    
    return info_out;
  }

  
}

#endif