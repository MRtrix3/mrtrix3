Different subjects will have subtly different brain coverage. To ensure subsequent analysis is performed in voxels that contain data from all subjects, we warp all subject masks into template space and compute the mask intersection. For each subject::

    foreach * : mrtransform IN/dwi_mask_upsampled.mif -warp IN/subject2template_warp.mif -interp nearest IN/dwi_mask_in_template_space.mif

Compute the intersection of all warped masks::

    mrmath */dwi_mask_in_template_space.mif min ../template/mask_intersection.mif
