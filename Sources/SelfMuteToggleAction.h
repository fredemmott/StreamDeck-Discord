#pragma once

#include "DiscordESDAction.h"

class SelfMuteToggleAction final : public DiscordESDAction {
 public:
  using DiscordESDAction::DiscordESDAction;
  
  static const std::string ACTION_ID;
  virtual std::string GetActionID() const override { return ACTION_ID; }

  virtual void KeyUp(std::shared_ptr<DiscordClient> client) override {
    client->setIsMuted(!(mDiscordState.isMuted || mDiscordState.isDeafened));
  }

  virtual int GetDesiredState(const DiscordClient::State& state) override {
      return (state.isMuted || state.isDeafened) ? 1 : 0;
  }
};