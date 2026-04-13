/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "header.h"
#include "image.h"
#include "file/matrix.h"
#include "file/path.h"
#include "file/utils.h"
#include "file/ofstream.h"

#include "dwi/tractography/file_base.h"

#include "track/matrix.h"

#include "thread_queue.h"

#include "progressbar.h"



using namespace MR;
using namespace App;

#define DEFAULT_SIMILARITY_THRESHOLD 0.0
#define DEFAULT_SIMILARITY_DECAY 5.0

void usage ()
{
  AUTHOR = "Simone Zanoni (simone.zanoni@sydney.edu.au)";

  SYNOPSIS = "Perform a smoothing operation on streamline-wise values";

  DESCRIPTION
  + "This command smooths a per-streamline data file based on streamline similarity. "
    "For each streamline, a new value is computed as a weighted average of the values of its similar streamlines. "
    "Smoothing can be modulated by a similarity threshold, and/or a decay rate.";

  ARGUMENTS
  + Argument ("input", "provides a txt file containing the streamline-wise data to be smoothed").type_file_in()
  + Argument ("output", "provides a txt file containing the per streamline smoothed data").type_file_out()
  + Argument ("matrix", "provides a streamline-streamline similarity matrix folder").type_directory_in();

  OPTIONS

  + Option ("tck_weights_in", "a txt file containing per-track weights")
    + Argument ("value").type_file_in()

  + Option ("threshold", "a threshold to define the smoothing extent (default: " + str(DEFAULT_SIMILARITY_THRESHOLD, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("decay_rate", "a decay rate to modulate the smoothing effect (default: " + str(DEFAULT_SIMILARITY_DECAY, 2) + ")")
    + Argument ("value").type_float (0.0);

  REFERENCES
  + "Zanoni, S.; Lv, J.; Smith, R. E. & Calamante, F. "
    "Streamline-Based Analysis: "
    "A novel framework for tractogram-driven streamline-wise statistical analysis. "
    "Proceedings of the International Society for Magnetic Resonance in Medicine, 2025, 4781";

}



using value_type = float;
using vect_type = Eigen::VectorXf;


class Source
{
public:
    Source(const size_t N) : number(N), counter(0) {}

    bool operator()(size_t& track)
    {
        if ((track = counter) == number)
            return false;
        ++counter;
        return true;
    }

private:
    const size_t number;
    size_t counter;
};


class Worker
{
public:
    Worker(const vect_type& in_samples, std::function<value_type(size_t)> weight_func, vect_type& out_data,
           const Track::Matrix::Reader& matrix_in, const value_type& simthreshold, const value_type& simdecay) :
        in_samples(in_samples), 
        weight_func(weight_func), 
        output_data(out_data), 
        matrix(matrix_in), 
        threshold(simthreshold), 
        decay(simdecay),
        progress(std::make_shared<ProgressBar>("smoothing vector", in_samples.size())) {}

    bool operator()(const size_t track)
    {
        // initialised here to keep it thread-safe
        Image<int> index_img = matrix.get_index_image();
        Image<int> similar_img = matrix.get_streamlines_image();
        Image<value_type> score_img = matrix.get_values_image();

        index_img.move_index(0, track);
        size_t howmany = index_img.get_value();
        index_img.move_index(3, 1);
        int offset = index_img.get_value();

        value_type numerator = 0.0f;
        value_type denominator = 0.0f;
        for (size_t i = 0; i < howmany; ++i) {
          score_img.move_index(0, offset + i);
          similar_img.move_index(0, offset + i);

          value_type curr_sample = in_samples[similar_img.get_value()];
          value_type curr_weight = weight_func(similar_img.get_value());
          value_type curr_score = score_img.get_value();

          // avoids propagating NaNs and this also fixes existing NaNs
          if (curr_score >= threshold && std::isfinite(curr_score)) {
            value_type weight_score = curr_weight * std::pow(curr_score, decay);
            numerator += (curr_sample * weight_score);
            denominator += weight_score;
          }

          score_img.reset();
          similar_img.reset();
        }
        
        index_img.reset();

        value_type result = numerator / denominator;

        output_data[track] = result;

        ++(*progress);

        return true;
    }

private:
    const vect_type& in_samples;
    std::function<value_type(size_t)> weight_func;
    vect_type& output_data;
    const Track::Matrix::Reader& matrix;
    const value_type threshold;
    const value_type decay;
    std::shared_ptr<ProgressBar> progress;
};
  


void run()
{

  // LOAD
  const value_type similarity_threshold = get_option_value ("threshold", value_type(DEFAULT_SIMILARITY_THRESHOLD));
  if (similarity_threshold == 1.0)
    throw Exception("Setting -threshold to 1 would defeat the purpose of the smoothing");
  const value_type similarity_decay = get_option_value ("decay_rate", value_type(DEFAULT_SIMILARITY_DECAY));
  CONSOLE("Ignoring similarity scores that are smaller than " + str(similarity_threshold));
  CONSOLE("Modulating the smoothing weights using a decay rate of " + str(similarity_decay));

  vect_type in_samples = File::Matrix::load_vector<value_type> (argument[0]);

  // weights handling
  auto opt = get_options ("tck_weights_in");
  std::shared_ptr<vect_type> weights_ptr;
  std::function<value_type(size_t)> weight_function;
  if (opt.size()) {
    weights_ptr = std::make_shared<vect_type>(File::Matrix::load_vector<value_type>(opt[0][0]));
    if (weights_ptr->size() != in_samples.size())
      throw Exception("Error: size of in_samples and weights do not match.");
    weight_function = [weights_ptr](size_t idx) { return (*weights_ptr)[idx]; };
  }
  else {
    WARN("No weights provided. Using SIFT2 weights is recommended in the context of Streamline-Based Analysis");
    // This saves memory if weights are not provided by the user
    weight_function = [](size_t) { return 1.0f; };
  }

  Track::Matrix::Reader matrix_obj(argument[2]);

  size_t size_in = in_samples.size();
  if (matrix_obj.size() != size_in) {
    throw Exception("Error: size of in_sample and matrix files do not match");
  }


  // RUN
  vect_type output_data(vect_type::Zero (in_samples.size()));


  Source source(in_samples.size());
  
  Thread::run_queue(
    source,
    Thread::batch(size_t()),
    Thread::multi(Worker(in_samples, weight_function, output_data, matrix_obj, similarity_threshold, similarity_decay))
  );

  File::Matrix::save_vector(output_data, argument[1]);

}
