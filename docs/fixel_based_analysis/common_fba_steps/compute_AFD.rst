Here we segment each FOD lobe to identify the number and orientation of fixels in each voxel. The output also contains the apparent fibre density (AFD) value per fixel (estimated as the FOD lobe integral)::

    foreach * : fod2fixel -mask ../template/template_mask.mif IN/fod_in_template_space_NOT_REORIENTED.mif IN/fixel_in_template_space_NOT_REORIENTED -afd fd.mif

Note that in the following steps we will use the more generic shortened acronym Fibre Density (FD) to refer to the AFD metric.

