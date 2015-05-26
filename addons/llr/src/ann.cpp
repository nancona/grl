/** \file ann.cpp
 * \brief Approximate nearest neighbor projector source file.
 *
 * \author    Wouter Caarls <wouter@caarls.org>
 * \date      2015-01-22
 *
 * \copyright \verbatim
 * Copyright (c) 2015, Wouter Caarls
 * All rights reserved.
 *
 * This file is part of GRL, the Generic Reinforcement Learning library.
 *
 * GRL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * \endverbatim
 */

#include <ANN/ANN.h>
#include <grl/projectors/ann.h>

#define ANN_MAX_NEIGHBORS 64

using namespace grl;

REGISTER_CONFIGURABLE(ANNProjector)

void ANNProjector::request(const std::string &role, ConfigurationRequest *config)
{
  config->push_back(CRP("samples", "Maximum number of samples to store", max_samples_, CRP::Configuration, 100));
  config->push_back(CRP("neighbors", "Number of neighbor indices to return", neighbors_, CRP::Configuration, 1, ANN_MAX_NEIGHBORS));
  config->push_back(CRP("locality", "Locality of weighing function", locality_, CRP::Configuration, 0., DBL_MAX));
  config->push_back(CRP("bucket_size", "?", bucket_size_, CRP::Configuration, 1));
  config->push_back(CRP("error_bound", "?", error_bound_, CRP::Configuration, 0., DBL_MAX));
  config->push_back(CRP("scaling", "Input dimension scaling", scaling_));
  
  if (role == "observation")
    config->push_back(CRP("inputs", "int.observation_dims", "Number of input dimensions", dims_, CRP::System, 1, SS_MAX_COORDS));
  else if (role == "action")
    config->push_back(CRP("inputs", "int.action_dims", "Number of input dimensions", dims_, CRP::System, 1, SS_MAX_COORDS));
  else if (role == "pair")
    config->push_back(CRP("inputs", "int.observation_dims+int.action_dims", "Number of input dimensions", dims_, CRP::System, 1, SS_MAX_COORDS));
  else
    config->push_back(CRP("inputs", "Number of input dimensions", dims_, CRP::System, 1, SS_MAX_COORDS));
}

void ANNProjector::configure(Configuration &config)
{
  max_samples_ = config["samples"];
  neighbors_ = config["neighbors"];
  locality_ = config["locality"];
  bucket_size_ = config["bucket_size"];
  error_bound_ = config["error_bound"];
  dims_ = config["inputs"];
  scaling_ = config["scaling"];

  if (scaling_.empty())
    scaling_.resize(dims_, 1.);

  if (scaling_.size() != dims_)
    throw bad_param("projector/sample/ann:scaling");
  
  // Initialize memory
  reset();
}

void ANNProjector::reconfigure(const Configuration &config)
{
  if (config.has("action") && config["action"].str() == "reset")
  {
    DEBUG("Initializing sample store");
  
    WriteGuard guard(rwlock_);
    store_ = StorePtr(new SampleStore());
    indexed_samples_ = 0;
  }
}

void ANNProjector::push(Sample *sample)
{
  WriteGuard guard(rwlock_);
  
  store_->push_back(sample);
}

void ANNProjector::reindex()
{
  // Create new pruned store
  StorePtr newstore = StorePtr(store_->prune(max_samples_));
  
  DEBUG("Building kd-tree with " << newstore->size() << " samples");

  // Build new index using new store
  ANNkd_tree *newindex = new ANNkd_tree((ANNcoord**)newstore->samples(), newstore->size(), dims_, bucket_size_);
  
  {
    WriteGuard guard(rwlock_);

    store_ = newstore;
    safe_delete(&index_);
    index_ = newindex;
    indexed_samples_ = store_->size();
  }
}

ANNProjector *ANNProjector::clone() const
{
  return NULL;
}

ProjectionPtr ANNProjector::project(const Vector &in) const
{
  ReadGuard guard(rwlock_);

  if (in.size() != dims_)
    throw bad_param("projector/sample/ann:dims");
  
  ANNcoord query[SS_MAX_COORDS];
  
  // Scale input dimensions
  for (size_t ii=0; ii < dims_; ++ii)
    query[ii] = scaling_[ii]*in[ii];
    
  size_t index_samples = std::min(neighbors_, indexed_samples_),
         linear_samples = store_->size()-indexed_samples_,
         available_samples = std::min(neighbors_, index_samples+linear_samples);
         
  SampleProjection *projection = new SampleProjection;
  projection->store = store_;
  projection->query = scaling_*in;

  if (available_samples)
  {
    std::vector<SampleRef> refs(index_samples+linear_samples);
      
    if (index_samples)
    {
      // Search store using index
      ANNkd_tree_copy index(*index_);
      ANNidx nn_idx[ANN_MAX_NEIGHBORS];
      ANNdist dd[ANN_MAX_NEIGHBORS];
      
      index.annkSearch(query, index_samples, nn_idx, dd, error_bound_);
      
      for (size_t ii=0; ii < index_samples; ++ii)
      {
        if (nn_idx[ii] == ANN_NULL_IDX)
        {
          // Search failed
          return ProjectionPtr(projection);
        }
      
        refs[ii].index = nn_idx[ii];
        refs[ii].dist = (float)dd[ii];
      }
    }
    else
    {
      // Don't search linear samples when memory is empty, to avoid
      // unnecessary work in the batch case. For the on-line case, it
      // only affects the first sample anyway.
      return ProjectionPtr(projection);
    }

    // Search overflowing samples linearly
    for (size_t ii=0; ii < linear_samples; ++ii)
    {
      double dist=0;
      for (size_t dd=0; dd < dims_; ++dd)
        dist += pow((*store_)[indexed_samples_+ii]->in[dd] - query[dd], 2);
      
      refs[index_samples+ii].index = indexed_samples_+ii;
      refs[index_samples+ii].dist = (float)dist;
    }
    
    std::sort(refs.begin(), refs.end());
    
    // Return ProjectionPtr pointing to current store
    projection->indices.resize(available_samples);
    projection->weights.resize(available_samples);
    
    double hSqr = refs[available_samples-1].dist;
    
    for (size_t ii=0; ii < available_samples; ++ii)
    {
      projection->indices[ii] = refs[ii].index;
      if (hSqr)
        projection->weights[ii] = sqrt(exp(-locality_*refs[ii].dist/hSqr));
      else
        projection->weights[ii] = 1;
    }
  }

  return ProjectionPtr(projection);
}

void ANNProjector::finalize()
{
  reindex();
}
