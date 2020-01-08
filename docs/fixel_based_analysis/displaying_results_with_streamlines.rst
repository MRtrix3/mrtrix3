Displaying results with streamlines
===================================

Fixels rendered directly as lines using the *fixel plot tool* of :ref:`mrview` are
appropriate for viewing 2D slices; however, to better appreciate all the fibre pathways
affected and to visualise the full extent of the results in *3D*, it's also possible to
use a visualisation based on the whole-brain template-derived tractogram [Raffelt2017]_.

First use :code:`tckedit` to reduce the whole-brain template tractogram to a sensible
number of streamlines (2 million is quite a lot for certain graphics cards to render
smoothly). This step assumes you have the same folder structure and filenames from the
FBA tutorials. From the :code:`template` directory::

    tckedit tracks_2_million_sift.tck -num 200000 tracks_200k_sift.tck

Map fixel values to streamline points, save them in a "track scalar file". For example::

    fixel2tsf stats_fdc/fwe_pvalue.mif tracks_200k_sift.tck fdc_fwe_pvalue.tsf
    fixel2tsf stats_fdc/abs_effect_size.mif tracks_200k_sift.tck fdc_abs_effect_size.tsf

Visualise track scalar files using the tractogram tool in MRview. First load the
streamlines (tracks_200k_sift.tck). Then right click and select 'colour by (track)
scalar file'. For example you might load the :code:`abs_effect_size.tsf` file. Then
to dynamically threshold (remove) streamline points by p-value select the "Thresholds"
dropdown and select "Separate Scalar file" to load :code:`fwe_pvalue.tsf`.

Note that you can also threshold and view all brain fixels by deselecting "crop to slice"
in the fixel plot tool. However it can be harder to appreciate the specific pathways
affected. The downside to viewing and colouring results by streamline, then viewing all
streamlines (uncropped to slice), is that without transparency you only see the colours
on the outside of the significant pathways, where normally the effect size/p-value is most
severe in the 'core' of the fibre pathway.

Good examples of both fixel as well as streamline visualisations of FBA results can be
found in [Mito2018a]_.

