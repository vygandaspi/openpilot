#include <sys/resource.h>
#include <limits.h>

#include <cstdio>
#include <cstdlib>

#include "cereal/visionipc/visionipc_client.h"
#include "common/swaglog.h"
#include "common/util.h"
#include "selfdrive/modeld/models/body.h"

ExitHandler do_exit;

void run_model(BodyModelState &model, VisionIpcClient &vipc_client) {
  PubMaster pm({"driverStateV2"});
  SubMaster sm({"carState", "carControl"});
  float inputs[INPUT_SIZE];

  while (!do_exit) {
    VisionIpcBufExtra extra = {};
    VisionBuf *buf = vipc_client.recv(&extra);
    if (buf == nullptr) continue;

    sm.update(0);
    if (sm.updated("carState")) {
      inputs[0] = sm["carState"].getCarState().getWheelSpeeds().getFl();
      inputs[1] = sm["carState"].getCarState().getWheelSpeeds().getFr();
      inputs[2] = sm["carState"].getCarState().getVEgo();
      inputs[3] = sm["carState"].getCarState().getAEgo();
    }
    if (sm.updated("carControl")) {
      inputs[4] = sm["carControl"].getCarControl().getOrientationNED()[1];
      inputs[5] = sm["carControl"].getCarControl().getAngularVelocity()[1];
    }

    BodyModelResult res = bodymodel_eval_frame(&model, inputs);

    // send dm packet
    MessageBuilder msg;
    auto framed = msg.initEvent().initDriverStateV2();
    auto data = framed.initLeftDriverData();
    framed.setFrameId(extra.frame_id);
    data.setLeftEyeProb(res.torque_left);
    data.setRightEyeProb(res.torque_right);
    pm.send("driverStateV2", msg);
  }
}

int main(int argc, char **argv) {
  setpriority(PRIO_PROCESS, 0, -15);

  // init the models
  BodyModelState model;
  bodymodel_init(&model);

  VisionIpcClient vipc_client = VisionIpcClient("camerad", VISION_STREAM_WIDE_ROAD, true);
  while (!do_exit && !vipc_client.connect(false)) {
    util::sleep_for(100);
  }

  // run the models
  if (vipc_client.connected) {
    LOGW("connected with buffer size: %d", vipc_client.buffers[0].len);
    run_model(model, vipc_client);
  }

  bodymodel_free(&model);
  return 0;
}