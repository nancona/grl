#include <XMLConfiguration.h>

#include <grl/environments/leo/leo.h>
#include <ctime>
#include <ratio>
#include <chrono>

using namespace grl;

REGISTER_CONFIGURABLE(LeoEnvironment)

void CGrlLeoBhWalkSym::resetState()
{
  mIsObserving          = false;
  mLastRewardedFoot     = lpFootLeft;
  mLastStancelegWasLeft = -1;
  mFootstepLength       = 0.0;
  mLastFootstepLength   = 0.0;
  mNumFootsteps         = 0;
  mWalkedDistance       = 0.0;
  mTrialEnergy          = 0.0;

  // Reset velocity filters to zero velocity (this is the result of robot->setIC)
  for (int iJoint=0; iJoint<ljNumJoints; iJoint++)
    mJointSpeedFilter[iJoint].clear();

  for (int i=0; i<ljNumDynamixels; i++)
    mJointSpeedFilter[i].init(1.0/mTotalStepTime, 10.0);
  mJointSpeedFilter[ljTorso].init(mTotalStepTime, 25.0);	// 25Hz because? : 1) this encoder has 8x the resolution of a dynamixel 2) torso angles/velocities are more important
}

void CGrlLeoBhWalkSym::fillLeoState(const Vector &obs, const Vector &action, CLeoState &leoState)
{
  // '-' required to match with Erik's code, but does not matter for learning.
  // Erik used a rotation matrix which was rotating a unit vector. For torso it seems
  // the positive direction was not same as for other joints, internally defined in ODE.
  leoState.mJointAngles[ljTorso]      = -obs[svTorsoAngle];
  leoState.mJointSpeeds[ljTorso]      = -mJointSpeedFilter[ljTorso].filter(obs[svTorsoAngleRate]);
  leoState.mJointAngles[ljShoulder]   = obs[svLeftArmAngle];
  leoState.mJointSpeeds[ljShoulder]   = mJointSpeedFilter[ljShoulder].filter(obs[svLeftArmAngleRate]);
  leoState.mJointAngles[ljHipRight]   = obs[svRightHipAngle];
  leoState.mJointSpeeds[ljHipRight]   = mJointSpeedFilter[ljHipRight].filter(obs[svRightHipAngleRate]);
  leoState.mJointAngles[ljHipLeft]    = obs[svLeftHipAngle];
  leoState.mJointSpeeds[ljHipLeft]    = mJointSpeedFilter[ljHipLeft].filter(obs[svLeftHipAngleRate]);
  leoState.mJointAngles[ljKneeRight]  = obs[svRightKneeAngle];
  leoState.mJointSpeeds[ljKneeRight]	= mJointSpeedFilter[ljKneeRight].filter(obs[svRightKneeAngleRate]);
  leoState.mJointAngles[ljKneeLeft]   = obs[svLeftKneeAngle];
  leoState.mJointSpeeds[ljKneeLeft]   = mJointSpeedFilter[ljKneeLeft].filter(obs[svLeftKneeAngleRate]);
  leoState.mJointAngles[ljAnkleRight] = obs[svRightAnkleAngle];
  leoState.mJointSpeeds[ljAnkleRight] = mJointSpeedFilter[ljAnkleRight].filter(obs[svRightAnkleAngleRate]);
  leoState.mJointAngles[ljAnkleLeft]  = obs[svLeftAnkleAngle];
  leoState.mJointSpeeds[ljAnkleLeft]	= mJointSpeedFilter[ljAnkleLeft].filter(obs[svLeftAnkleAngleRate]);

  leoState.mFootContacts  = obs[svRightToeContact]?LEO_FOOTSENSOR_RIGHT_TOE:0;
  leoState.mFootContacts |= obs[svRightHeelContact]?LEO_FOOTSENSOR_RIGHT_HEEL:0;
  leoState.mFootContacts |= obs[svLeftToeContact]?LEO_FOOTSENSOR_LEFT_TOE:0;
  leoState.mFootContacts |= obs[svLeftHeelContact]?LEO_FOOTSENSOR_LEFT_HEEL:0;

  // required for the correct energy calculation in the reward function
  if (action.size())
  {
    leoState.mActuationVoltages[ljShoulder]   = action[avLeftArmTorque];
    leoState.mActuationVoltages[ljHipRight]   = action[avRightHipTorque];
    leoState.mActuationVoltages[ljHipLeft]    = action[avLeftHipTorque];
    leoState.mActuationVoltages[ljKneeRight]  = action[avRightKneeTorque];
    leoState.mActuationVoltages[ljKneeLeft]   = action[avLeftKneeTorque];
    leoState.mActuationVoltages[ljAnkleRight] = action[avRightAnkleTorque];
    leoState.mActuationVoltages[ljAnkleLeft]  = action[avLeftAnkleTorque];
  }
}

