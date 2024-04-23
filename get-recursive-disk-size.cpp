#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace fs = std::filesystem;

// Define a structure to hold disk space usage and last modified time for each
// directory
struct DirectoryInfo {
  size_t diskSpaceUsage;
  size_t lastModified;
};

// Define a thread-safe cache of directory information
std::unordered_map<fs::path, DirectoryInfo> cache;
std::mutex cacheMutex;

// Function to calculate disk space usage for a directory
size_t calculateDiskSpace(const fs::path &directory) {
  size_t totalSpaceUsed = 0;

  for (const auto &entry : fs::recursive_directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      totalSpaceUsed += fs::file_size(entry.path());
    }
  }

  return totalSpaceUsed;
}

// Function to get disk space usage for a directory, with caching and thread
// safety
size_t getDiskSpaceUsage(const fs::path &directory) {
  std::lock_guard<std::mutex> lock(cacheMutex);

  size_t lastModified =
      fs::last_write_time(directory).time_since_epoch().count();

  // Check if directory information is present in the cache
  auto it = cache.find(directory);
  if (it != cache.end()) {
    // Check if directory has been modified since last calculation
    if (lastModified <= it->second.lastModified) {
      return it->second
          .diskSpaceUsage; // Return cached value if directory is unmodified
    }
  }

  // Calculate disk space usage for the directory
  size_t diskSpaceUsage = calculateDiskSpace(directory);

  // Update cache with new disk space usage and last modified time
  cache[directory] = DirectoryInfo{diskSpaceUsage, lastModified};

  return diskSpaceUsage;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <directory_path>\n";
    return 1;
  }

  fs::path directory(argv[1]);
  size_t sleepInSeconds = 0;
  if (argc == 3) {
    sleepInSeconds = std::stol(argv[2]);
  }

  // Measure time for first call
  auto start1 = std::chrono::steady_clock::now();
  std::cout << "Disk space usage for " << directory << ": "
            << getDiskSpaceUsage(directory) << " bytes\n";
  auto end1 = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed_seconds1 = end1 - start1;
  std::cout << "Time elapsed for first call: " << elapsed_seconds1.count()
            << "s\n";

  // Sleep to simulate modification of directory
  std::this_thread::sleep_for(std::chrono::seconds(sleepInSeconds));

  // Measure time for second call
  auto start2 = std::chrono::steady_clock::now();
  std::cout << "Disk space usage for " << directory
            << " (after modification): " << getDiskSpaceUsage(directory)
            << " bytes\n";
  auto end2 = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed_seconds2 = end2 - start2;
  std::cout << "Time elapsed for second call: " << elapsed_seconds2.count()
            << "s\n";

  return 0;
}
