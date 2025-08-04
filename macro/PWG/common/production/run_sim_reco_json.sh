#!/bin/bash

# Colored printout
export RED='\033[1;31m'
export GREEN='\033[1;32m'
export CYAN='\033[1;36m'
export NC='\033[0m'
export BOLD='\033[1m'

function getTraList () {
  traFiles=$(getJsonVal "['${step}']['traFiles']")
  traList="traFiles.list"
  for key in ${traFiles}
  do
    echo "${key}" >> "${traList}"
  done
  sed -i'' -e 's/\[//g' ${traList}
  sed -i'' -e 's/\]//g' ${traList}
  sed -i'' -e 's/,//g' ${traList}
  echo ${traList}
}

steps="transport digitization reconstruction AT"
source ${cbmRoot}
for step in ${steps}; do
  readStepInfo
  start=`date +%s`
  if [ ${run} == true ]; then
    echo "Maschine : $(uname -n)"
    echo " "
    echo " "
    echo -e "${CYAN} ##### Starting step : ${step} ##### ${NC}"
    export taskId=${SLURM_ARRAY_TASK_ID}
    plutoShift=$(getJsonVal "['accessory']['transport']['plutoShift']")
    export plutoFileId=$(printf %05d $((${taskId}-${plutoShift})))
    #    export plutoFileId=$(printf $((${taskId}-${plutoShift})))
    config=${srcDir}/${configName}
    macro=${srcDir}/${macroName}
    outFile=$(getJsonVal "['${step}']['output']['path']")
    outDir=$(dirname ${outFile})
    log=${outDir}/${step}.log

    mkdir -pv ${outDir}
    cd ${outDir}
    ln -sfv ${VMCWORKDIR}/macro/run/.rootrc ${outDir}
    if [ ${step} == reconstruction ]; then
      getTraList
      rawFile=$(getJsonVal "['reconstruction']['rawFile']")
      nTimeSlices=$(getJsonVal "['reconstruction']['nTimeSlices']")
      firstTimeSlice=$(getJsonVal "['reconstruction']['firstTimeSlice']")
      sEvBuildRaw=$(getJsonVal "['reconstruction']['sEvBuildRaw']")
      isL1Matching=$(getJsonVal "['reconstruction']['isL1Matching']")
      isL1EffQA=$(getJsonVal "['reconstruction']['isL1EffQA']")
      echo "  "
      echo -e "${BOLD} Run reconstruction: ${macro}(\"${rawFile}\",${nTimeSlices},${firstTimeSlice},\"${outFile}\",\
	         ${overwrite},\"${sEvBuildRaw}\",\"${config}\",\"${traList}\",${isL1Matching},${isL1EffQA}) ${NC}"
      root -b -l -q "${macro}(\"${rawFile}\",${nTimeSlices},${firstTimeSlice},\"${outFile}\",\
           ${overwrite},\"${sEvBuildRaw}\",\"${config}\",\"${traList}\",${isL1Matching},${isL1EffQA})" &>${log}
      if [ $? -eq 0 ]; then
        echo -e "${GREEN} Reconstruction executed successfully ${NC}"
      else
        echo -e "${RED} Reconstruction failed ${NC}"
        exit 1
      fi
      rm -v ${traList}
    elif [ ${step} == AT ]; then
      getTraList
      rawFile=$(getJsonVal "['AT']['rawFile']")
      recFile=$(getJsonVal "['AT']['recFile']")
      unigenFile=$(getJsonVal "['AT']['unigenFile']")
      eventMode=$(getJsonVal "['digitization']['eventMode']")
      tslength=-1
      if [ ${eventMode} == false ]; then
        tslength=$(getJsonVal "['digitization']['timeSliceLength']")
      fi
      echo "  "
      echo -e "${BOLD} Run AT converter: ${macro}(\"${traList}\",\"${rawFile}\",\"${recFile}\",\
	         \"${unigenFile}\",\"${outFile}\",${overwrite},\"${config}\",\"${tslength}\") ${NC}"
      root -b -l -q "${macro}(\"${traList}\",\"${rawFile}\",\"${recFile}\",\
	         \"${unigenFile}\",\"${outFile}\",${overwrite},\"${config}\",\"${tslength}\")" &>${log}
      if [ $? -eq 0 ]; then
        echo -e "${GREEN} AnalysisTreeConverter executed successfully ${NC}"
      else
        echo -e "${RED} AnalysisTreeConverter failed ${NC}"
        exit 1
      fi
      rm -v ${traList}
    elif [ ${step} == digitization ]; then
      input=$(getJsonVal "['transport']['output']['path']")
      if [ ! -e ${outFile}.par.root ] || [ ${overwrite} == true ]; then
        echo "Copying parameter file..."
        cp -v ${input}.par.root ${outDir}
      fi
      echo "  "
      echo -e "${BOLD} Run digitization: ${macro}(\"${config}\",${nEvents}) ${NC}"
      root -b -l -q "${macro}(\"${config}\",${nEvents})" &>${log}
      if [ $? -eq 0 ]; then
        echo -e "${GREEN} ${step} executed successfully ${NC}"
      else
        echo -e "${RED} ${step} failed ${NC}"
        exit 1
      fi
    elif [ ${step} == transport ]; then
      seed=$(expr $(getJsonVal "['accessory']['transport']['seed']"))
      echo "  "
      echo -e "${BOLD} Run transport: ${macro}(\"${config}\",${nEvents},${seed}) ${NC}"

      root -b -l -q "${macro}(\"${config}\",${nEvents},${seed})" &>${log}

      if [ $? -eq 0 ]; then
        echo -e "${GREEN} ${step} executed successfully ${NC}"
      else
        echo -e "${RED} ${step} failed ${NC}"
        exit 1
      fi
    fi

    gzip -f ${log}

    rm -v .rootrc
    if [ ${step} == reconstruction ]; then
      if [ "${isL1EffQA}" == true ]; then
        rm -v *{core,moni,CA,Fair}* all_*.par
      else
        rm -v *{core,moni,CA,L1,Edep}* all_*.par
      fi
    fi
    if [ ${step} == digitization ]; then
      rm -v *{moni,Fair}* all_*.par
    fi
    if [ ${step} == AT ]; then
      rm -v *{core,CA,L1}*
    fi

    echo " "
    end=$(date +%s)
    runtime=$((end - start))
    echo "Runtime : ${runtime}s"
    echo " "

    cd -
    export taskId=
  fi
done
