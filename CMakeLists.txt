cmake_minimum_required(VERSION 3.16)
project(merge_bags)
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)

# Foxy依赖包
find_package(rclcpp REQUIRED)
find_package(rosbag2_cpp REQUIRED)
find_package(rosbag2_transport REQUIRED)
find_package(turtlesim REQUIRED)
find_package(rosbag2_cpp REQUIRED)
find_package(rosbag2_storage REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(sbg_driver REQUIRED)
find_package(nmea_msgs REQUIRED)
find_package(nav_msgs REQUIRED)

add_executable(simple_bag_recorder src/simple_bag_recorder.cpp)
ament_target_dependencies(simple_bag_recorder rclcpp rosbag2_cpp)

install(TARGETS
  simple_bag_recorder
  DESTINATION lib/${PROJECT_NAME}
)

add_executable(simple_bag_reader src/simple_bag_reader.cpp)
ament_target_dependencies(simple_bag_reader rclcpp rosbag2_transport turtlesim)

install(TARGETS
  simple_bag_reader
  DESTINATION lib/${PROJECT_NAME}
)

add_executable(bag_merge_tool src/multi_bag_process.cpp)
target_link_libraries(bag_merge_tool stdc++fs)
ament_target_dependencies(bag_merge_tool 
	rclcpp 
	rosbag2_cpp
  rosbag2_storage
  sensor_msgs
  sbg_driver
  nmea_msgs
  nav_msgs
)
install(TARGETS
  bag_merge_tool
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
