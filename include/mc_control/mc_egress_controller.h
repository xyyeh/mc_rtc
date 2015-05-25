#ifndef _H_MCEGRESSCONTROLLER_H_
#define _H_MCEGRESSCONTROLLER_H_

#include <mc_control/mc_controller.h>

#include <mc_tasks/CoMTask.h>
#include <mc_tasks/EndEffectorTask.h>

namespace mc_control
{

struct MCEgressController : public MCController
{
public:
  enum EgressPhase
  {
    MOVEFOOTINSIDE = 1,
    LIFTBODY,
    ROTATEBODY,
    PUTDOWNBODY,
    FREEHAND,
    SAFEHAND,
    MOVEFOOTOUTSIDE
  };
public:
  MCEgressController(const std::string & env_path, const std::string & env_name);

  virtual void reset(const ControllerResetData & reset_data) override;

  virtual bool run() override;
  /* Services */
  bool change_ef(const std::string & ef_name);

  bool move_ef(const Eigen::Vector3d & v, const Eigen::Matrix3d & m);

  bool move_com(const Eigen::Vector3d & v);

  bool next_phase();
public:
  mc_solver::CollisionsConstraint collsConstraint;
  std::shared_ptr<mc_tasks::CoMTask> comTask;
  std::shared_ptr<mc_tasks::EndEffectorTask> efTask;
private:
  void resetBasePose();
  EgressPhase phase;
  bool need_setup;
  void setup_movefootinside();
  void setup_liftbody();
  void setup_rotatebody();
};

}

#endif