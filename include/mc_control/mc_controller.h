/*
 * Copyright 2015-2019 CNRS-UM LIRMM, CNRS-AIST JRL
 */

#pragma once

#include <mc_control/Configuration.h>
#include <mc_control/generic_gripper.h>

#include <mc_observers/ObserverLoader.h>

#include <mc_rbdyn/Robots.h>

#include <mc_rtc/gui.h>
#include <mc_rtc/log/Logger.h>

#include <mc_solver/CollisionsConstraint.h>
#include <mc_solver/ContactConstraint.h>
#include <mc_solver/DynamicsConstraint.h>
#include <mc_solver/KinematicsConstraint.h>
#include <mc_solver/QPSolver.h>
#include <mc_solver/msg/QPResult.h>

#include <mc_tasks/PostureTask.h>

namespace mc_rbdyn
{
struct Contact;
}

#include <mc_control/api.h>

namespace mc_control
{

/** \class ControllerResetData
 * \brief Contains information allowing the controller to start smoothly from
 * the current state of the robot
 * \note
 * For now, this only contains the state of the robot (free flyer and joints state)
 */
struct MC_CONTROL_DLLAPI ControllerResetData
{
  /** Contains free flyer + joints state information */
  const std::vector<std::vector<double>> & q;
};

struct MCGlobalController;

/** \class MCController
 * \brief MCController is the base class to implement all controllers. It
 * assumes that at least two robots are provided. The first is considered as the
 * "main" robot. Some common constraints and a posture task are defined (but not
 * added to the solver) for this robot
 */
struct MC_CONTROL_DLLAPI MCController
{
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  friend struct MCGlobalController;

public:
  virtual ~MCController();
  /** This function is called at each time step of the process driving the robot
   * (i.e. simulation or robot's controller). This function is the most likely
   * to be overriden for complex controller behaviours.
   * \return True if the solver succeeded, false otherwise
   * \note
   * This is meant to run in real-time hence some precaution should apply (e.g.
   * no i/o blocking calls, no thread instantiation and such)
   *
   * \note
   * The default implementation does the bare minimum (i.e. call run on QPSolver)
   * It is recommended to use it in your override.
   */
  virtual bool run();

  /** This function is called before the run() function at each time step of the process
   * driving the robot (i.e. simulation or robot's controller). The default
   * behaviour is to call the run() function of each loaded observer and update
   * the realRobot instance when desired.
   *
   * The default behaviour is determined by the following configuration entries:
   * - "EnabledObservers": ["Encoder", "BodySensor", "KinematicInertial"],
   * - "UpdateObservers": ["Encoder", "KinematicInertial"],
   *
   * This is meant to run in real-time hence some precaution should apply (e.g.
   * no i/o blocking calls, no thread instantiation and such)
   *
   * \note Some estimators are likely to require extra information. For this, each observer
   * has const access to the MCController instance, and can thus access all const information
   * available from it. The default estimators provided by mc_rtc (currently)
   * rely on robots() and realRobots() information. Additionally, the
   * KinematicInertialObserver requires an anchor frame with the environement.
   * This is to be provided by overriding the anchorFrame() method.
   *
   * \note If the default pipeline behaviour does not suit you, you may override
   * this method.
   *
   * @returns true if all observers ran as expected, false otherwise
   */
  virtual bool runObservers();

  /*! @brief Reset the observers. This function is called after the reset()
   * function.
   *
   * \see runObservers()
   *
   * @returns True when all observers have been succesfully reset.
   */
  virtual bool resetObservers();

  /*! @brief Returns a kinematic anchor frame.
   *  This is typically used by state observers such as mc_observers::KinematicInertialObserver to obtain a reference
   * frame for the estimation. In the case of a biped robot, this is typically a frame in-between the feet of the robot.
   * See the specific requirements for the active observers in your controller.
   *
   * @returns An anchor frame in-between the feet.
   *
   * @throws std::runtime_error Default implemetation throws. Please override this function in your controller when
   * required.
   */
  virtual sva::PTransformd anchorFrame(const mc_rbdyn::Robot & robot) const;

  /**
   * WARNING EXPERIMENTAL
   * Runs the QP on real_robot state
   * ONLY SUPPORTS ONE ROBOT FOR NOW
   */
  virtual bool runClosedLoop();

  /** Can be called in derived class instead of run to use a feedback strategy
   * different from the default one
   *
   * \param fType Type of feedback used in the solver
   *
   */
  bool run(mc_solver::FeedbackType fType);

