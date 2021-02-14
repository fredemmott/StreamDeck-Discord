/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#include "DiscordESDActionV2.h"

#include <StreamDeckSDK/ESDConnectionManager.h>
#include <StreamDeckSDK/ESDLogger.h>

DiscordESDActionV2::DiscordESDActionV2(
  ESDConnectionManager* esd,
  const std::string& context,
  std::shared_ptr<DiscordClient> discordClient)
  : ESDAction(esd, context), mDiscordClient(discordClient) {
    SetDiscordClient(discordClient);
}

void DiscordESDActionV2::WillAppear(const nlohmann::json&) {
  auto client = mDiscordClient.lock();
  if (!client) {
    GetESD()->ShowAlertForContext(GetContext());
    return;
  }

  GetESD()->SetState(GetDesiredState(*client), GetContext());
}

void DiscordESDActionV2::KeyUp(const nlohmann::json&) {
  auto client = mDiscordClient.lock();
  if (!client) {
    GetESD()->ShowAlertForContext(GetContext());
    return;
  }

  KeyUp(*client);
}

void DiscordESDActionV2::SetDiscordClient(
  const std::shared_ptr<DiscordClient>& client
) {
  mDiscordClient = client;
  if (!client) {
    GetESD()->ShowAlertForContext(GetContext());
    return;
  }
  ESDDebug("Got new client");
  Reconnected(*client);
  GetESD()->SetState(GetDesiredState(*client), GetContext());
  ESDDebug("Set initial state");
}
