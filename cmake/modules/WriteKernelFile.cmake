################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################
 MACRO (WRITE_KERNEL_FILE filename)

 #MESSAGE("
 #  Python3_FOUND=${Python3_FOUND}
 #  Python3_Interpreter_FOUND=${Python3_Interpreter_FOUND}
 #  Python3_EXECUTABLE=${Python3_EXECUTABLE}
 #  "
 #)

 String(REGEX REPLACE "^.*(install).*$" "\\1" INSTALL_VERSION ${filename})
 String(COMPARE EQUAL "install" "${INSTALL_VERSION}" INSTALL_TRUE)

 List(REMOVE_ITEM LD_LIBRARY_PATH ${CMAKE_BINARY_DIR}/lib)

 If (INSTALL_TRUE)
   SET(_INSTALLDIR ${CMAKE_INSTALL_PREFIX})
   SET(_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
   #SET(VMCWORKDIR ${CMAKE_INSTALL_PREFIX}/share/fairbase)
   SET(FAIRLIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
   SET(_LD_LIBRARY_PATH ${FAIRLIBDIR} ${LD_LIBRARY_PATH})
 Else (INSTALL_TRUE)
   SET(_INSTALLDIR ${CMAKE_BINARY_DIR})
   SET(_BINDIR ${CMAKE_BINARY_DIR})
   #SET(VMCWORKDIR ${CMAKE_SOURCE_DIR})
   SET(FAIRLIBDIR ${CMAKE_BINARY_DIR}/lib)
   SET(_LD_LIBRARY_PATH ${FAIRLIBDIR} ${LD_LIBRARY_PATH})
 EndIf (INSTALL_TRUE)

 If(NOT DEFINED FULL_CONFIG_FILE)
   Set(FULL_CONFIG_FILE "true")
 EndIf(NOT DEFINED FULL_CONFIG_FILE)


 IF(CMAKE_SYSTEM_NAME MATCHES Linux)
   IF(FAIRROOTPATH)
     configure_file(${FAIRROOTPATH}/share/fairbase/cmake/scripts/check_system.sh.in
                    ${CMAKE_CURRENT_BINARY_DIR}/check_system.sh
                   )
     configure_file(${FAIRROOTPATH}/share/fairbase/cmake/scripts/check_system.csh.in
                    ${CMAKE_CURRENT_BINARY_DIR}/check_system.csh
                   )
   ELSE(FAIRROOTPATH)
     configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/check_system.sh.in
                    ${CMAKE_CURRENT_BINARY_DIR}/check_system.sh
                   )
     configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/check_system.csh.in
                    ${CMAKE_CURRENT_BINARY_DIR}/check_system.csh
                   )
   ENDIF(FAIRROOTPATH)

   EXECUTE_PROCESS(COMMAND lsb_release -sd
                    OUTPUT_VARIABLE _linux_flavour
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                   )

   IF(_linux_flavour)
     STRING(REGEX REPLACE "^\"" "" _linux_flavour ${_linux_flavour})
     STRING(REGEX REPLACE "\"$" "" _linux_flavour ${_linux_flavour})
   ENDIF(_linux_flavour)

   EXECUTE_PROCESS(COMMAND uname -m
                   OUTPUT_VARIABLE _system
                   OUTPUT_STRIP_TRAILING_WHITESPACE
                  )

 ElseIf(CMAKE_SYSTEM_NAME MATCHES Darwin)
   IF(FAIRROOTPATH)
     configure_file(${FAIRROOTPATH}/share/fairbase/cmake/scripts/check_system_mac.sh.in
                    ${CMAKE_CURRENT_BINARY_DIR}/check_system.sh
                   )
     configure_file(${FAIRROOTPATH}/share/fairbase/cmake/scripts/check_system_mac.csh.in
                    ${CMAKE_CURRENT_BINARY_DIR}/check_system.csh
                   )
   ELSE(FAIRROOTPATH)
     configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/check_system_mac.sh.in
                    ${CMAKE_CURRENT_BINARY_DIR}/check_system.sh
                   )
     configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/check_system_mac.csh.in
                    ${CMAKE_CURRENT_BINARY_DIR}/check_system.csh
                   )
   ENDIF(FAIRROOTPATH)
   EXECUTE_PROCESS(COMMAND uname -sr
                   OUTPUT_VARIABLE _linux_flavour
                   OUTPUT_STRIP_TRAILING_WHITESPACE
                  )
   EXECUTE_PROCESS(COMMAND uname -m
                   OUTPUT_VARIABLE _system
                   OUTPUT_STRIP_TRAILING_WHITESPACE
                  )
 ENDIF(CMAKE_SYSTEM_NAME MATCHES Linux)


 CONVERT_LIST_TO_STRING(${Geant4_INCLUDE_DIRS})
 Set(Geant4_INCLUDE_DIRS ${output})

 CONVERT_LIST_TO_STRING(${Geant4VMC_INCLUDE_DIRS})
 Set(Geant4VMC_INCLUDE_DIRS ${output})

 #CONVERT_LIST_TO_STRING(${Geant4VMC_CMAKE_INSTALL_LIBDIR})
 #Set(Geant4VMC_LIBRARY_DIR ${output})

 CONVERT_LIST_TO_STRING(${Geant4VMC_MACRO_DIR})
 Set(Geant4VMC_MACRO_DIR ${output})

 Write_Geant4Data_Variables_jupy()

 CONVERT_LIST_TO_STRING(${PYTHIA6_LIBRARY_DIR})
 Set(PYTHIA6_LIBRARY_DIR ${output})

 CONVERT_LIST_TO_STRING(${Geant3_SYSTEM_DIR})
 Set(G3SYS ${output})

 CONVERT_LIST_TO_STRING(${Geant3_INCLUDE_DIRS})
 Set(Geant3_INCLUDE_DIRS ${output})

 CONVERT_LIST_TO_STRING(${Geant3_LIBRARY_DIR})
 Set(Geant3_LIBRARY_DIR ${output})

 CONVERT_LIST_TO_STRING(${Geant3_LIBRARIES})
 Set(Geant3_LIBRARIES ${output})

 CONVERT_LIST_TO_STRING(${ROOT_LIBRARY_DIR})
 Set(ROOT_LIBRARY_DIR ${output})

 CONVERT_LIST_TO_STRING(${ROOT_LIBRARIES})
 Set(ROOT_LIBRARIES ${output})

 CONVERT_LIST_TO_STRING(${ROOT_INCLUDE_DIR})
 Set(ROOT_INCLUDE_DIR ${output} )

 #Set(VMCWORKDIR ${C})

 Set(FAIRLIBDIR ${FAIRLIBDIR})

 List(APPEND _LD_LIBRARY_PATH ${PYTHIA6_LIBRARY_DIR} ${Geant3_LIBRARY_DIR} ${Geant4VMC_LIBRARY_DIR})

 List(REMOVE_DUPLICATES _LD_LIBRARY_PATH)
 CONVERT_LIST_TO_STRING(${_LD_LIBRARY_PATH})

 IF(CMAKE_SYSTEM_NAME MATCHES Linux)
   Set(MY_LD_LIBRARY_PATH ${output})
 ELSE(CMAKE_SYSTEM_NAME MATCHES Linux)
   IF(CMAKE_SYSTEM_NAME MATCHES Darwin)
     Set(MY_DYLD_LIBRARY_PATH ${output})
   ENDIF(CMAKE_SYSTEM_NAME MATCHES Darwin)
 ENDIF(CMAKE_SYSTEM_NAME MATCHES Linux)

 Set(USE_VGM 1)

 SET(PYTHONPATH ${CMAKE_SOURCE_DIR}/python ${FAIRLIBDIR} ${SIMPATH}/lib ${SIMPATH}/lib/root ${SIMPATH}/lib/Geant4 ${SIMPATH}/lib/g4py ${PYTHONPATH})
 UNIQUE(PYTHONPATH "${PYTHONPATH}")
 CONVERT_LIST_TO_STRING(${PYTHONPATH})
 SET(MY_PYTHONPATH ${output})

 SET (PATH ${ROOTSYS}/bin ${PATH})
 UNIQUE(PATH "${PATH}")
 CONVERT_LIST_TO_STRING(${PATH})
 Set(MY_PATH ${output})

 # If(FAIRSOFT_EXTERN)
   # Set(PYTHIA8DATA "${SIMPATH}/share/pythia8/xmldoc")
 # Else(FAIRSOFT_EXTERN)
   # Set(PYTHIA8DATA "${SIMPATH}/generators/pythia8/xmldoc")
 # EndIf(FAIRSOFT_EXTERN)

 CONVERT_LIST_TO_STRING(${ROOT_INCLUDE_PATH})
 Set(ROOT_INCLUDE_PATH ${output})

 IF(${filename} MATCHES "[.]json.*$")
   MESSAGE("Configuring Jupyter kernelspec file ${filename}")
   String(REPLACE "_install" "" KERNELFILE ${filename})
   configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/jupyter/${KERNELFILE}.in
                ${CMAKE_CURRENT_BINARY_DIR}/share/jupyter/kernels/${filename}
                 )
   get_filename_component(KERNELSUBPATH ${KERNELFILE} DIRECTORY)
   file(COPY ${CMAKE_SOURCE_DIR}/cmake/scripts/jupyter/logo-64x64.png
        DESTINATION share/jupyter/kernels/${KERNELSUBPATH}
       )

   IF(${filename} MATCHES "install$")
     install( FILES       ${CMAKE_BINARY_DIR}/share/jupyter/kernels/${filename}
              DESTINATION share/jupyter/kernels
              RENAME ${KERNELFILE}
            )
     install( FILES       ${CMAKE_SOURCE_DIR}/cmake/scripts/jupyter/logo-64x64.png
              DESTINATION share/jupyter/kernels/${KERNELSUBPATH}
            )
   ENDIF(${filename} MATCHES "install$")
              
 ENDIF(${filename} MATCHES "[.]json.*$")

ENDMACRO (WRITE_KERNEL_FILE)
