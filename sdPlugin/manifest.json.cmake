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
	    "Name": "Toggle Discord Mute",
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
      "Name": "Discord Mute On",
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
      "Name": "Discord Mute Off",
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
	    "Name": "Toggle Discord Deafen",
      "Tooltip": "Toggle Deafen in Discord",
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
      "Name": "Set Discord Deafen On",
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
      "Name": "Set Discord Deafen Off",
      "Tooltip": "Set Deafen in Discord to Off",
      "UUID": "com.fredemmott.discord.deafenoff"
    },
    {
      "Icon": "discord-deafen-off",
      "States": [
        {
          "Image": "discord-deafen-off"
        }
      ],
      "SupportedInMultiActions": true,
      "Name": "Set Discord PTT OFF",
      "Tooltip": "Set PTT in Discord to Off",
      "UUID": "com.fredemmott.discord.pttoff"
    },
    {
      "Icon": "discord-deafen-on",
      "States": [
        {
          "Image": "discord-deafen-on"
        }
      ],
      "SupportedInMultiActions": true,
      "Name": "Set Discord PTT ON",
      "Tooltip": "Set PTT in Discord to ON",
      "UUID": "com.fredemmott.discord.ptton"
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
	    "Name": "Toggle Discord PTT",
      "Tooltip": "Toggle PTT in Discord",
      "UUID": "com.fredemmott.discord.ptt"
    }
  ],
  "Author": "Fred Emmott",
  "CodePathMac": "sddiscord",
  "CodePathWin": "sddiscord.exe",
  "PropertyInspectorPath": "propertyinspector/index.html",
  "Description": "Toggle Discord Mute and Deafen.",
  "Name": "Discord Mute/Deafen",
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
