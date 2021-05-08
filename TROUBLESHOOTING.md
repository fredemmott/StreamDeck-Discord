# Troubleshooting

- Make sure you have the latest versions of Discord and the StreamDeck software installed
- Check that *everything* exactly matches. `https://localhost/`, `http://localhost/`, and `https://localhost` are considered different by Discord, and the plugin will not work if anything other than `https://localhost/` is used.
- Try regenerating app IDs and secrets, and copy-pasting again
- Make sure you are copying the app id and secrets from the 'OAuth2' section, not
  other information from the 'General Information' section
- Do not run Discord or the StreamDeck software as administrator
- Try restarting both Discord and the StreamDeck software
