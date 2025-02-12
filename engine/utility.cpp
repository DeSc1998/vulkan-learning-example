
#include "pch.hpp"

#include "utility.hpp"

namespace ds {

#ifdef NDEBUG
  static constexpr auto debug_mode = false;
#else
  static constexpr auto debug_mode = true;
#endif

  std::string read_file( const fs::path& path ) {
    std::ifstream  file { path, std::ios::binary };
    std::stringbuf output { };

    if ( !file.is_open( ) ) {
      fmt::print( "ERROR: could not open file '{}'\n", path.c_str( ) );
      exit( 1 );
    }

    if constexpr ( debug_mode )
      fmt::print( "INFO: reading file '{}' \n", path.c_str( ) );

    file.get( output, EOF );
    return output.str( );
  }

  std::vector< std::string > split_by_lines( const std::string& str ) {
    auto                       iter        = str.begin( );
    auto                       substrbegin = str.begin( );
    std::vector< std::string > out { };
    while ( iter < str.end( ) ) {
      if ( *iter == '\n' ) {
        const auto size = iter - substrbegin;
        if ( size > 0 )
          out.emplace_back( std::string {
            &*substrbegin, static_cast< std::string::size_type >( size ) } );
        substrbegin = iter + 1;
      }
      ++iter;
    }

    return out;
  }

  std::vector< std::string > split_by_words( const std::string& str ) {
    auto                       iter        = str.begin( );
    auto                       substrbegin = str.begin( );
    std::vector< std::string > out { };
    while ( iter < str.end( ) ) {
      if ( *iter == ' ' ) {
        const auto size = iter - substrbegin;
        if ( size > 0 )
          out.emplace_back( std::string {
            &*substrbegin, static_cast< std::string::size_type >( size ) } );
        substrbegin = ++iter;
        continue;
      }
      ++iter;
    }
    const auto size = iter - substrbegin;
    if ( size > 0 )
      out.emplace_back( std::string {
        &*substrbegin, static_cast< std::string::size_type >( size ) } );

    return out;
  }

  void todo( std::string_view msg, std::source_location loc ) {
    if constexpr ( debug_mode ) {
      fmt::print( "  TODO: In {} on line {}: {}\n", loc.file_name( ),
                  loc.line( ), msg );
    }
  }

  void info( std::string_view msg ) {
    if constexpr ( debug_mode ) {
      fmt::print( "INFO: {}\n", msg );
    }
  }

  void error( std::string_view msg, vk::Result result ) {
    fmt::print( "ERROR: {}\n    Result code: {:d}\n", msg, (int64_t)result );
  }

  void exit_on_fail( std::string_view msg, vk::Result result ) {
    if ( result != vk::Result::eSuccess ) {
      error( msg, result );
      exit( 1 );
    }
  }

}
