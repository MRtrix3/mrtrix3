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

#include "progressbar.h"

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
      { NOMEMALIGN
        public:
          SubjectDataImportBase (const std::string& path) :
              path (path) { }

          /*!
          * @param row the row of a matrix into which the data from this
          * particular file should be loaded
          */
          virtual void operator() (matrix_type::RowXpr column) const = 0;

          /*!
           * @param index extract the data from this file corresponding to a particular
           * row in the measurements vector
           */
          virtual default_type operator[] (const size_t index) const = 0;

          const std::string& name() const { return path; }

          virtual size_t size() const = 0;

        protected:
          const std::string path;

      };
      //! @}



      // During the initial import, the above class can simply be fed one subject at a time
      //   according to per-file path
      // However for use in GLMTTestVariable, a class is needed that stores a list of text files,
      //   where each text file contains a list of file names (one for each subject), and
      //   for each subject a mechanism of data access is spawned & remains open throughout
      //   processing.
      class CohortDataImport
      { NOMEMALIGN
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

          size_t size() const { return files.size(); }

          std::shared_ptr<SubjectDataImportBase> operator[] (const size_t i) const
          {
            assert (i < files.size());
            return files[i];
          }

          bool allFinite() const;

        protected:
          vector<std::shared_ptr<SubjectDataImportBase>> files;
      };



      template <class SubjectDataImport>
      void CohortDataImport::initialise (const std::string& path)
      {
        // Read the provided text file one at a time
        // For each file, create an instance of SubjectDataImport
        //   (which must derive from SubjectDataImportBase)
        ProgressBar progress ("Importing data from files listed in \"" + Path::basename (path) + "\"");
        const std::string directory = Path::dirname (path);
        std::ifstream ifs (path.c_str());
        if (!ifs)
          throw Exception ("Unable to open subject file list \"" + path + "\"");
        std::string line;
        while (getline (ifs, line)) {
          size_t p = line.find_last_not_of(" \t");
          if (p != std::string::npos)
            line.erase (p+1);
          if (line.size()) {
            const std::string filename (Path::join (directory, line));
            try {
              std::shared_ptr<SubjectDataImport> subject (new SubjectDataImport (filename));
              files.emplace_back (subject);
            } catch (Exception& e) {
              throw Exception (e, "Reading text file \"" + Path::basename (path) + "\": input image data file not found: \"" + filename + "\"");
            }
          }
          ++progress;
        }
      }




    }
  }
}


#endif
