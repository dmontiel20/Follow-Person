#include <memory>

#include "follow_person/detection_2_tf_node.hpp"
#include "rclcpp/rclcpp.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  auto detection2tf = std::make_shared<follow_person::Detection2TFNode>();

  rclcpp::spin(detection2tf);

  rclcpp::shutdown();
  return 0;
}