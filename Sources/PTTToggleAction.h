#pragma once

#include "DiscordESDAction.h"

class PTTToggleAction final : public DiscordESDAction {
 public:
  using DiscordESDAction::DiscordESDAction;
  static const std::string ACTION_ID;
  virtual std::string GetActionID() const override { return ACTION_ID; }

  virtual void KeyUp(std::shared_ptr<DiscordClient> client) override {
    client->setIsPTT(!mDiscordState.isPTT);
  }

  virtual int GetDesiredState(const DiscordClient::State& state) override {
      return state.isPTT ? 1 : 0;
  }
};