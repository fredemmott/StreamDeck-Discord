/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#include "DiscordESDAction.h"

#include <StreamDeckSDK/ESDConnectionManager.h>

DiscordESDAction::DiscordESDAction(
  ESDConnectionManager* esd,
  const std::string& context,
  std::weak_ptr<DiscordClient> discordClient)
  : ESDAction(esd, context), mDiscordClient(discordClient) {
  if (auto client = discordClient.lock()) {
    mDiscordState = client->getState();
  }
}

void DiscordESDAction::DiscordStateDidChange(
  std::weak_ptr<DiscordClient> client,
  const DiscordClient::State& newState) {
  mDiscordClient = client;
  const auto oldState = mDiscordState;
  mDiscordState = newState;

  const auto oldESDState = GetDesiredState(oldState);
  const auto newESDState = GetDesiredState(newState);
  if (oldESDState != newESDState) {
    GetESD()->SetState(newESDState, GetContext());
  }
}

void DiscordESDAction::WillAppear(const nlohmann::json&) {
  GetESD()->SetState(GetDesiredState(mDiscordState), GetContext());
}

void DiscordESDAction::KeyUp(const nlohmann::json&) {
  if (auto client = mDiscordClient.lock()) {
    KeyUp(client);
    return;
  }

  GetESD()->ShowAlertForContext(GetContext());
}