void CGrlLeoBhWalkSym::parseLeoState(const CLeoState &leoState, Vector &obs)
{
  obs[siTorsoAngle]           = leoState.mJointAngles[ljTorso];
  obs[siTorsoAngleRate]       = leoState.mJointSpeeds[ljTorso];
  obs[siHipStanceAngle]       = leoState.mJointAngles[mHipStance];
  obs[siHipStanceAngleRate]   = leoState.mJointSpeeds[mHipStance];
  obs[siHipSwingAngle]        = leoState.mJointAngles[mHipSwing];
  obs[siHipSwingAngleRate]    = leoState.mJointSpeeds[mHipSwing];
  obs[siKneeStanceAngle]      = leoState.mJointAngles[mKneeStance];
  obs[siKneeStanceAngleRate]  = leoState.mJointSpeeds[mKneeStance];
  obs[siKneeSwingAngle]       = leoState.mJointAngles[mKneeSwing];
  obs[siKneeSwingAngleRate]   = leoState.mJointSpeeds[mKneeSwing];
}

void CGrlLeoBhWalkSym::setCurrentSTGState(CLeoState *leoState)
{
  mCurrentSTGState = leoState;
}

void CGrlLeoBhWalkSym::setPreviousSTGState(CLeoState *leoState)
{
  mPreviousSTGState = *leoState;
}

void CGrlLeoBhWalkSym::updateDerivedStateVars(CLeoState* currentSTGState)
{
  CLeoBhWalkSym::updateDerivedStateVars(currentSTGState);
}

/////////////////////////////////

LeoEnvironment::LeoEnvironment() :
  bhWalk_(&leoSim_),
  observation_dims_(CGrlLeoBhWalkSym::svNumStates),
  requested_action_dims_(CGrlLeoBhWalkSym::svNumActions),
  learn_stance_knee_(0),
  time_test_(0),
  time_learn_(0),
  time0_(0),
  test_(0),
  tau_(1.0/30),
  exporter_(NULL)
{
}

void LeoEnvironment::request(ConfigurationRequest *config)
{
  ODEEnvironment::request(config);

  config->push_back(CRP("observe", "string.observe_", "Comma-separated list of state elements observed by an agent"));
  config->push_back(CRP("actuate", "string.actuate_", "Comma-separated list of action elements provided by an agent"));
  config->push_back(CRP("learn_stance_knee", "Learn stance knee", learn_stance_knee_, CRP::Configuration, 0, 1));
  config->push_back(CRP("exporter", "exporter", "Optional exporter for transition log (supports time, state, observation, action, reward, terminal)", exporter_, true));
}

