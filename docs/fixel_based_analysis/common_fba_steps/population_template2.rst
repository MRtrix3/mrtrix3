Run the template building script as follows:

.. code-block:: console

    $ population_template ../template/fod_input -mask_dir ../template/mask_input ../template/wmfod_template.mif -voxel_size 1.25

**If you are building a template from your entire study population**, run the
population_template script use the :code:`-warp_dir warps` option to output a
directory containing all subject warps to the template. Saving the warps here
will enable you to skip the next step. Note that the warps used (and therefore
output) from the population_template script are 5D images containing both
forward and reverse warps (see :ref:`mrregister` for more info). After
population template creation is complete, to convert this warp format to a more
conventional 4D deformation field format ready for the subsequent steps, run

.. code-block:: console

    $ foreach ../template/warps/* : warpconvert -type warpfull2deformation -template ../template/wmfod_template.mif IN PRE/subject2template_warp.mif
