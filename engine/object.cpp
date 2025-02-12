
#include <string>
#include <string_view>

#include "object.hpp"
#include "utility.hpp"

namespace ds {
  Model model_from_file( const std::string file_path ) {
    const auto file  = read_file( file_path );
    const auto lines = split_by_lines( file );
    Model      model { };
    for ( const auto& line : lines ) {
      if ( line.empty( ) )
        continue;

      const auto tokens = split_by_words( line );
      auto       iter   = tokens.begin( );
      const auto prefix = std::string_view { *iter++ };
      if ( prefix.compare( "v" ) == 0 ) {
        const float x = std::atof( ( *iter++ ).c_str( ) );
        const float y = std::atof( ( *iter++ ).c_str( ) );
        const float z = std::atof( ( *iter++ ).c_str( ) );
        model.vertices.emplace_back( x, y, z );
      }
      if ( prefix.compare( "f" ) == 0 ) {
        const auto x = std::atol( ( *iter++ ).c_str( ) );
        const auto y = std::atol( ( *iter++ ).c_str( ) );
        const auto z = std::atol( ( *iter++ ).c_str( ) );
        model.faces.emplace_back( x, y, z );
      }
    }

    return model;
  }
}
