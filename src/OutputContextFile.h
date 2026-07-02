#ifndef HAD_OUTPUT_CONTEXT_FILE_H
#define HAD_OUTPUT_CONTEXT_FILE_H

#include <filesystem>
#include <fstream>
#include <optional>

#include "OutputContext.h"

class OutputContextFile : public OutputContext {
  public:
    OutputContextFile(const std::optional<std::filesystem::path>& file_name, bool binary = false);
    ~OutputContextFile() override;

  protected:
    bool close() override;

    // Utility methods for subclasses.
    void cond_print_string(const std::string& pre, const std::string& str, const std::string& post);
    void cond_print_hash(const std::string& pre, int t, const Hashes* h, const std::string& post);

    [[nodiscard]] bool is_open() const { return is_stdout() || file_stream.is_open(); }
    [[nodiscard]] bool is_stdout() const { return !file_name.has_value(); }

    std::optional<std::filesystem::path> file_name;
    std::ostream& stream;

  private:
    std::ofstream file_stream;
};

#endif // HAD_OUTPUT_CONTEXT_FILE_H
