
set(HAL_CBM
	${CMAKE_CURRENT_SOURCE_DIR}
)

set(HAL_DIR ${CBMROOT_SOURCE_DIR}/external/)
set(KF_HEADERS
	${CBMROOT_SOURCE_DIR}/L1
	${CBMROOT_SOURCE_DIR}/L1/L1Algo
	${CBMROOT_SOURCE_DIR}/L1/ParticleFinder
	${CLHEP_INCLUDE_DIR}
	${Boost_INCLUDE_DIRS}
	${VC_INCLUDE_DIRS}
	${KFParticle_INCLUDE_DIR}
	${CBMROOT_SOURCE_DIR}/KF
	${CBMROOT_SOURCE_DIR}/KF/Interface
	${CBMROOT_SOURCE_DIR}/external/KFParticle/KFParticle
)
set(HAL_CBM_FORMAT
	${CBMBASE_DIR}
	${CBMDATA_DIR}
	${CBMDATA_DIR}/global
	${CBMDATA_DIR}/sts
	${CBMDATA_DIR}/tof
	${CBMDATA_DIR}/trd
	${CBMDATA_DIR}/rich
	${CBMDATA_DIR}/much
	${CBMDATA_DIR}/mvd
	${HAL_CBM}/format
	${HAL_CBM}/format/combined
	${HAL_CBM}/format/anatree
	${HAL_CBM}/format/anatree/reco
	${HAL_CBM}/format/anatree/mc
	${HAL_CBM}/format/anatree/femto
	${HAL_CBM}/format/unigen
)

set(HAL_CBM_CUTS
    ${HAL_CBM}/cuts/pair
    ${HAL_CBM}/cuts/track
)
#set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
add_subdirectory(format)
add_subdirectory(cuts)
add_subdirectory(helpers)
add_subdirectory(qa)
