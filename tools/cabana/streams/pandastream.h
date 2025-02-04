#pragma once

#include "tools/cabana/streams/livestream.h"
#include "selfdrive/boardd/panda.h"

struct BusConfig {
  int can_speed_kbps = 500;
  int data_speed_kbps = 2000;
  bool can_fd = false;
};

struct PandaStreamConfig {
  QString serial = "";
  std::vector<BusConfig> bus_config;
};

class PandaStream : public LiveStream {
  Q_OBJECT
public:
  PandaStream(QObject *parent, PandaStreamConfig config_ = {});

  inline QString routeName() const override {
    return QString("Live Streaming From Panda %1").arg(config.serial);
  }

protected:
  void streamThread() override;
  bool connect();

  std::unique_ptr<Panda> panda;
  PandaStreamConfig config = {};
};

