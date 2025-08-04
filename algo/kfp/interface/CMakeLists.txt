if(NOT CBM_ONLINE_STANDALONE)
  ### CbmKFParticleOnlineInterface
  # Just reusing the KFParticle library, compiled in the external
  add_library(CbmKFParticleOnlineInterface INTERFACE)
  target_include_directories(CbmKFParticleOnlineInterface INTERFACE)
  target_compile_definitions(CbmKFParticleOnlineInterface 
    INTERFACE DO_TPCCATRACKER_EFF_PERFORMANCE NonhomogeneousField CBM USE_TIMERS)
  target_link_libraries(CbmKFParticleOnlineInterface INTERFACE KFParticle)
  install(TARGETS CbmKFParticleOnlineInterface DESTINATION lib)
else()
  # Creating a replacement of the KFParticle library for a standalone mode
  set(KFP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../external/KFParticle/KFParticle)

  set(SRCS
    ${KFP_SOURCE_DIR}/KFParticle.cxx
    ${KFP_SOURCE_DIR}/KFParticleBase.cxx
    ${KFP_SOURCE_DIR}/KFParticleBaseSIMD.cxx
    ${KFP_SOURCE_DIR}/KFParticleDatabase.cxx
    ${KFP_SOURCE_DIR}/KFParticleFinder.cxx
    ${KFP_SOURCE_DIR}/KFParticlePVReconstructor.cxx
    ${KFP_SOURCE_DIR}/KFParticleSIMD.cxx
    ${KFP_SOURCE_DIR}/KFParticleTopoReconstructor.cxx
    ${KFP_SOURCE_DIR}/KFPEmcCluster.cxx
    ${KFP_SOURCE_DIR}/KFPTrack.cxx
    ${KFP_SOURCE_DIR}/KFPTrackVector.cxx
    ${KFP_SOURCE_DIR}/KFPVertex.cxx
    ${KFP_SOURCE_DIR}/KFVertex.cxx
  )
  
  add_library(CbmKFParticleOnlineInterface SHARED ${SRCS})
  target_include_directories(CbmKFParticleOnlineInterface PUBLIC ${KFP_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(CbmKFParticleOnlineInterface PUBLIC NonhomogeneousField CBM CBM_ONLINE)
  target_link_libraries(CbmKFParticleOnlineInterface 
                 PUBLIC Vc::Vc 
                        ROOT::Core
                        )
  install(TARGETS CbmKFParticleOnlineInterface DESTINATION lib)
endif()
