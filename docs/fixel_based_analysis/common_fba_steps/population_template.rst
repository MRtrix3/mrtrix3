Population template creation is one of the most time consuming steps in a
fixel-based analysis. If you have a very large number of subjects in your
study, you can opt to create the template from a limited subset of 30-40
individuals. Typically, subjects are chosen so the generated template is
representative of your population (e.g. similar number of patients and
controls, though avoid patients with excessive abnormalities compared to
the rest of the population). To build a template, put all FOD images in
a single folder and put a set of corresponding mask images (with the same
prefix as the FOD images) in another folder (using masks speeds up
registration significantly)::

      mkdir -p ../template/fod_input
      mkdir ../template/mask_input

