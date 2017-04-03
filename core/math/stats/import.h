/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */
#ifndef __math_stats_import_h__
#define __math_stats_import_h__

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "file/path.h"

#include "math/stats/typedefs.h"


namespace MR
{
  namespace Math
  {
    namespace Stats
    {



      /** \addtogroup Statistics
      @{ */
      /*! A base class defining the interface for importing subject data
       * This class defines the interface for how subject data is imported
       * into a GLM measurement matrix. Exactly how the subject data is
       * 'vectorised' will depend on the particular type of data being
       * tested; nevertheless, the data for each subject should be stored
       * in a single column within the measurement matrix (or in some
       * cases, within the design matrix).
       */
      class SubjectDataImportBase
      {
        public:
          SubjectDataImportBase (const std::string& path) :
              path (path) { }

          /*!
          * @param column the column of a matrix into which the data from this
          * particular file should be loaded
          */
          virtual void operator() (Eigen::Block<matrix_type, 1, Eigen::Dynamic>& column) = 0;

          /*!
           * @param index extract the data from this file corresponding to a particular
           * row in the measurements vector
           */
          virtual void operator[] (const size_t index) = 0;

        protected:
          const std::string path;

      };
      //! @}



      // TODO Implementation of the above class would be more difficult for 0.3.15 version of
      //   fixelcfestats because it would need to keep track of a mapping between template
      //   fixel index, and subject voxel / fixel index. Would it be preferable to do the
      //   0.3.16 merge first?



      // During the initial import, the above class can simply be fed one subject at a time
      //   according to per-file path
      // However for use in GLMTTestVariable, a class is needed that stores a list of text files,
      //   where each text file contains a list of file names (one for each subject), and
      //   for each subject a mechanism of data access is spawned & remains open throughout
      //   processing.
      class CohortDataImport
      {
        public:
          CohortDataImport() { }

          // Needs to be its own function rather than the constructor
          //   so that the correct template type can be invoked explicitly
          template <class SubjectDataImport>
          void initialise (const std::string&);

          /*!
           * @param index for a particular element being tested (data will be acquired for
           * all subjects for that element)
           */
          vector_type operator() (const size_t index) const;

          bool valid() const { return files.size(); }

        protected:
          std::vector<std::shared_ptr<SubjectDataImportBase>> files;
      };



      template <class SubjectDataImport>
      void CohortDataImport::initialise (const std::string& path)
      {
        // TODO Read the provided text file one at a time
        // For each file, create an instance of SubjectDataImport
        //   (which must derive from SubjectDataImportBase)
        const std::string directory = Path::dirname (path);
        std::ifstream ifs (path.c_str());
        std::string line;
        while (getline (ifs, line)) {
          std::string filename (Path::join (directory, line));
          size_t p = filename.find_last_not_of(" \t");
          if (std::string::npos != p)
            filename.erase(p+1);
          if (!Path::exists (filename))
            throw Exception ("Reading text file \"" + Path::basename (path) + "\": input data file not found: \"" + filename + "\"");
          files.push_back (std::make_shared<SubjectDataImport> (filename));
        }
      }




    }
  }
}


#endif
