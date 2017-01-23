Population template creation is the most time consuming step in a fixel-based analysis. If you have a large number of subjects in your study, we recommend building the template from a subset of 20-40 individuals. Subjects should be chosen to ensure the generated template is representative of your population (i.e. equal number of patients and controls). To build a template, place all FOD images in a single folder. We also recommend placing a set of corresponding mask images (with the same prefix as the FOD images) in another folder. Using masks can speed up registration significantly::

    mkdir -p ../template/fod_input
    mkdir -p ../template/mask_input

Symbolic link all FOD images (and masks) into a single input folder. If you have fewer than 40 subjects in your study, you can use the entire population to build the template::

    foreach * : ln -sr IN/fod.mif ../template/fod_input/PRE.mif
    foreach * : ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

Alternatively, if you have more than 40 subjects you can randomly select a subset of the individuals. If your study has multiple groups, then ideally you want to select the same number of subjects from each group to ensure the template is un-biased. Assuming the subject directory labels can be used to identify members of each group, you could use::

    foreach `ls -d *patient | sort -R | tail -20` : ln -sr IN/fod.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif
    foreach `ls *control | sort -R | tail -20` : ln -sr IN/fod.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

Run the template building script as follows::

    population_template ../template/fod_input -mask_dir ../template/mask_input ../template/fod_template.mif

**If you are building a template from your entire study population**, run the population_template script use the :code:`-warp_dir warps` option to output a directory containing all subject warps to the template. Saving the warps here will enable you to skip the next step. Note that the warps used (and therefore output) from the population_template script are 5D images containing both forward and reverse warps (see :ref:`mrregister`for more info). To convert this warp format to a more conventional 4D deformation field format ready for the subsequent steps, run::

    foreach ../template/warps/* : warpconvert -type warpfull2deformation -template ../template/fod_template.mif IN PRE/subject2template_warp.mif
