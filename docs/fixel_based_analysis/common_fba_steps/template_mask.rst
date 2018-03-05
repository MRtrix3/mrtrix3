Different subjects have different brain coverage. To ensure subsequent analysis is performed in voxels that contain data from all subjects, we warp all subject masks into template space and compute the template mask as the intersection of all subject masks in template space. For each subject::

    foreach * : mrtransform IN/dwi_mask_upsampled.mif -warp IN/subject2template_warp.mif -interp nearest -datatype bit IN/dwi_mask_in_template_space.mif

Compute the template mask as the intersection of all warped masks::

    mrmath */dwi_mask_in_template_space.mif min ../template/template_mask.mif -datatype bit
