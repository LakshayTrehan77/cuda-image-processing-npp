// Batch image processing using CUDA NPP: Gaussian blur + Sobel edge detection
// Processes all .pgm images in an input directory and writes results to output directory.

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#pragma warning(disable : 4819)
#endif

#include <Exceptions.h>
#include <ImageIO.h>
#include <ImagesCPU.h>
#include <ImagesNPP.h>

#include <dirent.h>
#include <string.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <cuda_runtime.h>
#include <npp.h>

#include <helper_cuda.h>
#include <helper_string.h>

// Print CUDA and NPP version info
static void printDeviceInfo() {
  const NppLibraryVersion *libVer = nppGetLibVersion();
  printf("NPP Library Version %d.%d.%d\n", libVer->major, libVer->minor, libVer->build);

  int driverVersion, runtimeVersion;
  cudaDriverGetVersion(&driverVersion);
  cudaRuntimeGetVersion(&runtimeVersion);
  printf("CUDA Driver Version:  %d.%d\n", driverVersion / 1000, (driverVersion % 100) / 10);
  printf("CUDA Runtime Version: %d.%d\n", runtimeVersion / 1000, (runtimeVersion % 100) / 10);
}

// Collect all .pgm files from a directory
static std::vector<std::string> collectPgmFiles(const std::string &dir) {
  std::vector<std::string> files;
  DIR *dp = opendir(dir.c_str());
  if (!dp) {
    std::cerr << "Cannot open input directory: " << dir << std::endl;
    return files;
  }
  struct dirent *entry;
  while ((entry = readdir(dp)) != nullptr) {
    std::string name(entry->d_name);
    if (name.size() > 4 && name.substr(name.size() - 4) == ".pgm") {
      files.push_back(dir + "/" + name);
    }
  }
  closedir(dp);
  return files;
}

// Apply Gaussian blur then Sobel edge detection via NPP on a single image
static bool processImage(const std::string &inputPath,
                         const std::string &outputPath,
                         const std::string &filter) {
  npp::ImageCPU_8u_C1 hostSrc;
  npp::loadImage(inputPath, hostSrc);
  npp::ImageNPP_8u_C1 devSrc(hostSrc);

  NppiSize roi = {(int)devSrc.width(), (int)devSrc.height()};

  if (filter == "blur" || filter == "all") {
    // Gaussian blur with 5x5 mask
    npp::ImageNPP_8u_C1 devBlur(roi.width, roi.height);
    NppiMaskSize maskSize = NPP_MASK_SIZE_5_X_5;
    NPP_CHECK_NPP(nppiFilterGauss_8u_C1R(
        devSrc.data(), devSrc.pitch(),
        devBlur.data(), devBlur.pitch(),
        roi, maskSize));

    if (filter == "blur") {
      npp::ImageCPU_8u_C1 hostDst(devBlur.size());
      devBlur.copyTo(hostDst.data(), hostDst.pitch());
      saveImage(outputPath, hostDst);
      nppiFree(devBlur.data());
      nppiFree(devSrc.data());
      return true;
    }
    // chain into Sobel for "all"
    npp::ImageNPP_8u_C1 devEdge(roi.width, roi.height);
    NPP_CHECK_NPP(nppiFilterSobelHoriz_8u_C1R(
        devBlur.data(), devBlur.pitch(),
        devEdge.data(), devEdge.pitch(),
        roi));
    npp::ImageCPU_8u_C1 hostDst(devEdge.size());
    devEdge.copyTo(hostDst.data(), hostDst.pitch());
    saveImage(outputPath, hostDst);
    nppiFree(devBlur.data());
    nppiFree(devEdge.data());
    nppiFree(devSrc.data());
    return true;
  }

  // default: Sobel edge detection only
  npp::ImageNPP_8u_C1 devDst(roi.width, roi.height);
  NPP_CHECK_NPP(nppiFilterSobelHoriz_8u_C1R(
      devSrc.data(), devSrc.pitch(),
      devDst.data(), devDst.pitch(),
      roi));
  npp::ImageCPU_8u_C1 hostDst(devDst.size());
  devDst.copyTo(hostDst.data(), hostDst.pitch());
  saveImage(outputPath, hostDst);
  nppiFree(devSrc.data());
  nppiFree(devDst.data());
  return true;
}