void LeoEnvironment::configure(Configuration &config)
{
  // Setup path to a configuration file
  std::string xml = std::string(LEO_CONFIG_DIR) + "/" + config["xml"].str();
  config.set("xml", xml);

  // Read yaml first. Settings will be overwritten by ODEEnvironment::configure,
  // which are different because they belong to ODE simulator.
  ODEEnvironment::configure(config);
  ode_observation_dims_ = config["observation_dims"];
  ode_action_dims_ = config["action_dims"];
  learn_stance_knee_ = config["learn_stance_knee"];

  exporter_ = (Exporter*) config["exporter"].ptr();
  if (exporter_)
    exporter_->init({"time", "state0", "state1", "action", "reward", "terminal"});

  // Process configuration for leosim
  CXMLConfiguration xmlConfig;
  if (!xmlConfig.loadFile(xml))
  {
    ERROR("Couldn't load XML configuration file \"" << xml << "\"!\nPlease check that the file exists and that it is sound (error: " << xmlConfig.errorStr() << ").");
    return;
  }

  // Resolve expressions
  xmlConfig.resolveExpressions();
  
  // Read rewards and preprogrammed angles
  bhWalk_.readConfig(xmlConfig.root());

  // Read desired control frequency
  double desiredFrequency;
  CConfigSection configNode = xmlConfig.root().section("policy");
  configNode.get("frequency", &desiredFrequency);
  tau_ = 1/desiredFrequency;
  INFO("Time step: " << tau_);

  // Parse observations
  std::string observe = config["observe"].str();
  const std::vector<std::string> observeList = cutLongStr(observe);
  fillObserve(env_->getSensors(), observeList, observe_);
  if (observe_.size() != ode_observation_dims_)
    throw bad_param("leo/walk:observe");
  observation_dims_ = (observe_.array() != 0).count();
  
  // mask observation min/max vectors
  Vector ode_observation_min, ode_observation_max, observation_min, observation_max;
  config.get("observation_min", ode_observation_min);
  config.get("observation_max", ode_observation_max);
  observation_min.resize(observation_dims_);
  observation_max.resize(observation_dims_);
  for (int i = 0, j = 0; i < observe_.size(); i++)
    if (observe_[i])
    {
      observation_min[j]   = ode_observation_min[i];
      observation_max[j++] = ode_observation_max[i];
    }
  config.set("observation_dims", observation_dims_);
  config.set("observation_min", observation_min);
  config.set("observation_max", observation_max);

  // Parse actions
  std::vector<int> knee_idx;
  int omit_knee_idx = -1;
  std::string actuate = config["actuate"].str();
  std::vector<std::string> actuateList = cutLongStr(actuate);
  fillActuate(env_->getActuators(), actuateList, actuate_, knee_idx);
  if (actuate_.size() != ode_action_dims_)
    throw bad_param("leo/walk:actuate_");
  requested_action_dims_ = (actuate_.array() != 0).count();
  if (learn_stance_knee_)
    action_dims_ = requested_action_dims_;
  else
  {
    if (knee_idx.size() != 2)
      throw bad_param("leo/walk:actuate_ (if any of knees is learnt, then always include both knees)");
    action_dims_ = requested_action_dims_ - 1;
    omit_knee_idx = knee_idx[1];
  }

  // mask observation min/max vectors
  Vector ode_action_min, ode_action_max, action_min, action_max;
  config.get("action_min", ode_action_min);
  config.get("action_max", ode_action_max);
  action_min.resize(action_dims_);
  action_max.resize(action_dims_);
  for (int i = 0, j = 0; i < actuate_.size(); i++)
    if (actuate_[i] && i != omit_knee_idx)
    {
      action_min[j]   = ode_action_min[i];
      action_max[j++] = ode_action_max[i];
    }

  config.set("action_dims", action_dims_);
  config.set("action_min", action_min);
  config.set("action_max", action_max);

  // reserve memory
  ode_obs_.resize(ode_observation_dims_);
  ode_action_.resize(ode_action_dims_);

  // Zeromq
  zmq_.init("tcp://*:5556", "tcp://localhost:5555");
}

void LeoEnvironment::reconfigure(const Configuration &config)
{
  ODEEnvironment::reconfigure(config);
  time_test_ = time_learn_ = time0_ = 0;
}

LeoEnvironment *LeoEnvironment::clone()
{
  return new LeoEnvironment(*this);
}

