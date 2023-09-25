#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <filename> <num_iterations>" << std::endl;
        return 1;
    }

    const std::string filename = argv[1];
    const int num_iterations = std::stoi(argv[2]);

    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        return 1;
    }

    // Get the file size
    file.seekg(0, std::ios::end);
    const std::streampos file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (file_size <= 0) {
        std::cerr << "Error: File '" << filename << "' is empty or cannot be read." << std::endl;
        file.close();
        return 1;
    }

    // Read the file sequentially multiple times
    for (int i = 0; i < num_iterations; i++) {
        char *buffer = new char[file_size];

        file.read(buffer, file_size);

        if (!file) {
            std::cerr << "Error reading file '" << filename << "' during iteration " << i + 1 << std::endl;
            delete[] buffer;
            file.close();
            return 1;
        }

        delete[] buffer;
        file.clear();
        file.seekg(0, std::ios::beg);
    }

    std::cout << "Read file '" << filename << "' into page cache " << num_iterations << " times." << std::endl;

    file.close();
    return 0;
}