static void printUsage(const char *prog) {
  printf("\nUsage:\n");
  printf("  %s --input=<dir> --output=<dir> [--filter=<blur|sobel|all>]\n", prog);
  printf("\nOptions:\n");
  printf("  --input=<dir>    Directory containing .pgm grayscale images\n");
  printf("  --output=<dir>   Directory to write processed images\n");
  printf("  --filter=<mode>  Processing mode: blur, sobel (default), or all\n");
  printf("\nExample:\n");
  printf("  %s --input=data/input --output=data/output --filter=all\n\n", prog);
}

int main(int argc, char *argv[]) {
  printf("\n=== GPU Batch Image Processor ===\n\n");

  findCudaDevice(argc, (const char **)argv);
  printDeviceInfo();

  if (checkCmdLineFlag(argc, (const char **)argv, "help")) {
    printUsage(argv[0]);
    return EXIT_SUCCESS;
  }

  // Parse arguments
  char *inputArg = nullptr;
  char *outputArg = nullptr;
  char *filterArg = nullptr;

  if (checkCmdLineFlag(argc, (const char **)argv, "input")) {
    getCmdLineArgumentString(argc, (const char **)argv, "input", &inputArg);
  }
  if (checkCmdLineFlag(argc, (const char **)argv, "output")) {
    getCmdLineArgumentString(argc, (const char **)argv, "output", &outputArg);
  }
  if (checkCmdLineFlag(argc, (const char **)argv, "filter")) {
    getCmdLineArgumentString(argc, (const char **)argv, "filter", &filterArg);
  }

  std::string inputDir  = inputArg  ? inputArg  : "data/input";
  std::string outputDir = outputArg ? outputArg : "data/output";
  std::string filter    = filterArg ? filterArg : "sobel";

  printf("Input dir : %s\n", inputDir.c_str());
  printf("Output dir: %s\n", outputDir.c_str());
  printf("Filter    : %s\n\n", filter.c_str());

  std::vector<std::string> files = collectPgmFiles(inputDir);
  if (files.empty()) {
    std::cerr << "No .pgm files found in: " << inputDir << std::endl;
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  printf("Found %zu image(s) to process.\n\n", files.size());

  // Create output directory if not present
  std::string mkdirCmd = "mkdir -p " + outputDir;
  system(mkdirCmd.c_str());

  int processed = 0;
  int failed = 0;
  auto t0 = std::chrono::high_resolution_clock::now();

  for (const std::string &inPath : files) {
    // Build output filename
    std::string basename = inPath.substr(inPath.rfind('/') + 1);
    std::string stem = basename.substr(0, basename.rfind('.'));
    std::string outPath = outputDir + "/" + stem + "_" + filter + ".pgm";

    try {
      processImage(inPath, outPath, filter);
      printf("[OK] %s -> %s\n", basename.c_str(), outPath.c_str());
      processed++;
    } catch (npp::Exception &e) {
      std::cerr << "[FAIL] " << basename << ": " << e << std::endl;
      failed++;
    } catch (...) {
      std::cerr << "[FAIL] " << basename << ": unknown error" << std::endl;
      failed++;
    }
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(t1 - t0).count();

  printf("\n=== Summary ===\n");
  printf("Processed : %d\n", processed);
  printf("Failed    : %d\n", failed);
  printf("Total time: %.3f seconds\n", elapsed);
  printf("Avg/image : %.3f ms\n", processed > 0 ? (elapsed / processed) * 1000.0 : 0.0);

  return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
