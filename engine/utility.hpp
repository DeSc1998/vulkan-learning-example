#pragma once

#include <filesystem>
#include <source_location>
#include <string>
#include <string_view>

namespace ds {

  namespace fs = std::filesystem;

  std::string                read_file( const fs::path& path );
  std::vector< std::string > split_by_lines( const std::string& );
  std::vector< std::string > split_by_words( const std::string& );

  void todo( std::string_view msg,
             std::source_location = std::source_location::current( ) );
  void info( std::string_view msg );

  void error( std::string_view msg,
              vk::Result       result = vk::Result::eErrorUnknown );
  void exit_on_fail( std::string_view msg, vk::Result result );

}
