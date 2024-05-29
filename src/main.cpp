#include <fstream>
#include <iostream>
#include <stacktrace>

#include <absl/log/initialize.h>
#include <absl/log/log.h>
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>

#include "demo_game.h"

class StdoutLogSink final : public absl::LogSink {
  void Send(const absl::LogEntry &entry) override {
    std::cout << entry.text_message_with_prefix_and_newline();
    log_file_ << entry.text_message_with_prefix_and_newline();
    if (entry.log_severity() == absl::LogSeverity::kError || entry.log_severity() == absl::LogSeverity::kFatal) {
      std::cout << to_string(std::stacktrace::current());
      log_file_ << to_string(std::stacktrace::current());
    }
  }

  std::ofstream log_file_{"log.txt"};
};

int main() {
  StdoutLogSink log_sink{};
  absl::AddLogSink(&log_sink);
  absl::InitializeLog();

  chove::DemoGame game{chove::windowing::RendererType::kVulkan};
  game.Run();
  return 0;
}
