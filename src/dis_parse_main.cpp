#include <cstdlib>
#include <iostream>
#include <string>

#include "bas/dis/dis_binary_parser.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: bas_dis_parse <dis_binary_file>\n";
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

    std::cout << "InputFile: " << argv[1] << "\n";
    std::cout << "Frames: " << batches.size() << "\n";
    std::cout << "EntityPDUs: " << entity_count << "\n";
    std::cout << "FirePDUs: " << fire_count << "\n";
  } catch (const std::exception& e) {
    std::cerr << "DIS parse failed: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
