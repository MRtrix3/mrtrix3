Here we reorient the direction of all fixels based on the local transformation at each voxel in the warp::

    foreach * : fixelreorient IN/fixel_in_template_space_NOT_REORIENTED IN/subject2template_warp.mif IN/fixel_in_template_space
    
After this step, the :code:`fixel_in_template_space_NOT_REORIENTED` folders can be safely removed.
