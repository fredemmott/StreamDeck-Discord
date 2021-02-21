#include "DiscordStreamDeckPlugin.h"

#include <StreamDeckSDK/ESDMain.h>
#include <StreamDeckSDK/ESDLogger.h>

int main(int argc, const char** argv) {
  ESDDebug("int main()");
  auto result = esd_main(argc, argv, new DiscordStreamDeckPlugin());
  ESDDebug("Exit with code {}", result);
  return result;
}