void LeoEnvironment::start(int test, Vector *obs)
{
  test_ = test;

  // TODO: obtain current state of Leo
  //ODEEnvironment::start(test, &ode_obs_);

  // Experiment
  ///////////////////////////////////////
//  const int size = 20;
//  double data_tr[size];
//  double data_rc[size];
//  std::cout << sizeof(data_tr) << std::endl;
/*
  char ch[] = "ABC";
  std::cout << ch << " size is: " << sizeof(ch) << std::endl;

  while (1)
  {
    auto begin = std::chrono::high_resolution_clock::now();

//    for (int i = 0; i < size; i++)
//      data_tr[i] = rand();

    zmq_.send(reinterpret_cast<void*>(ch), sizeof(ch));
    //
    //zmq_.recv(reinterpret_cast<void*>(data_rc), sizeof(data_rc));

    //for (int i = 0; i < size; i++)
    //  if (data_tr[i] != data_rc[i])
    //    std::cout << "Data is incorrect" << std::endl;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    std::cout << duration << "ns delay" << std::endl;
  }
*/
  ///////////////////////////////////////

  bhWalk_.resetState();

  // Parse obs into CLeoState (Start with left leg being the stance leg)
  bhWalk_.fillLeoState(ode_obs_, Vector(), leoState_);
  bhWalk_.setCurrentSTGState(&leoState_);
  bhWalk_.setPreviousSTGState(&leoState_);

  // update derived state variables
  bhWalk_.updateDerivedStateVars(&leoState_); // swing-stance switching happens here

  // construct new obs from CLeoState
  obs->resize(observation_dims_);
  bhWalk_.parseLeoState(leoState_, *obs);

  bhWalk_.setCurrentSTGState(NULL);

  if (exporter_)
    exporter_->open((test_?"test":"learn"), (test_?time_test_:time_learn_) != 0.0);
  time0_ = test_?time_test_:time_learn_;
}

double LeoEnvironment::step(const Vector &action, Vector *obs, double *reward, int *terminal)
{
  // Obtain state of the Leo
  int size = 4;
  char data[size];
  zmq_.recv(reinterpret_cast<void*>(data), sizeof(data), ZMQ_DONTWAIT);
  std::cout << data << std::endl;

  sleep(1);

  double &time = test_?time_test_:time_learn_;
  bhWalk_.setCurrentSTGState(&leoState_);

  // auto actuate unlearned joints to find complete action vector
  double actionArm, actionStanceKnee, actionSwingKnee, actionStanceHip, actionSwingHip;
  actionStanceHip = action[0];
  actionSwingHip  = action[1];
  if (!learn_stance_knee_)
  {
    // Auto actuation of the stance knee
    actionStanceKnee = bhWalk_.grlAutoActuateKnee();
    actionSwingKnee  = action[2];
  }
  else
  {
    // Learn both actions
    actionStanceKnee = action[2];
    actionSwingKnee  = action[3];
  }
  Vector actionAnkles;
  bhWalk_.grlAutoActuateAnkles(actionAnkles);
  actionArm = bhWalk_.grlAutoActuateArm();

  // concatenation happens in the order of <actionvar> definitions in an xml file
  // shoulder, right hip, left hip, right knee, left knee, right ankle, left ankle
  if (bhWalk_.stanceLegLeft())
    ode_action_ << actionArm, actionSwingHip, actionStanceHip, actionSwingKnee, actionStanceKnee, actionAnkles;
  else
    ode_action_ << actionArm, actionStanceHip, actionSwingHip, actionStanceKnee, actionSwingKnee, actionAnkles;

  //ode_action_ << ConstantVector(7, 5.0); // #ivan

  bhWalk_.setPreviousSTGState(&leoState_);

  // TODO: apply control to Leo
  //double tau = ODEEnvironment::step(ode_action_, &ode_obs_, reward, terminal);

  // Filter joint speeds
  // Parse obs into CLeoState
  bhWalk_.fillLeoState(ode_obs_, ode_action_, leoState_);
  bhWalk_.setCurrentSTGState(&leoState_);

  // update derived state variables
  bhWalk_.updateDerivedStateVars(&leoState_);

  // construct new obs from CLeoState
  bhWalk_.parseLeoState(leoState_, *obs);

  // Determine reward
  *reward = bhWalk_.calculateReward();

  // ... and termination
  if (*terminal == 1) // timeout
    *terminal = 1;
  else if (bhWalk_.isDoomedToFall(&leoState_, false))
    *terminal = 2;
  else
    *terminal = 0;

  // Export & debug
  std::vector<double> s1(leoState_.mJointAngles, leoState_.mJointAngles + ljNumJoints);
  std::vector<double> v1(leoState_.mJointSpeeds, leoState_.mJointSpeeds + ljNumJoints);
  std::vector<double> a(leoState_.mActuationVoltages, leoState_.mActuationVoltages + ljNumDynamixels);

  if (exporter_)
  {
    std::vector<double> s0(bhWalk_.getPreviousSTGState()->mJointAngles, bhWalk_.getPreviousSTGState()->mJointAngles + ljNumJoints);
    std::vector<double> v0(bhWalk_.getPreviousSTGState()->mJointSpeeds, bhWalk_.getPreviousSTGState()->mJointSpeeds + ljNumJoints);
    s0.insert(s0.end(), v0.begin(), v0.end());
    s1.insert(s1.end(), v1.begin(), v1.end());
    
    Vector s0v, s1v, av;
    toVector(s0, s0v);
    toVector(s1, s1v);
    toVector(a, av);

    exporter_->write({grl::VectorConstructor(time), s0v,  s1v,
                      av, grl::VectorConstructor(*reward), grl::VectorConstructor(*terminal)
                     });
  }

  TRACE("State angles: " << s1);
  TRACE("State velocities: " << v1);
  TRACE("Contacts: " << (int)leoState_.mFootContacts);
  TRACE("RL action: " << action);
  TRACE("Full action: " << a);
  TRACE("Reward: " << *reward);

  time += tau_;
  return tau_;
}

