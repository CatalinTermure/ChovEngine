#include "logging.h"

namespace chove {

void Logging::Log(Logging::Level level, const std::string &message) {

  std::ostream *os = &std::cout;
  if (level == Level::kWarning || level == Level::kEsoteric || level == Level::kError || level == Level::kFatal) {
    os = &std::cerr;
  }

  switch (level) {
    case Level::kInfo:(*os) << "[INFO]: ";
      break;
    case Level::kWarning:(*os) << "[WARNING]: ";
      break;
    case Level::kEsoteric:(*os) << "[ESOTERIC]: ";
      break;
    case Level::kError:(*os) << "[ERROR]: ";
      break;
    case Level::kFatal:(*os) << "[FATAL]: ";
      break;
  }

  (*os) << message << '\n';

  if (level == Level::kFatal) {
    exit(1);
  }
}
}
