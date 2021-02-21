/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#include "DiscordESDAction.h"

#include <StreamDeckSDK/ESDConnectionManager.h>
#include <StreamDeckSDK/ESDLogger.h>

DiscordESDAction::DiscordESDAction(
  ESDConnectionManager* esd,
  const std::string& action,
  const std::string& context,
  std::shared_ptr<DiscordClient> discordClient)
  : ESDAction(esd, action, context) {
  if (discordClient && discordClient->getState().rpcState == DiscordClient::RpcState::READY) {
    asio::post([=]() { SetDiscordClient(discordClient); });
  }
}

void DiscordESDAction::WillAppear(const nlohmann::json&) {
  auto client = mDiscordClient.lock();
  if (!client) {
    ShowAlert();
    return;
  }

  SetState(GetDesiredState(*client));
}

void DiscordESDAction::KeyUp(const nlohmann::json& settings) {
  auto client = mDiscordClient.lock();
  if (!client) {
    ShowAlert();
    return;
  }

  KeyUp(settings, *client);
}

void DiscordESDAction::SetDiscordClient(
  const std::shared_ptr<DiscordClient>& client
) {
  mDiscordClient = client;
  if (!client) {
    ESDDebug("Set null client");
    ShowAlert();
    return;
  }
  ESDDebug("Set non-null client");
  Reconnected(*client);
  SetState(GetDesiredState(*client));
}

std::weak_ptr<DiscordClient> DiscordESDAction::GetDiscordClient() {
  return mDiscordClient;
}