void LeoEnvironment::report(std::ostream &os)
{
  double &time  = test_?time_test_ :time_learn_;
  os << bhWalk_.getProgressReport(time-time0_);
}

void LeoEnvironment::fillObserve( const std::vector<CGenericStateVar> &genericStates,
                                     const std::vector<std::string> &observeList,
                                     Vector &out) const
{
  out.resize(genericStates.size());
  for (int i = 0; i < out.size(); i++) out[i] = 0;
  std::vector<std::string>::const_iterator listMember = observeList.begin();
  std::vector<CGenericStateVar>::const_iterator gState;
  std::string::const_iterator it;

  for (; listMember < observeList.end(); listMember++)
  {
    bool found = false;
    gState = genericStates.begin();
    for (int i = 0; gState < genericStates.end(); gState++, i++)
    {
      const std::string &name = gState->name();
      it = std::search(name.begin(), name.end(), listMember->begin(), listMember->end());

      if (it != name.end())
      {
        it += listMember->size(); // point at the end of substring
        if (it == name.end() || *it == '.')
        {
          INFO("Adding to the observation vector: " << name);
          out[i] = 1;
          found = true;
        }
      }
    }

    if (!found)
    {
      ERROR("Requested unregistered field '" << *listMember << "'");
      throw bad_param("leo:observe");
    }
  }
}

void LeoEnvironment::fillActuate(const std::vector<CGenericActionVar> &genericAction,
                                     const std::vector<std::string> &actuateList,
                                     Vector &out, std::vector<int> &knee_idx) const
{
  out.resize(genericAction.size());
  for (int i = 0; i < out.size(); i++) out[i] = 0;
  std::vector<std::string>::const_iterator listMember = actuateList.begin();
  std::vector<CGenericActionVar>::const_iterator gAction;
  std::string::const_iterator it;
  std::string knee_str = "knee";

  for (; listMember < actuateList.end(); listMember++)
  {
    bool found = false;
    gAction = genericAction.begin();
    for (int i = 0; gAction < genericAction.end(); gAction++, i++)
    {
      const std::string &name = gAction->name();
      it = std::search(name.begin(), name.end(), listMember->begin(), listMember->end());

      if (it != name.end())
      {
        it += listMember->size(); // point at the end of substring
        if (it == name.end() || *it == '.')
        {
          INFO("Adding to the actuation vector: " << name);
          out[i] = 1;
          if (std::search(listMember->begin(), listMember->end(), knee_str.begin(), knee_str.end()) != listMember->end())
            knee_idx.push_back(i);
          found = true;
        }
      }
    }

    if (!found)
    {
      ERROR("Requested unregistered field '" << *listMember << "'");
      throw bad_param("leo:actuate");
    }
  }
}