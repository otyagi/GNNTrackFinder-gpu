# CMakeList file for library libCbmRecoOfflineSteer
# V. Friese,     13 May 2023


set(SRCS
  Config.cxx
  Run.cxx
  TaskFactory.cxx
  )

set(INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})

if(OpenMP_CXX_FOUND)
  list(APPEND DEPENDENCIES OpenMP::OpenMP_CXX)
endif()


set(LIBRARY_NAME CbmRecoOfflineSteer)
set(LINKDEF RootLinkDef.h)
set(PUBLIC_DEPENDENCIES
  AlgoOffline
  CbmData
  FairRoot::Base
  ROOT::Core
  external::yaml-cpp
  CbmEventBuilder
  )

set(PRIVATE_DEPENDENCIES
  CbmBase
  CbmTrdBase
  FairLogger::FairLogger
  FairRoot::Online
  ROOT::Hist
  ROOT::RHTTP
  CbmGlobal
  CbmRichBase
  CbmTofBase
  CbmFsdBase
  CbmPsdBase
  CbmMuchReco
  CbmMvdReco
  CbmRichReco
  CbmRecoSts
  CbmTofReco
  CbmTrdReco
  CbmFsdReco
  CbmPsdReco
  CbmSimSteer
  L1
  KF
  )

# Check if the compiler supports std::execution in the respective STL
# library
CHECK_CXX_SOURCE_COMPILES("
#include <numeric>
#include <vector>
#include <execution>

int main(int argc, char *argv[])
{
    std::vector<double> v(10, 1);

    auto result = std::reduce(std::execution::par, v.begin(), v.end());
    return 0;
}" HAS_STD_EXECUTION)

if (HAS_STD_EXECUTION)
  message("Execution is available in STL")
  add_definitions(-DWITH_EXECUTION)
endif()

generate_cbm_library()
