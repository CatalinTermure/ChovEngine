#ifndef LABSEXTRA_UTILS_LOGGING_H_
#define LABSEXTRA_UTILS_LOGGING_H_

#include <iostream>

namespace chove {
class Logging {
 public:
  Logging() = delete;

  enum class Level {
    kInfo,
    kWarning,
    kEsoteric,
    kError,
    kFatal
  };

  static void Log(Logging::Level level, const std::string &message);
};
}

#endif //LABSEXTRA_UTILS_LOGGING_H_
