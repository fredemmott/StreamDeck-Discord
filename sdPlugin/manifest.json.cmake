{
  "Actions": [
    {
      "Icon": "discord-mic-off",
      "States": [
        {
          "Image": "discord-mic-on"
        },
        {
          "Image": "discord-mic-off"
        }
      ],
      "SupportedInMultiActions": false,
      "Name": "Toggle Self-Mute",
      "Tooltip": "Toggle Self-Mute in Discord",
      "UUID": "com.fredemmott.discord.mute"
    },
    {
      "Icon": "discord-mic-off",
      "States": [
        {
          "Image": "discord-mic-off"
        }
      ],
      "SupportedInMultiActions": true,
      "Name": "Self-Mute On",
      "Tooltip": "Set Self-Mute in Discord to On",
      "UUID": "com.fredemmott.discord.muteon"
    },
    {
      "Icon": "discord-mic-on",
      "States": [
        {
          "Image": "discord-mic-on"
        }
      ],
      "SupportedInMultiActions": true,
      "Name": "Self-Mute Off",
      "Tooltip": "Set Self-Mute in Discord to Off",
      "UUID": "com.fredemmott.discord.muteoff"
    },
    {
      "Icon": "discord-deafen-off",
      "States": [
        {
          "Image": "discord-deafen-off"
        },
        {
          "Image": "discord-deafen-on"
        }
      ],
      "SupportedInMultiActions": false,
      "Name": "Toggle Self-Deafen",
      "Tooltip": "Toggle Self-Deafen in Discord",
      "UUID": "com.fredemmott.discord.deafen"
    },
    {
      "Icon": "discord-deafen-on",
      "States": [
        {
          "Image": "discord-deafen-on"
        }
      ],
      "SupportedInMultiActions": true,
      "Name": "Self-Deafen On",
      "Tooltip": "Set Deafen in Discord to On",
      "UUID": "com.fredemmott.discord.deafenon"
    },
    {
      "Icon": "discord-deafen-off",
      "States": [
        {
          "Image": "discord-deafen-off"
        }
      ],
      "SupportedInMultiActions": true,
      "Name": "Self-Deafen Off",
      "Tooltip": "Set Deafen in Discord to Off",
      "UUID": "com.fredemmott.discord.deafenoff"
    },
    {
      "Icon": "discord-ptt",
      "States": [
        {
          "Image": "discord-va"
        },
        {
          "Image": "discord-ptt"
        }
      ],
      "SupportedInMultiActions": false,
      "Name": "Toggle PTT and VA Modes",
      "Tooltip": "Toggle Discord between Push-to-Talk and Voice-Activated",
      "UUID": "com.fredemmott.discord.ptt"
    },
    {
      "Icon": "discord-va",
      "States": [
        {
          "Image": "discord-va"
        },
        {
          "Image": "discord-va"
        }
      ],
      "SupportedInMultiActions": true,
      "Name": "Voice-Activated Mode",
      "Tooltip": "Set Push-To-Talk off",
      "UUID": "com.fredemmott.discord.pttoff"
    },
    {
      "Icon": "discord-ptt",
      "States": [
        {
          "Image": "discord-ptt"
        },
        {
          "Image": "discord-ptt"
        }
      ],
      "SupportedInMultiActions": true,
      "Name": "Push-To-Talk Mode",
      "Tooltip": "Set Voice-Activated off)",
      "UUID": "com.fredemmott.discord.ptton"
    }
  ],
  "Author": "Fred Emmott",
  "CodePathMac": "sddiscord",
  "CodePathWin": "sddiscord.exe",
  "PropertyInspectorPath": "propertyinspector/index.html",
  "Description": "Discord Self-Mute/Deafen and PTT/VA",
  "Name": "Discord Self-Mute/Deafen and PTT/VA",
  "Category": "Discord",
  "Icon": "discord-mic-off",
  "Version": "${CMAKE_PROJECT_VERSION}",
  "URL": "https://github.com/fredemmott/streamdeck-discord",
  "OS": [
    {
        "Platform": "windows",
        "MinimumVersion" : "10"
    },
    {
        "Platform": "mac",
        "MinimumVersion": "10.11"
    }
  ],
  "SDKVersion": 2,
  "Software": {
    "MinimumVersion" : "4.1"
  }
}