  /** This function is called when the controller is stopped.
   *
   * The default implementation does nothing.
   *
   * For example, it can be overriden to signal threads launched by the
   * controller to pause.
   *
   */
  virtual void stop();

  /** Gives access to the result of the QP execution
   * \param t Unused at the moment
   */
  virtual const mc_solver::QPResultMsg & send(const double & t);

  /** Reset the controller with data provided by ControllerResetData. This is
   * called at two possible points during a simulation/live execution:
   *   1. Actual start
   *   2. Switch from a previous (MCController-like) controller
   * In the first case, the data comes from the simulation/controller. In the
   * second case, the data comes from the previous MCController instance.
   * \param reset_data Contains information allowing to reset the controller
   * properly
   * \note
   * The default implementation reset the main robot's state to that provided by
   * reset_data (with a null speed/acceleration). It maintains the contacts as
   * they were set by the controller previously.
   */
  virtual void reset(const ControllerResetData & reset_data);

  /** Return the main robot (first robot provided in the constructor)
   * \anchor mc_controller_robot_const_doc
   */
  const mc_rbdyn::Robot & robot() const;

  /** Return the env "robot"
   * \note
   * In multi-robot scenarios, the env robot is either:
   *   1. The first robot with zero dof
   *   2. The last robot provided at construction
   * \anchor mc_controller_env_const_doc
   */
  const mc_rbdyn::Robot & env() const;

  /** Return the mc_rbdyn::Robots controlled by this controller
   * \anchor mc_controller_robots_const_doc
   */
  const mc_rbdyn::Robots & robots() const;

  /** Non-const variant of \ref mc_controller_robots_const_doc "robots()" */
  mc_rbdyn::Robots & robots();

  /** Non-const variant of \ref mc_controller_robot_const_doc "robot()" */
  mc_rbdyn::Robot & robot();

  /** Non-const variant of \ref mc_controller_env_const_doc "env()" */
  mc_rbdyn::Robot & env();

  /** Return the mc_solver::QPSolver instance attached to this controller
   * \anchor mc_controller_qpsolver_const_doc
   */
  const mc_solver::QPSolver & solver() const;

  /** Non-const variant of \ref mc_controller_qpsolver_const_doc "solver()" */
  mc_solver::QPSolver & solver();

  /** Returns mc_rtc::Logger instance */
  mc_rtc::Logger & logger();

  /** Returns mc_rtc::gui::StateBuilder ptr */
  std::shared_ptr<mc_rtc::gui::StateBuilder> gui()
  {
    return gui_;
  }

  /** Return the mc_rbdyn::Robots real robots instance
   * \anchor mc_controller_real_robots_const_doc
   */
  const mc_rbdyn::Robots & realRobots() const;
  /** Non-const variant of \ref mc_controller_real_robots_const_doc "realRobots()" **/
  mc_rbdyn::Robots & realRobots();

  /** Return the main mc_rbdyn::Robot real robot instance
   * \anchor mc_controller_real_robot_const_doc
   */
  const mc_rbdyn::Robot & realRobot() const;
  /** Non-const variant of \ref mc_controller_real_robot_const_doc "realRobot()" */
  mc_rbdyn::Robot & realRobot();

  /** Returns a list of robots supported by the controller.
   * \return Vector of supported robots designed by name (as returned by
   * RobotModule::name())
   * \note
   * Default implementation returns an empty list which indicates that all
   * robots are supported.
   */
  virtual std::vector<std::string> supported_robots() const;

protected:
  /** Builds a controller base with an empty environment
   * \param robot Pointer to the main RobotModule
   * \param dt Controller timestep
   * your controller
   */
  MCController(std::shared_ptr<mc_rbdyn::RobotModule> robot, double dt);

  /** Builds a multi-robot controller base
   * \param robots Collection of robot modules used by the controller
   * \param dt Timestep of the controller
   */
  MCController(const std::vector<std::shared_ptr<mc_rbdyn::RobotModule>> & robot_modules, double dt);

protected:
  /** Load an additional robot into the controller
   *
   * \param rm RobotModule used to load the robot
   *
   * \param name Name of the robot
   *
   * \returns The loaded robot
   */
  mc_rbdyn::Robot & loadRobot(mc_rbdyn::RobotModulePtr rm, const std::string & name);

protected:
  /** QP solver */
  std::shared_ptr<mc_solver::QPSolver> qpsolver;
  /** Real robots provided by MCGlobalController, nullptr until ::reset */
  std::shared_ptr<mc_rbdyn::Robots> real_robots;

