#include "ThreadPool.h"
#include "CL/cl2.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

uint32_t mWidth{4000}, mHeight{3000};
uint32_t mGridX{8}, mGridY{6};
uint32_t mGridWidth{}, mGridHeight{};

uint16_t *outRGBImg{};
cl::CommandQueue mClQueue;

void processGrid(uint32_t x, uint32_t y) {
  cl_int status{CL_SUCCESS};
  printf("(%u, %u)\n", x, y);
  const size_t outputBufSize = mGridWidth * mGridHeight * sizeof(uint16_t);
  cl::Buffer dstRGB = cl::Buffer(CL_MEM_WRITE_ONLY, outputBufSize, nullptr, &status);
  if (CL_SUCCESS != status) throw "Error1";
  {
    cl::array<size_t, 3> buffer_origin = {0, 0, 0};
    cl::array<size_t, 3> host_origin = {mGridWidth * x * sizeof(uint16_t), mGridHeight * y, 0};
    cl::array<size_t, 3> region = {mGridWidth * sizeof(uint16_t), mGridHeight, 1};

    status = mClQueue.enqueueReadBufferRect(dstRGB, CL_TRUE, buffer_origin, host_origin, region, 0, 0, mWidth * sizeof(uint16_t), 0, outRGBImg, nullptr, nullptr);
    if (CL_SUCCESS != status) throw "Error2";
  }
}

int main() {
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  cl::Platform plat;
  for (auto &p : platforms) {
      std::string platver = p.getInfo<CL_PLATFORM_VERSION>();
      std::cout << "Plat: " << platver << "\n";
      if (platver.find("OpenCL 2.") != std::string::npos || platver.find("OpenCL 3.") != std::string::npos) {
          plat = p;
      }
  }
  if (plat() == 0) {
      std::cout << "No OpenCL 2.0 platform found \n";
      return -1;
  }

  cl::Platform newP = cl::Platform::setDefault(plat);
  if (newP != plat) {
      std::cout << "Error setting default platform.";
      return -1;
  }

  mClQueue = cl::CommandQueue(cl::QueueProperties::Profiling | cl::QueueProperties::OutOfOrder);
  while (mWidth % mGridX) {
    --mGridX;
  }
  while (mHeight % mGridY) {
    --mGridY;
  }
  mGridWidth = mWidth / mGridX;
  mGridHeight = mHeight / mGridY;
  auto safePtr = std::make_unique<uint16_t[]>(mWidth * mHeight);
  outRGBImg = safePtr.get();
  processGrid(1, 1);
  auto mThreadPool = std::make_unique<ThreadPool>(mGridX * mGridY);
  for (int y{}; y < mGridY; ++y) {
    for (int x{}; x < mGridX; ++x) {
        mThreadPool->Enqueue(&processGrid, x, y);
    }
  }
  mThreadPool->wait();
  std::cout << "Finish\n";
}
