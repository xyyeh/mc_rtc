/*
 * Copyright 2015-2019 CNRS-UM LIRMM, CNRS-AIST JRL
 */

#include <mc_control/fsm/Controller.h>
#include <mc_control/fsm/states/Parallel.h>

namespace mc_control
{

namespace fsm
{

ParallelState::DelayedState::DelayedState(Controller & ctl,
                                          const std::string & name,
                                          double delay,
                                          mc_rtc::Configuration config)
: state_(nullptr), name_(name), config_(config), delay_(delay)
{
  if(delay == 0)
  {
    createState(ctl);
  }
}

bool ParallelState::DelayedState::run(Controller & ctl, double time)
{
  if(state_)
  {
    return state_->run(ctl);
  }
  if(time > delay_)
  {
    createState(ctl);
  }
  return false;
}

StatePtr & ParallelState::DelayedState::state()
{
  return state_;
}

void ParallelState::DelayedState::createState(Controller & ctl)
{
  state_ = ctl.factory().create(name_, ctl, config_);
}

void ParallelState::configure(const mc_rtc::Configuration & config)
{
  config_.load(config);
}

void ParallelState::start(Controller & ctl)
{
  std::vector<std::string> states = config_("states");
  if(states.size() == 0)
  {
    LOG_ERROR_AND_THROW(std::runtime_error, "ParallelState requires at least one state to run")
  }
  for(const auto & s : states)
  {
    if(!ctl.factory().hasState(s))
    {
      LOG_ERROR_AND_THROW(std::runtime_error, name() + ": " + s + " is not available")
    }
  }
  auto states_config = config_("configs", mc_rtc::Configuration{});
  auto delays = config_("delays", mc_rtc::Configuration{});
  for(auto & s : states)
  {
    double delay = delays(s, 0.0);
    auto cfg = states_config(s, mc_rtc::Configuration{});
    states_.emplace_back(ctl, s, delay, cfg);
  }
}

bool ParallelState::run(Controller & ctl)
{
  bool ret = true;
  for(auto & s : states_)
  {
    ret = s.run(ctl, time_) && ret;
  }
  time_ += ctl.solver().dt();
  if(ret)
  {
    output(states_.back().state()->output());
  }
  return ret;
}

void ParallelState::stop(Controller & ctl)
{
  for(auto & s : states_)
  {
    if(s.state())
    {
      s.state()->stop(ctl);
    }
  }
}

void ParallelState::teardown(Controller & ctl)
{
  for(auto & s : states_)
  {
    if(s.state())
    {
      s.state()->teardown_(ctl);
    }
  }
}

bool ParallelState::read_msg(std::string & msg)
{
  for(auto & s : states_)
  {
    if(s.state() && s.state()->read_msg(msg))
    {
      return true;
    }
  }
  return false;
}

bool ParallelState::read_write_msg(std::string & msg, std::string & out)
{
  for(auto & s : states_)
  {
    if(s.state() && s.state()->read_write_msg(msg, out))
    {
      return true;
    }
  }
  return false;
}

} // namespace fsm

} // namespace mc_control

EXPORT_SINGLE_STATE("Parallel", mc_control::fsm::ParallelState)
