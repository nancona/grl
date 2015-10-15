/** \file nmpc.h
 * \brief NMPC policy header file.
 *
 * \author    Wouter Caarls <wouter@caarls.org>
 * \date      2015-05-08
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

#ifndef GRL_NMPC_POLICY_H_
#define GRL_NMPC_POLICY_H_

#include <grl/policy.h>
#include <grl/policies/common_code.h> // MUSCOD-II thread-safe data structure

class MUSCOD;

namespace grl
{

/// NMPC policy
class NMPCPolicy : public Policy
{
  public:
    TYPEINFO("policy/nmpc", "Nonlinear model predictive control policy using the MUSCOD library")

  protected:
    int verbose_;

    // MUSCOD-II interface
    void *so_handle_; // hangle to a shared library with problem definitions
    void (*so_convert_obs_for_muscod)(const std::vector<double> *from, std::vector<double> *to);
    MuscodData data_;
    MUSCOD *muscod_;
    std::string model_name_, lua_model_;
    size_t outputs_;
    Vector pf_;

  public:
    NMPCPolicy() : muscod_(NULL), outputs_(1), verbose_(false) { }
    ~NMPCPolicy();

    // From Configurable
    virtual void request(ConfigurationRequest *config);
    virtual void configure(Configuration &config);
    virtual void reconfigure(const Configuration &config);
    virtual void muscod_reset(Vector &initial_obs, double time);

    // From Policy
    virtual NMPCPolicy *clone() const;
    virtual void act(double time, const Vector &in, Vector *out);
};

}

#endif /* GRL_NMPC_POLICY_H_ */