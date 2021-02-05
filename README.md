![Continuous Integration](https://github.com/fredemmott/StreamDeck-Discord/workflows/Continuous%20Integration/badge.svg)

# StreamDeck-Discord

StreamDeck-Discord is a C++ plugin for the Elgato StreamDeck for controlling self-mute and self-deafen in the Discord app.

# Installation

Installation and demo video: https://youtu.be/MSMbRtj2fFA

1. Download the `com.fredemmott.discord.streamDeckPlugin` file from [the releases page](https://github.com/fredemmott/StreamDeck-Discord/releases/latest), and double-click it.
1. Add a mute or deafen button to your streamdeck
1. Go to [the Discord developer portal](https://discordapp.com/developers), and create an application
1. Go to 'OAuth2' on the left hand side
1. Add `https://localhost/` as a redirect URI
1. go back to 'General Information'
1. copy the client ID and paste it into 'app ID' in the StreamDeck button properties
1. copy the client secret and paste it into 'app secret' in the StreamDeck button properties
1. Discord should now ask you for permission a few times, then you're done :)
1. You might get a firewall prompt. You need to allow it.

# Need Help?

1. Try the suggestions in [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
2. If that does not resolve the problem, sign up/sign in to github, and click 'New discussion' on https://github.com/fredemmott/StreamDeck-Discord/discussions ; create an entry in the "Q & A" category

I'm not able to offer 1:1 help, e.g. via Discord.

# Limitations

- only one application can control Discord voice settings at a time. Stop anything else that controls Discord mute/deafen, such as the Discord applet in Logitech Gaming Software
- mute/deafen only apply while the application is running. For example, if you mute the microphone, then quit the StreamDeck software, the microphone might not be muted.

These limitations are imposed by Discord and can not be fixed by this plugin.

# Why does this plugin ask for my Discord username and avatar?

This information is not used or shared by the plugin. It asks for this permission as if it doesn't,
Discord will separately prompt for it anyway, even though it's not wanted. Asking for it up front
means you will get one permission prompt instead of two.

See https://support.discordapp.com/hc/en-us/community/posts/360038932712-allow-apps-without-identify-role

# Why does this plugin ask my firewall for permission to access the internet?

Discord plugins use OAuth2. The flow is basically:

1. this plugin asks the local Discord app for a code
1. the Discord app gives this plugin a code
1. the plugin asks the Discord web API to convert the code into an OAuth token
1. the Discord API gives the plugin the token
1. the plugin uses the token to log in to the client

The third step requires internet access.

See https://support.discordapp.com/hc/en-us/community/posts/360038666632-IPC-authentication

# Why does this plugin require me to register an app with Discord?

See https://support.discordapp.com/hc/en-us/community/posts/360038666632-IPC-authentication

# License

This project is [MIT-licensed](LICENSE).
