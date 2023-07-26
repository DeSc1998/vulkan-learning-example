#pragma once

#include <vulkan/vulkan.hpp>

namespace ds {

  struct Component {
    float pos[3];
    float color[4];
    float time;

    static vk::VertexInputBindingDescription binding_description( );
    static std::array< vk::VertexInputAttributeDescription, 3 >
      attribute_description( );

    static std::vector< Component > read_from_file( std::string_view );
  };

}