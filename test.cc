#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <chrono>
#include <vector>
#include <cmath>

#define REGION_SIZE (1ULL << 30) // 1GB

void shuffle(uint64_t *array, size_t n)
{
    if (n > 1)
    {
        size_t i;
        for (i = 0; i < n - 1; i++)
        {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            uint64_t t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

std::pair<double, double> calculateAverageAndStdDevMilliseconds(std::vector<double>& durationsMillis) {
    double sum = 0.0;
    double sumSquared = 0.0;

    // Calculate the sum of durations in milliseconds
    for (const double& durationMillis : durationsMillis) {
        sum += durationMillis;
        sumSquared += durationMillis * durationMillis;
    }

    // Calculate the average in milliseconds
    double averageMillis = sum / durationsMillis.size();

    // Calculate the standard deviation in milliseconds
    double varianceMillis = (sumSquared / durationsMillis.size()) - (averageMillis * averageMillis);
    double stdDeviationMillis = std::sqrt(varianceMillis);

    return std::make_pair(averageMillis, stdDeviationMillis);
}

int main()
{
    int num_iterations = 1;
    std::vector<double> durations;
    for (int idx = 0; idx < num_iterations; idx++) {
        // Open a file for memory mapping
        int fd = open("file-1g", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd == -1)
        {
        perror("open");
        return 1;
        }

        // Extend the file to the desired size
        if (ftruncate(fd, REGION_SIZE) == -1)
        {
            perror("ftruncate");
            close(fd);
            return 1;
        }
        
        // Memory map the file
        uint8_t *mapped_region = static_cast<uint8_t *>(mmap(nullptr, REGION_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));
        if (mapped_region == MAP_FAILED)
        {
            perror("mmap");
            close(fd);
            return 1;
        }

        // Create an array of page indices and shuffle them
        size_t num_pages = REGION_SIZE / getpagesize();
        uint64_t *page_indices = new uint64_t[num_pages];

        for (size_t i = 0; i < num_pages; i++)
        {
            page_indices[i] = i * getpagesize();
        }

        srand(static_cast<unsigned int>(time(nullptr)));
        shuffle(page_indices, num_pages);

        auto start_time = std::chrono::high_resolution_clock::now();
        // Write the first byte of each page in the shuffled order
        for (size_t i = 0; i < num_pages; i++)
        {
            // std::cout << i << std::endl;
            mapped_region[page_indices[i]] = 0xFF; // Write the first byte of the page
        }

        auto end_time = std::chrono::high_resolution_clock::now();

        // Sync changes to the disk
        if (msync(mapped_region, REGION_SIZE, MS_SYNC) == -1)
        {
            perror("msync");
        }

        
        // Clean up and close the file
        delete[] page_indices;
        munmap(mapped_region, REGION_SIZE);
        close(fd);
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        durations.push_back(duration.count());
    }
    auto tup = calculateAverageAndStdDevMilliseconds(durations);
    auto average = tup.first;
    auto std = tup.second;
    std::cout << "Execution time: " << average << " milliseconds" << std::endl;
    std::cout << "STD: " << std << " milliseconds" << std::endl;
    return 0;
}
