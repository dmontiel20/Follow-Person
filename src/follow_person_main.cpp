#include <memory>

#include "follow_person/follow_person_node.hpp"
#include "rclcpp/rclcpp.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  auto followperson =  follow_person::FollowPersonNode::make_shared();

  rclcpp::spin(followperson->get_node_base_interface());

  rclcpp::shutdown();
  return 0;
}