#ifndef DETECTION_2_TF_NODE_HPP_
#define DETECTION_2_TF_NODE_HPP_

#include <memory>

#include "follow_person/PIDController.hpp"

#include "vision_msgs/msg/detection3_d_array.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/macros.hpp"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/transform_broadcaster.h"
#include "geometry_msgs/msg/twist.hpp"
#include "tf2_ros/buffer.h"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "geometry_msgs/msg/transform.hpp"
#include "tf2/convert.h"
#include "tf2/LinearMath/Transform.h"
#include <cmath>
#include "geometry_msgs/msg/quaternion.hpp"
#include "tf2/LinearMath/Matrix3x3.hpp"
#include "rclcpp/node.hpp"

namespace follow_person {

class Detection2TFNode : public rclcpp::Node
{
public:
  using Detection3D = vision_msgs::msg::Detection3DArray;
  using TransformStamped = geometry_msgs::msg::TransformStamped;
  Detection2TFNode();
private:
  void detection3d_callback(const Detection3D::ConstSharedPtr& detection);
  rclcpp::Subscription<Detection3D>::SharedPtr detection_sub_;
  std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
  TransformStamped transform_;

};

}

#endif