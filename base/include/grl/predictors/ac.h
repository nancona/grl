/** \file ac.h
 * \brief Actor-critic predictor header file.
 *
 * \author    Wouter Caarls <wouter@caarls.org>
 * \date      2015-02-16
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

#ifndef GRL_AC_PREDICTOR_H_
#define GRL_AC_PREDICTOR_H_

#include <grl/predictor.h>
#include <grl/projector.h>
#include <grl/representation.h>
#include <grl/policy.h>
#include <grl/discretizer.h>
#include <grl/mapping.h>

namespace grl
{

/// Actor-critic predictor for \link ActionPolicy ActionPolicies \endlink.
class ActionACPredictor : public Predictor
{
  public:
    TYPEINFO("predictor/ac/action", "Actor-critic predictor for direct action policies")

  protected:
    Projector *critic_projector_;
    Representation *critic_representation_;
    Trace *critic_trace_;
    
    Projector *actor_projector_;
    Representation *actor_representation_;
    Trace *actor_trace_;

    double alpha_, beta_, gamma_, lambda_;
    
    std::string update_method_;
    Vector step_limit_;
    
    Vector min_, max_;

  public:  
    ActionACPredictor() : critic_projector_(NULL), critic_representation_(NULL), critic_trace_(NULL),
                          actor_projector_(NULL), actor_representation_(NULL), actor_trace_(NULL),
                          alpha_(0.2), beta_(0.01), gamma_(0.97), lambda_(0.65), update_method_("proportional") { }

    // From Configurable
    virtual void request(ConfigurationRequest *config);
    virtual void configure(Configuration &config);
    virtual void reconfigure(const Configuration &config);
    
    // From Predictor
    virtual void update(const Transition &transition);
    virtual void finalize();
};

/// Actor-critic predictor for \link ActionProbabilityPolicy ActionProbabilityPolicies \endlink.
class ProbabilityACPredictor : public Predictor
{
  public:
    TYPEINFO("predictor/ac/probability", "Actor-critic predictor for action-probability policies")
    
  protected:
    Projector *critic_projector_;
    Representation *critic_representation_;
    Trace *critic_trace_;
    
    Projector *actor_projector_;
    Representation *actor_representation_;
    Trace *actor_trace_;
    
    Discretizer* discretizer_;

    double alpha_, beta_, gamma_, lambda_;
  public:
    ProbabilityACPredictor() : critic_projector_(NULL), critic_representation_(NULL), critic_trace_(NULL),
                               actor_projector_(NULL), actor_representation_(NULL), actor_trace_(NULL),
                               discretizer_(NULL), alpha_(0.2), beta_(0.1), gamma_(0.97), lambda_(0.65) { }	
  
    // From Configurable
    virtual void request(ConfigurationRequest *config);
    virtual void configure(Configuration &config);
    virtual void reconfigure(const Configuration &config);
    
    // From Predictor
    virtual void update(const Transition &transition);
    virtual void finalize();
};

/// Q-based actor-critic predictor.
class QACPredictor : public Predictor
{
  public:
    TYPEINFO("predictor/ac/q", "Actor-critic predictor for direct action policies with a Q-value based critic")

  protected:
    Mapping *target_;

    Projector *critic_projector_;
    Representation *critic_representation_;
    Trace *critic_trace_;
    
    Projector *actor_projector_;
    Representation *actor_representation_;
    Trace *actor_trace_;
    
    double alpha_, beta_, gamma_, lambda_, kappa_;
    
    std::string update_method_;
    Vector step_limit_;
    
    Vector min_, max_;

  public:  
    QACPredictor() : target_(NULL), critic_projector_(NULL), critic_representation_(NULL), critic_trace_(NULL),
                     actor_projector_(NULL), actor_representation_(NULL), actor_trace_(NULL),
                     alpha_(0.2), beta_(0.01), gamma_(0.97), lambda_(0.65), kappa_(1.), update_method_("proportional") { }

    // From Configurable
    virtual void request(ConfigurationRequest *config);
    virtual void configure(Configuration &config);
    virtual void reconfigure(const Configuration &config);
    
    // From Predictor
    virtual void update(const Transition &transition);
    virtual void finalize();
};

/// QV-based actor-critic predictor.
class QVACPredictor : public Predictor
{
  public:
    TYPEINFO("predictor/ac/qv", "Actor-critic predictor for direct action policies with a Q critic storing advantages over a V critic")

  protected:
    Projector *critic_q_projector_;
    Representation *critic_q_representation_;
    
    Projector *critic_v_projector_;
    Representation *critic_v_representation_;
    Trace *critic_v_trace_;
    
    Projector *actor_projector_;
    Representation *actor_representation_;
    Trace *actor_trace_;
    
    double alpha_, beta_v_, beta_a_, gamma_, lambda_;
    
    std::string update_method_;
    Vector step_limit_;
    
    Vector min_, max_;

  public:  
    QVACPredictor() : critic_q_projector_(NULL), critic_q_representation_(NULL), 
                      critic_v_projector_(NULL), critic_v_representation_(NULL), critic_v_trace_(NULL),
                      actor_projector_(NULL), actor_representation_(NULL), actor_trace_(NULL),
                      alpha_(0.2), beta_v_(0.05), beta_a_(0.2), gamma_(0.97), lambda_(0.65), update_method_("proportional") { }

    // From Configurable
    virtual void request(ConfigurationRequest *config);
    virtual void configure(Configuration &config);
    virtual void reconfigure(const Configuration &config);
    
    // From Predictor
    virtual void update(const Transition &transition);
    virtual void finalize();
};

}

#endif /* GRL_AC_PREDICTOR_H_ */
