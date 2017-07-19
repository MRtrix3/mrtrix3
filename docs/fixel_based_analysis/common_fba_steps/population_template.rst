Population template creation is a very time consuming step in a fixel-based analysis. If you have a very large number of subjects in your study, we recommend building the template from a limited subset of 30-40 individuals. Subjects should be chosen to ensure the generated template is representative of your population (e.g. similar number of patients and controls). To build a template, place all FOD images in a single folder. We also highly recommend placing a set of corresponding mask images (with the same prefix as the FOD images) in another folder. Using masks can speed up registration significantly as well as result in a much more accurate template in specific scenarios::

      mkdir -p ../template/fod_input
      mkdir ../template/mask_input
