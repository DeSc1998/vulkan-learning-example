
#include <fmt/format.h>
#include <string>
#include <string_view>

#include "component.hpp"
#include "utility.hpp"

namespace ds {

  vk::VertexInputBindingDescription Component::binding_description( ) {
    vk::VertexInputBindingDescription tmp { };

    tmp.binding   = 0;
    tmp.stride    = sizeof( Component );
    tmp.inputRate = vk::VertexInputRate::eVertex;

    return tmp;
  }

  std::array< vk::VertexInputAttributeDescription, 3 >
    Component::attribute_description( ) {
    std::array< vk::VertexInputAttributeDescription, 3 > out { };

    out[0].binding  = 0;
    out[0].location = 0;
    out[0].format   = vk::Format::eR32G32B32Sfloat;
    out[0].offset   = offsetof( Component, pos );

    out[1].binding  = 0;
    out[1].location = 1;
    out[1].format   = vk::Format::eR32G32B32A32Sfloat;
    out[1].offset   = offsetof( Component, color );

    out[2].binding  = 0;
    out[2].location = 2;
    out[2].format   = vk::Format::eR32Sfloat;
    out[2].offset   = offsetof( Component, time );

    return out;
  }

  Component parse( std::string_view input ) {
    Component out { };
    size_t    pos = 0;
    // position
    for ( size_t i = 0; i < 3; i++ ) {
      // fmt::print( "info: reading index {} of position\n", i );

      size_t tmp;
      auto   s = input.substr( pos );

      std::string curr { s.data( ), s.size( ) };
      out.pos[i] = std::stof( curr, &tmp );
      pos += tmp;
    }

    // color
    for ( size_t i = 0; i < 4; i++ ) {
      // fmt::print( "info: reading index {} of color\n", i );

      size_t tmp;
      auto   s = input.substr( pos );

      std::string curr { s.data( ), s.size( ) };
      out.color[i] = std::stof( curr, &tmp );
      pos += tmp;
    }

    return out;
  }

  std::vector< Component >
    Component::read_from_file( std::string_view filepath ) {
    std::vector< Component > out { };

    auto content = read_file( filepath );
    auto lines   = split_by_lines( content );

    for ( const auto& line : lines ) {
      out.emplace_back( parse( line ) );
    }

    return out;
  }

}