/**
  * Assignment: memory
  * Operating Systems
  */

// function/class definitions you are going to use
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdint>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

// although it is good habit, you don't have to type 'std::' before many objects by including this line
using namespace std;

// size of the image
const int64_t SIZE = 16384ULL;
const int64_t REPEAT = 1ULL;

int main(int argc, char* argv[]) {
  float* img = new float[SIZE * SIZE];

  // fill with dummy data
  for (int64_t i = 0; i < SIZE; i++) {
    for (int64_t j = 0; j < SIZE; j++) {
      img[j * SIZE + i] = (2 * j + i) % 32768;
    }
  }

  // this dummy value is needed to avoid compilers eliminating the loop as part of a optimisation
  uint64_t dummy = 0;

  // ADJUST BELOW, BUT keep writing to the dummy variable
  float* res = new float[SIZE * SIZE];  // result of mean filter
  const int block_size = 64;                                                      // CHANGED added a block size for Blocking

  // Apply an averaging imaging filter to some input image, and write in to an output image.
  // A pixel in the output image is calculated by averaging 9 pixels: the pixel at the same
  // coordinates in the input image, and the adjecent pixels.
  for (int64_t r = 0; r < REPEAT; ++r) {
    for (int64_t jj = 1; jj < SIZE - 1; jj += block_size) {                       // CHANGED introduced Blocking
      for (int64_t ii = 1; ii < SIZE - 1; ii += block_size) {                     // CHANGED introduced Blocking
        for (int64_t j = jj; j < std::min(jj + block_size, SIZE - 1); ++j) {      // CHANGED swapped i for j
          for (int64_t i = ii; i < std::min(ii + block_size, SIZE - 1); ++i) {    // CHANGED swapped j for i
            res[j * SIZE + i] = 0;
            
            for (long l = -1; l < 2; l++) {                                       // CHANGED swapped k for l
              for (long k = -1; k < 2; k++) {                                     // CHANGED swapped l for k
                res[j * SIZE + i] += img[(j + l) * SIZE + i + k];
              }
            }
            
            res[j * SIZE + i] /= 9;
            dummy += res[j * SIZE + i];                                           // CHANGED added dummy update
          }
        }
      }
    }                                                                             // CHANGED removed dummy update loop
  }

  delete[] res;

  // ADJUST ABOVE, BUT keep writing to the dummy variable
  delete[] img;

  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);

  std::cout << "user time:                    " << usage.ru_utime.tv_sec << "." << std::fixed << std::setw(6) << std::setprecision(6) << std::setfill('0') << usage.ru_utime.tv_usec << " s" << std::endl;
  std::cout << "soft page faults:             " << usage.ru_minflt << std::endl;
  std::cout << "hard page faults:             " << usage.ru_majflt << std::endl;
#ifdef __APPLE__
  std::cout << "max memory:                   " << usage.ru_maxrss/1024 << " KiB" << std::endl;
#else
  std::cout << "max memory:                   " << usage.ru_maxrss << " KiB" << std::endl;
#endif
  std::cout << "voluntary context switches:   " << usage.ru_nvcsw << std::endl;
  std::cout << "involuntary context switches: " << usage.ru_nivcsw << std::endl;
  std::cout << "dummy value (ignore):         " << dummy << std::endl; // this value is printed to avoid optimisations
  std::cout << "typical page size:            " << sysconf(_SC_PAGESIZE) << std::endl;
  // ...
  return 0;
}

