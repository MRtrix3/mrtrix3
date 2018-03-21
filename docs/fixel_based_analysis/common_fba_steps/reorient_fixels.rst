Here we reorient the fixels of all subjects in template space based on the local transformation at each voxel in the warps used previously::

    foreach * : fixelreorient IN/fixel_in_template_space_NOT_REORIENTED IN/subject2template_warp.mif IN/fixel_in_template_space
    
After this step, the :code:`fixel_in_template_space_NOT_REORIENTED` folders can be safely removed.

