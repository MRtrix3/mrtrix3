Warping images using warps generated from other packages
=========================================================

The :code:`mrtransform` command applies warps in a deformation field format (i.e. where each voxel specifies the scanner space position in the corresponding image). 
However, you can also use :code:`mrtransform` to apply warps generated from other packages that are in any format using the following steps. 

1. **Generate an identity warp** using the input moving image (i.e. the image you wish to warp)::

    warpinit input_fod_image.mif identity_warp[].nii


2. **Compute a MRtrix compatible warp** by transforming the identity warp using your registration of choice. For example if you are using the ANTs registration package::

    for i in {0..2}; 
    do;
      WarpImageMultiTransform 3 identity_warp${i}.nii mrtrix_warp{i}.nii -R template.nii ants_warp.nii ants_affine.txt;
    done;


3. **Correct the mrtrix warp**. When transforming `identity_warp.nii` and producing the mrtrix_warp images, most registration packages will output 0.0 as the default value when the transformation maps outside the input image. This will result in many voxels in the output mrtrix_warp (which is a deformation field) pointing to the origin (0.0, 0.0, 0,0), particularly around the edge of the warp if an affine registration was performed. To correct this and convert all voxels with 0.0,0.0,0.0 to nan,nan,nan (suitable for `mrtransform`)::
    
    warpcorrect mrtrix_warp[].nii mrtrix_warp_corrected.mif


4. **Warp the image**. :code:`mrtransform` can warp any 3D or 4D image, however if the number of volumes in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (i.e. 6, 15, 28 etc), then it  assumes the image to be an FOD image and `reorientation <http://www.ncbi.nlm.nih.gov/pubmed/22183751>`_ is automatically applied. Also note that `FOD modulation <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ can be applied using the option `-modulation`. This preserves the total apparent fibre density across the width of each bundle before and after warping::

    mrtransform input_fod_image.mif -warp mrtrix_warp_corrected.mif warped_fod_image.mif
    

