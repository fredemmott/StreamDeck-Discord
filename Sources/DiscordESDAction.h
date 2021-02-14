/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */
#pragma once

#include <StreamDeckSDK/ESDAction.h>

#include "DiscordClient.h"

class DiscordESDAction : public ESDAction {
 public:
  DiscordESDAction(
    ESDConnectionManager* esd,
    const std::string& context,
    std::shared_ptr<DiscordClient> client);

  virtual std::string GetActionID() const = 0;

  void SetDiscordClient(const std::shared_ptr<DiscordClient>& client);

  virtual void KeyUp(const nlohmann::json&) override final;
  virtual void WillAppear(const nlohmann::json&) override final;
 protected:

   virtual int GetDesiredState(const DiscordClient&) = 0;
   virtual void KeyUp(DiscordClient&) = 0;
   virtual void Reconnected(DiscordClient&) = 0;
  private:
   std::weak_ptr<DiscordClient> mDiscordClient;
};
