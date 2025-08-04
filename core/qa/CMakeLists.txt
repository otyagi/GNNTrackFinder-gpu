set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/checker
  ${CMAKE_CURRENT_SOURCE_DIR}/report
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

set(SRCS
  CbmQaOnlineInterface.cxx
  CbmQaCanvas.cxx
  CbmQaManager.cxx
  CbmQaEff.cxx
  CbmQaPie.cxx
  CbmQaHist.cxx
  CbmQaTable.cxx
  CbmQaTask.cxx
  CbmQaIO.cxx
  CbmQaUtil.cxx
  CbmQaCompare.cxx
  checker/CbmQaCheckerCore.cxx
  checker/CbmQaCheckerFileHandler.cxx
  checker/CbmQaCheckerHist1DHandler.cxx
  checker/CbmQaCheckerHist2DHandler.cxx
  checker/CbmQaCheckerObjectHandler.cxx
  checker/CbmQaCheckerProfile1DHandler.cxx
  checker/CbmQaCheckerObjectDB.cxx
  report/CbmQaReportBuilder.cxx
  report/CbmQaReportBeamerEngine.cxx
  report/CbmQaReportLatexEngine.cxx
  report/CbmQaReportLatexFormat.cxx
  report/CbmQaReportHtmlEngine.cxx
  report/CbmQaReportSection.cxx
  report/CbmQaReportTable.cxx
  )

set(HEADERS
  CbmQaConstants.h
  report/CbmQaReportDefines.h
  report/CbmQaReportElement.h
  report/CbmQaReportEngine.h
  report/CbmQaReportFigure.h
  report/CbmQaReportHeader.h
  report/CbmQaReportTail.h
  )

set(LIBRARY_NAME CbmQaBase)
set(LINKDEF ${LIBRARY_NAME}LinkDef.h)
set(PUBLIC_DEPENDENCIES
  AlgoOffline
  FairRoot::Base
  FairLogger::FairLogger
  external::yaml-cpp
  ROOT::Core
  ROOT::Gpad
  ROOT::Hist
  Boost::filesystem
  fmt::fmt
  )

set(PRIVATE_DEPENDENCIES
  CbmData
  external::yaml-cpp
  )

set(INTERFACE_DEPENDENCIES
  ROOT::Graf
  )

generate_cbm_library()

Install(FILES
        CbmQaOnlineInterface.h
        CbmQaCompare.h
        CbmQaManager.h
        CbmQaTask.h
        CbmQaCanvas.h
        CbmQaTable.h
        CbmQaHist.h
        CbmQaEff.h
        CbmQaPie.h
        CbmQaUtil.h
        CbmQaConstants.h
        CbmQaCmpDrawer.h
        checker/CbmQaCheckerTypedefs.h

        report/CbmQaReportDefines.h
        report/CbmQaReportBuilder.h
        report/CbmQaReportLatexEngine.h
        report/CbmQaReportBeamerEngine.h
        report/CbmQaReportHtmlEngine.h
        report/CbmQaReportSection.h
        report/CbmQaReportTable.h
        report/CbmQaReportElement.h
        report/CbmQaReportEngine.h
        report/CbmQaReportFigure.h
        report/CbmQaReportHeader.h
        report/CbmQaReportTail.h
        DESTINATION include
        )

