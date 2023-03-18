#pragma once

#include <filesystem>
#include <source_location>
#include <string>
#include <string_view>

namespace ds {

  namespace fs = std::filesystem;

  enum class Error_code { success = 0, open_display_failure, unknwon_error };

  std::string read_file( const fs::path& path );

  void todo( std::string_view msg,
             std::source_location = std::source_location::current( ) );
  void info( std::string_view msg );

  void error( std::string_view msg,
              vk::Result       result = vk::Result::eErrorUnknown );
  void exit_on_fail( std::string_view msg, vk::Result result );

}