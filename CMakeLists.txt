cmake_minimum_required(VERSION 3.8)
project(follow_person)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(vision_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(message_filters REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(pcl_ros REQUIRED)
find_package(image_geometry REQUIRED)
find_package(depth_image_proc REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(rclcpp_lifecycle)
find_package(OpenCV REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)

set(dependencies
  rclcpp
  sensor_msgs
  vision_msgs
  image_transport
  cv_bridge
  OpenCV
  message_filters
  pcl_conversions
  pcl_ros
  image_geometry
  depth_image_proc
  geometry_msgs
  tf2_geometry_msgs
  tf2
  tf2_ros
  rclcpp_lifecycle
)

include_directories(include)

add_library(${PROJECT_NAME} SHARED
  src/follow_person/follow_person_node.cpp
  src/follow_person/detection_2_tf_node.cpp
  src/follow_person/PIDController.cpp
)
ament_target_dependencies(${PROJECT_NAME} ${dependencies})

add_executable(follow_person_main src/follow_person_main.cpp)
ament_target_dependencies(follow_person_main ${dependencies})
target_link_libraries(follow_person_main ${PROJECT_NAME})

add_executable(detection_2_tf_main src/detection_2_tf_main.cpp)
ament_target_dependencies(detection_2_tf_main ${dependencies})
target_link_libraries(detection_2_tf_main ${PROJECT_NAME})

install(TARGETS
  ${PROJECT_NAME}
  detection_2_tf_main
  follow_person_main
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)

  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)

  ament_lint_auto_find_test_dependencies()
endif()

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_dependencies(${dependencies})
ament_package()
