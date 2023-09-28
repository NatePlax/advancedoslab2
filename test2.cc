#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <sys/resource.h>
#include <chrono>

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

#define	IO_SIZE 4096
void do_file_io(int fd, char *buf, uint64_t *offset_array, size_t n, int opt_read)
{
  int ret = 0;
  for (int i = 0; i < n; i++) {
    ret = lseek(fd, offset_array[i], SEEK_SET);
    if (ret == -1) {
      perror("lseek");
      exit(-1);
    }
    if (opt_read)
      ret = read(fd, buf, IO_SIZE);
    else
      ret = write(fd, buf, IO_SIZE);
    if (ret == -1) {
      perror("read/write");
      exit(-1);
    }
  }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <file_path> <sequential=0|random=1> <write=0|read=1>" << std::endl;
        return -1;
    }

    const char *file_path = argv[1];
    const int opt_random = std::atoi(argv[2]);
    const int opt_read = std::atoi(argv[3]);

    int num_iterations = 1;
    std::vector<double> durations;
    for (int idx = 0; idx < num_iterations; idx++) {
        // Open the file with O_DIRECT flag for direct I/O
        int fd = open(file_path, O_RDWR | O_DIRECT);
        if (fd == -1) {
            perror("open");
            return -1;
        }

        // Allocate aligned memory buffer for I/O
        char *buf;
        if (posix_memalign((void **)&buf, 4096, IO_SIZE) != 0) {
            perror("posix_memalign");
            close(fd);
            return -1;
        }

        // Get the file size for constructing offset_array
        off_t file_size = lseek(fd, 0, SEEK_END);
        if (file_size == -1) {
            perror("lseek");
            close(fd);
            free(buf);
            return -1;
        }

        // Construct offset_array for sequential or random read/write
        size_t num_requests = file_size / IO_SIZE;
        uint64_t *offset_array = new uint64_t[num_requests];
        for (size_t i = 0; i < num_requests; i++) {
            offset_array[i] = i * IO_SIZE;
        }

        // Perform sequential or random read
        if (opt_random) {
            std::random_shuffle(offset_array, offset_array + num_requests);
        }

        auto start_time = std::chrono::high_resolution_clock::now();        
        do_file_io(fd, buf, offset_array, num_requests, opt_read);
        auto end_time = std::chrono::high_resolution_clock::now();

        // Clean up
        delete[] offset_array;
        free(buf);
        close(fd);
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        durations.push_back(duration.count());

        struct rusage usage;

      // Call getrusage to get resource usage information for the current process
      if (getrusage(RUSAGE_SELF, &usage) == 0 && idx == 0) {
          // Print out the resource usage information
          std::cout << "Resource Usage Information:" << std::endl;
          std::cout << "User CPU Time (seconds): " << usage.ru_utime.tv_sec << "." << usage.ru_utime.tv_usec << std::endl;
          std::cout << "System CPU Time (seconds): " << usage.ru_stime.tv_sec << "." << usage.ru_stime.tv_usec << std::endl;
          std::cout << "Maximum Resident Set Size (KB): " << usage.ru_maxrss << std::endl;
          std::cout << "Page Faults Without IO: " << usage.ru_minflt << std::endl;
          std::cout << "Page Faults With IO: " << usage.ru_majflt << std::endl;
          std::cout << "Voluntary Context Switches: " << usage.ru_nvcsw << std::endl;
          std::cout << "Involuntary Context Switches: " << usage.ru_nivcsw << std::endl;
      }
    }
    auto tup = calculateAverageAndStdDevMilliseconds(durations);
    auto average = tup.first;
    auto std = tup.second;
    std::cout << "Execution time: " << average << " milliseconds" << std::endl;
    std::cout << "STD: " << std << " milliseconds" << std::endl;
    return 0;
}