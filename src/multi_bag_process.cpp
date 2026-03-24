#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <filesystem>

#include "rclcpp/rclcpp.hpp"
#include "rosbag2_cpp/reader.hpp"
#include "rosbag2_cpp/writer.hpp"
#include "rosbag2_cpp/readers/sequential_reader.hpp"
#include "rosbag2_cpp/writers/sequential_writer.hpp"
#include "rosbag2_storage/topic_metadata.hpp"

// 定义一个包装类，用于在优先级队列中排序
struct MessageEnvelope
{
  std::shared_ptr<rosbag2_storage::SerializedBagMessage> msg;
  size_t reader_id;

  // 优先级队列默认为大顶堆，我们需要小顶堆（时间戳越小优先级越高）
  // 所以这里使用 > 运算符
  bool operator>(const MessageEnvelope &other) const
  {
    return msg->time_stamp > other.msg->time_stamp;
  }
};

class BagMerger
{
public:
  BagMerger(const std::string &input_bag_dir, const std::string &output_bag_path)
  {
    // 1. 配置转换选项 (Foxy 默认使用 CDR 序列化)
    rosbag2_cpp::ConverterOptions converter_options;
    converter_options.input_serialization_format = "cdr";
    converter_options.output_serialization_format = "cdr";

    // 2. 初始化多个 Readers
    std::vector<std::unique_ptr<rosbag2_cpp::Reader>> readers;
    std::set<std::string> registered_topics;

    // 3. 初始化 Writer
    auto writer_impl = std::make_unique<rosbag2_cpp::writers::SequentialWriter>();
    auto writer = std::make_unique<rosbag2_cpp::Writer>(std::move(writer_impl));
    rosbag2_cpp::StorageOptions out_storage_opts;
    out_storage_opts.uri = output_bag_path;
    out_storage_opts.storage_id = "sqlite3";
    writer->open(out_storage_opts, converter_options);

    // 3. 从输入目录加载所有 bag 文件
    std::vector<std::string> input_bag_paths;
    namespace fs = std::filesystem;
    for (const auto &entry : fs::directory_iterator(input_bag_dir))
    {
      if (entry.is_regular_file() && entry.path().extension().string() == ".db3")
      {
        input_bag_paths.push_back(entry.path().string());
      }
    }
    std::cout << "Find " << input_bag_paths.size() << " bags in directory: " << input_bag_dir << std::endl;
    std::cout << "bag name:" << std::endl;
    for (const auto &bag : input_bag_paths)
    {
      std::cout << bag << std::endl;
    }
    std::cout << "------------------------" << std::endl;

    // 4. 打开所有 Reader 并注册话题
    for (size_t i = 0; i < input_bag_paths.size(); ++i)
    {
      auto reader_impl = std::make_unique<rosbag2_cpp::readers::SequentialReader>();
      auto reader = std::make_unique<rosbag2_cpp::Reader>(std::move(reader_impl));

      rosbag2_cpp::StorageOptions in_storage_opts;
      in_storage_opts.uri = input_bag_paths[i];
      in_storage_opts.storage_id = "sqlite3";

      try
      {
        reader->open(in_storage_opts, converter_options);

        // 将该包的所有话题合并到 Writer 中
        auto topics = reader->get_all_topics_and_types();
        for (const auto &topic : topics)
        {
          if (registered_topics.find(topic.name) == registered_topics.end())
          {
            writer->create_topic(topic);
            registered_topics.insert(topic.name);
          }
        }
        readers.push_back(std::move(reader));
      }
      catch (const std::exception &e)
      {
        std::cerr << "Failed to open bag: " << input_bag_paths[i] << " Error: " << e.what() << std::endl;
      }
    }

    // 5. 使用优先级队列进行多路归并
    std::priority_queue<MessageEnvelope, std::vector<MessageEnvelope>, std::greater<MessageEnvelope>> pq;

    // 初始填充：每个 reader 贡献第一条消息
    for (size_t i = 0; i < readers.size(); ++i)
    {
      if (readers[i]->has_next())
      {
        pq.push({readers[i]->read_next(), i});
      }
    }

    std::cout << "开始有序合并..." << std::endl;
    size_t total_msgs = 0;

    // 核心循环：始终取出全局时间戳最小的消息
    while (!pq.empty())
    {
      MessageEnvelope top = pq.top();
      pq.pop();

      // 写入消息到新包
      writer->write(top.msg);
      total_msgs++;

      // 如果该 reader 还有后续消息，再补充一条进堆
      if (readers[top.reader_id]->has_next())
      {
        pq.push({readers[top.reader_id]->read_next(), top.reader_id});
      }

      if (total_msgs % 500 == 0)
      {
        std::cout << "Processed " << total_msgs << " messages..." << std::endl;
      }
    }

    std::cout << "Merged successfully! Total " << total_msgs << " messages." << std::endl;
    std::cout << "Output bag path: " << output_bag_path << std::endl;
  }
};

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << "<input_bag_dir> <output_bag_path>" << std::endl;
    return 1;
  }

  std::string input_bag_dir = argv[2];
  std::string output_bag = argv[3];

  // 启动合并逻辑
  BagMerger merger(input_bag_dir, output_bag);

  return 0;
}
