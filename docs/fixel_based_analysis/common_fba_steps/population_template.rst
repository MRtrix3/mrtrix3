Population template creation is the most time consuming step in a fixel-based analysis. If you have a large number of subjects in your study, we recommend building the template from a subset of 20-40 individuals. Subjects should be chosen to ensure the generated template is representative of your population (i.e. equal number of patients and controls). To build a template, place all FOD images in a single folder. We also recommend placing a set of corresponding mask images (with the same prefix as the FOD images) in another folder. Using masks can speed up registration significantly::

      mkdir -p ../template/fod_input
      mkdir ../template/mask_input