  /** Observers order provided by MCGlobalController
   * Observers will be run and update real robot in that order
   **/
  std::vector<mc_observers::ObserverPtr> observers_;
  /** Observers that will be run by the pipeline.
   *
   * The pair contains:
   * - The observer to run
   * - A boolean set to true if the observer updates the real robot instance
   *
   * Provided by MCGlobalController */
  std::vector<std::pair<mc_observers::ObserverPtr, bool>> pipelineObservers_;
  /** Logger provided by MCGlobalController */
  std::shared_ptr<mc_rtc::Logger> logger_;
  /** GUI state builder */
  std::shared_ptr<mc_rtc::gui::StateBuilder> gui_;

public:
  /** Controller timestep */
  const double timeStep;
  /** Grippers */
  std::map<std::string, std::shared_ptr<mc_control::Gripper>> grippers;
  /** Contact constraint for the main robot */
  mc_solver::ContactConstraint contactConstraint;
  /** Dynamics constraints for the main robot */
  mc_solver::DynamicsConstraint dynamicsConstraint;
  /** Kinematics constraints for the main robot */
  mc_solver::KinematicsConstraint kinematicsConstraint;
  /** Self collisions constraint for the main robot */
  mc_solver::CollisionsConstraint selfCollisionConstraint;
  /** Posture task for the main robot */
  std::shared_ptr<mc_tasks::PostureTask> postureTask;
};

} // namespace mc_control

#ifndef MC_RTC_NO_INCLUDE_VERSION
#  include <mc_rtc/version.h>
#endif

#ifdef WIN32
#  define CONTROLLER_MODULE_API __declspec(dllexport)
#else
#  if __GNUC__ >= 4
#    define CONTROLLER_MODULE_API __attribute__((visibility("default")))
#  else
#    define CONTROLLER_MODULE_API
#  endif
#endif

/** A simple compile-time versus run-time version checking macro
 *
 * If you are not relying on CONTROLLER_CONSTRUCTOR or
 * SIMPLE_CONTROLLER_CONSTRUCTOR you should use this in your MC_RTC_CONTROLLER
 * implementation
 *
 */
#define CONTROLLER_CHECK_VERSION(NAME)                                                                            \
  if(mc_rtc::MC_RTC_VERSION != mc_rtc::version())                                                                 \
  {                                                                                                               \
    LOG_ERROR(NAME << " was compiled with " << mc_rtc::MC_RTC_VERSION << " but mc_rtc is currently at version "   \
                   << mc_rtc::version() << ", you might experience subtle issues and should recompile your code") \
  }

/** Provides a handle to construct the controller with Json config */
#define CONTROLLER_CONSTRUCTOR(NAME, TYPE)                                                                        \
  extern "C"                                                                                                      \
  {                                                                                                               \
    CONTROLLER_MODULE_API void MC_RTC_CONTROLLER(std::vector<std::string> & names)                                \
    {                                                                                                             \
      CONTROLLER_CHECK_VERSION(NAME)                                                                              \
      names = {NAME};                                                                                             \
    }                                                                                                             \
    CONTROLLER_MODULE_API void destroy(mc_control::MCController * ptr)                                            \
    {                                                                                                             \
      delete ptr;                                                                                                 \
    }                                                                                                             \
    CONTROLLER_MODULE_API mc_control::MCController * create(const std::string &,                                  \
                                                            const std::shared_ptr<mc_rbdyn::RobotModule> & robot, \
                                                            const double & dt,                                    \
                                                            const mc_control::Configuration & conf)               \
    {                                                                                                             \
      return new TYPE(robot, dt, conf);                                                                           \
    }                                                                                                             \
  }

/** Provides a handle to construct a generic controller */
#define SIMPLE_CONTROLLER_CONSTRUCTOR(NAME, TYPE)                                                                 \
  extern "C"                                                                                                      \
  {                                                                                                               \
    CONTROLLER_MODULE_API void MC_RTC_CONTROLLER(std::vector<std::string> & names)                                \
    {                                                                                                             \
      CONTROLLER_CHECK_VERSION(NAME)                                                                              \
      names = {NAME};                                                                                             \
    }                                                                                                             \
    CONTROLLER_MODULE_API void destroy(mc_control::MCController * ptr)                                            \
    {                                                                                                             \
      delete ptr;                                                                                                 \
    }                                                                                                             \
    CONTROLLER_MODULE_API mc_control::MCController * create(const std::string &,                                  \
                                                            const std::shared_ptr<mc_rbdyn::RobotModule> & robot, \
                                                            const double & dt,                                    \
                                                            const mc_control::Configuration &)                    \
    {                                                                                                             \
      return new TYPE(robot, dt);                                                                                 \
    }                                                                                                             \
  }
