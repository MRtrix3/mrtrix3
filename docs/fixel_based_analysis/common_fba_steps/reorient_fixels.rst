Here we reorient the direction of all fixels based on the Jacobian matrix (local affine transformation) at each voxel in the warp. Note that in-place fixel reorientation can be performed by specifing the output fixel folder to be the same as the input, and using the :code:`-force` option::

    foreach * : fixelreorient IN/fixel_in_template_space IN/subject2template_warp.mif IN/fixel_in_template_space --force
