Different subjects have different brain coverage. To ensure subsequent analysis is performed in voxels that contain data from all subjects, we warp all subject masks into template space and compute the template mask as the intersection of all subject masks in template space. To warp all masks into template space::

    foreach * : mrtransform IN/dwi_mask_upsampled.mif -warp IN/subject2template_warp.mif -interp nearest -datatype bit IN/dwi_mask_in_template_space.mif

Compute the template mask as the intersection of all warped masks::

    mrmath */dwi_mask_in_template_space.mif min ../template/template_mask.mif -datatype bit

.. WARNING:: It is absolutely **crucial** to check at this stage that the resulting template mask includes *all* regions of the brain that are intended to be analysed. If this is not the case, the cause will be either an individual subject mask which did not include a certain region or the template building or registration having gone wrong for one or more subjects. It is advised to **go back** to these steps, and identify and resolve the problem before continuing any further.

.. NOTE:: It is possible at this stage to edit the template mask and remove certain regions that are either not part of the brain (though these are unlikely to survive the intersection step to begin with, as they would have to be present in *all* subject masks for this to happen), or otherwise not of interest. However, **do not add regions to the mask** at this stage. If there is a genuine need to do this, go back to the relevant steps which caused the exclusion of these regions to begin with (see also the above warning).

