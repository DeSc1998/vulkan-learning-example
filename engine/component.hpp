#pragma once

#include <vulkan/vulkan.hpp>

namespace ds {

  struct Component {
    float pos[4];
    float color[4];
    float time;

    static vk::VertexInputBindingDescription binding_description( );
    static std::array< vk::VertexInputAttributeDescription, 3 >
      attribute_description( );
  };

}