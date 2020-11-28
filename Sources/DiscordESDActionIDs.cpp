/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */
#include "DeafenOffAction.h"
#include "DeafenOnAction.h"
#include "DeafenToggleAction.h"
#include "SelfMuteOffAction.h"
#include "SelfMuteOnAction.h"
#include "SelfMuteToggleAction.h"

const std::string DeafenOffAction::ACTION_ID = "com.fredemmott.discord.deafenoff"; 
const std::string DeafenOnAction::ACTION_ID = "com.fredemmott.discord.deafenon"; 
const std::string DeafenToggleAction::ACTION_ID = "com.fredemmott.discord.deafen"; 
const std::string SelfMuteOffAction::ACTION_ID = "com.fredemmott.discord.muteoff"; 
const std::string SelfMuteOnAction::ACTION_ID = "com.fredemmott.discord.muteon"; 
const std::string SelfMuteToggleAction::ACTION_ID = "com.fredemmott.discord.mute"; 