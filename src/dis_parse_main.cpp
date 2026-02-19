#include <cstdlib>
#include <iostream>
#include <string>

#include "bas/dis/dis_binary_parser.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "用法: bas_dis_parse <DIS二进制文件路径>\n";
    return EXIT_FAILURE;
  }

  try {
    bas::DisBinaryParser parser;
    const auto batches = parser.ParseFile(argv[1]);

    std::size_t entity_count = 0;
    std::size_t fire_count = 0;
    for (const auto& batch : batches) {
      entity_count += batch.entity_updates.size();
      fire_count += batch.fire_events.size();
    }

    std::cout << "输入文件: " << argv[1] << "\n";
    std::cout << "时间帧数: " << batches.size() << "\n";
    std::cout << "实体状态PDU数量: " << entity_count << "\n";
    std::cout << "开火PDU数量: " << fire_count << "\n";
  } catch (const std::exception& e) {
    std::cerr << "DIS解析失败: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
