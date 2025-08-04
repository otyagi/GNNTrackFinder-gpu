# Create a library called "libCbmOnlineData" which includes the source files given in
# the array .
# The extension is already found.  Any number of sources could be listed here.

set(OFFLINE_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../core/data)

set(SRCS
  ${OFFLINE_DATA_DIR}/CbmDefs.cxx
  ${OFFLINE_DATA_DIR}/CbmAddress.cxx

  ${OFFLINE_DATA_DIR}/bmon/CbmBmonDigi.cxx

  ${OFFLINE_DATA_DIR}/sts/CbmStsAddress.cxx
  ${OFFLINE_DATA_DIR}/sts/CbmStsDigi.cxx

  ${OFFLINE_DATA_DIR}/rich/CbmRichDigi.cxx

  ${OFFLINE_DATA_DIR}/much/CbmMuchDigi.cxx
  ${OFFLINE_DATA_DIR}/much/CbmMuchAddress.cxx

  ${OFFLINE_DATA_DIR}/trd/CbmTrdDigi.cxx
  ${OFFLINE_DATA_DIR}/trd/CbmTrdRawMessageSpadic.cxx

  ${OFFLINE_DATA_DIR}/tof/CbmTofDigi.cxx
  ${OFFLINE_DATA_DIR}/tof/CbmTofAddress.cxx
  ${OFFLINE_DATA_DIR}/tof/CbmTofDetectorId.cxx
  ${OFFLINE_DATA_DIR}/tof/CbmTofDetectorId_v12b.cxx

  ${OFFLINE_DATA_DIR}/psd/CbmPsdDigi.cxx
  ${OFFLINE_DATA_DIR}/psd/CbmPsdAddress.cxx

  ${OFFLINE_DATA_DIR}/fsd/CbmFsdDigi.cxx
  ${OFFLINE_DATA_DIR}/fsd/CbmFsdAddress.cxx


  ${OFFLINE_DATA_DIR}/raw/CriGet4Mess001.cxx
  ${OFFLINE_DATA_DIR}/raw/StsXyterMessage.cxx
)

add_library(OnlineData SHARED ${SRCS})

target_include_directories(OnlineData
  PUBLIC ${OFFLINE_DATA_DIR}
  PUBLIC ${OFFLINE_DATA_DIR}/base
  PUBLIC ${OFFLINE_DATA_DIR}/bmon
  PUBLIC ${OFFLINE_DATA_DIR}/sts
  PUBLIC ${OFFLINE_DATA_DIR}/rich
  PUBLIC ${OFFLINE_DATA_DIR}/much
  PUBLIC ${OFFLINE_DATA_DIR}/trd
  PUBLIC ${OFFLINE_DATA_DIR}/tof
  PUBLIC ${OFFLINE_DATA_DIR}/psd
  PUBLIC ${OFFLINE_DATA_DIR}/fsd
  PUBLIC ${OFFLINE_DATA_DIR}/global
  PUBLIC ${OFFLINE_DATA_DIR}/raw
)

target_compile_definitions(OnlineData PUBLIC NO_ROOT)
target_link_libraries(OnlineData
                      OnlineDataLog
                      external::fles_ipc
                      xpu
                      Boost::serialization
                      external::fles_logging
                     )

install(TARGETS OnlineData DESTINATION lib)
