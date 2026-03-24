#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/serialization.hpp"
#include "rosbag2_cpp/reader.hpp"
#include "rosbag2_cpp/storage_options.hpp"
#include "rosbag2_storage/storage_interfaces/read_write_interface.hpp"
#include <rosbag2_cpp/readers/sequential_reader.hpp> // 必须包含这个头文件
#include "turtlesim/msg/pose.hpp"

using namespace std::chrono_literals;

class PlaybackNode : public rclcpp::Node
{
  public:
    PlaybackNode(const std::string & bag_filename)
    : Node("playback_node")
    {
      publisher_ = this->create_publisher<turtlesim::msg::Pose>("/turtle1/pose", 10);
      timer_ = this->create_wall_timer(
          100ms, std::bind(&PlaybackNode::timer_callback, this));

      rosbag2_cpp::StorageOptions storage_options;
      storage_options.uri = bag_filename;
      storage_options.storage_id = "sqlite3";
      auto reader_impl = std::make_unique<rosbag2_cpp::readers::SequentialReader>();

      // 2. 将实现传递给 Reader 的构造函数
      reader_ = std::make_unique<rosbag2_cpp::Reader>(std::move(reader_impl));
      
      rosbag2_cpp::ConverterOptions converter_options;
      converter_options.input_serialization_format = "cdr";
      converter_options.output_serialization_format = "cdr";
      reader_->open(storage_options, converter_options);
    }

  private:
    void timer_callback()
    {
      while (reader_->has_next()) {
        auto msg = reader_->read_next();

        if (msg->topic_name != "/turtle1/pose") {
          continue;
        }

        rclcpp::SerializedMessage serialized_msg(*msg->serialized_data);
        turtlesim::msg::Pose::SharedPtr ros_msg = std::make_shared<turtlesim::msg::Pose>();

        serialization_.deserialize_message(&serialized_msg, ros_msg.get());

        publisher_->publish(*ros_msg);
        std::cout << '(' << ros_msg->x << ", " << ros_msg->y << ")\n";

        break;
      }
    }

    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<turtlesim::msg::Pose>::SharedPtr publisher_;

    rclcpp::Serialization<turtlesim::msg::Pose> serialization_;
    std::unique_ptr<rosbag2_cpp::Reader> reader_;
};

int main(int argc, char ** argv)
{
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <bag>" << std::endl;
    return 1;
  }

  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PlaybackNode>(argv[1]));
  rclcpp::shutdown();

  return 0;
}
