#!/bin/bash
# Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# First commited by Florian Uhlig

# shell script to apply clusterizer calibrations
#SBATCH -J trk_cal_digi
#SBATCH -D /lustre/cbm/users/nh/CBM/cbmroot/trunk/macro/beamtime/mcbm2021
##SBATCH --time=8:00:00
#SBATCH --time=5-24:00:00
#SBATCH --mem=4000
#SBATCH --partition=long

trk_cal_digi() {
cRun=$1

iCalSet=$2
((iTmp  = $iCalSet ))
((iBRef = $iTmp % 1000))
((iTmp  = $iTmp - $iBRef))
((iSet  = $iTmp / 1000))
((iRef  = $iTmp % 1000000))
((iRef  = $iRef / 1000))
((iTmp  = $iTmp - $iRef))
((iDut  = $iTmp / 1000000))

iSel2=$3
cSel2=$iSel2
if (( iSel2<100 )); then 
cSel2="0"$iSel2
fi
if (( iSel2<10 )); then 
cSel2="00"$iSel2
fi
if (( iSel2<0 )); then 
cSel2="-01"
fi

cCalSet=$iCalSet
if (( iCalSet<100000000 )); then 
cCalSet="0"$iCalSet
fi
if (( iCalSet<10000000 )); then 
cCalSet="00"$iCalSet
fi
if (( iCalSet<1000000 )); then 
cCalSet="000"$iCalSet
fi
if (( iCalSet<100000 )); then 
cCalSet="0000"$iCalSet
fi
echo cCalSet = $cCalSet

Deadtime=$4
if [[ ${Deadtime} = "" ]]; then
Deadtime=50.
fi

CalIdMode=$5
if [[ ${CalIdMode} = "" ]]; then
 echo use native calibration file 
 CalIdMode=${cRun}
 CalFile=${cRun}_set${cCalSet}_93_1tofClust.hst.root
else 
 CalFile=${CalIdMode}_set${cCalSet}_93_1tofClust.hst.root
 RunFile=${cRun}_set${cCalSet}_93_1tofClust.hst.root
# rm ${RunFile}
# ln -s ${CalFile} ${RunFile} 
 echo use calibrations from  ${CalFile}
fi

iCalOpt=$6
if [[ ${iCalOpt} = "" ]]; then
  iCalOpt=1	
fi

iTraSetup=$7
if [[ $iTraSetup = "" ]]; then 
  iTraSetup=1
fi

iTrkPar=$8
if [[ $iTrkPar = "" ]]; then 
# fixed parameters, to be edited if necessary
#iTrkPar=0 # full 2021 setup
#iTrkPar=1  # for double stack analysis 
iTrkPar=2  # for mcbm cosmic 
#iTrkPar=3 # for CRI data ul 2021
fi

CalIdSet=$9
if [[ ${CalIdSet} = "" ]]; then
    echo use native calibration file
    CalIdSet=$cCalSet
else
    CalFile=${CalIdMode}_set${CalIdSet}_93_1tofClust.hst.root    
fi

echo trk_cal_digi for $cRun with iDut=$iDut, iRef=$iRef, iSet=$iCalSet, iSel2=$iSel2, iBRef=$iBRef, Deadtime=$Deadtime, CalFile=$CalFile, TrkPar=$iTrkPar

if [ -e /lustre/cbm ]; then
source /lustre/cbm/users/nh/CBM/cbmroot/trunk/build/config.sh -a
wdir=/lustre/cbm/users/nh/CBM/cbmroot/trunk/macro/beamtime/mcbm2021
outdir=/lustre/cbm/users/nh/CBM/cbmroot/trunk/macro/beamtime/mcbm2021/${cRun}
else 
wdir=`pwd`
outdir=${wdir}/${cRun}
fi

iter=0;

cd $wdir
mkdir $cRun
cd    $cRun 
cp    ../.rootrc .
cp    ../rootlogon.C .

echo Execute in `pwd` at shell level $iShLev: ./trk_cal_digi.sh $1 $2 $3 $4 $5 $6 $7 $8

if [[ $iShLev = "" ]]; then 
  iShLev=0
  nEvt=500000 
  dDTres=100000
  dDTRMSres=100000
  dL0DTRMSres=100000
# get initial digi calibration 
# cp -v  ./I*/${CalFile}  .
  cp -v ${CalFile}  IniTrk_${CalFile}
# get latest tracker offsets
# cp -v ../${cRun}_tofFindTracks.hst.root .
else
 (( iShLev += 1 ))  
fi 

rm -v TCalib.res
nEvtMax=0
(( nEvtMax = nEvt*10 ))

#frange1 limits DT spectrum range 
fRange1=1.5
# frange2 limits chi2
fRange2=6.0      # 9.
TRange2Limit=4.0 # 2.

iSel=12022
iGenCor=3
cCalSet2=${cCalSet}_$cSel2

case $iCalOpt in
  1)   # TOff
  ;;
  2)   # Walk
  (( nEvt *= 10 ))
  (( nEvtMax *= 10 ))
  ;;
esac
  
iIter=0
while [[ $dDTres -gt 0 ]]; do
  nEvt=`echo "scale=0;$nEvt * 1./1." | bc`
  #nEvt=`echo "scale=0;$nEvt * 1.1/1." | bc`
  if [ $nEvt -gt $nEvtMax ]; then
    nEvt=$nEvtMax
  fi

  fRange2=`echo "$fRange2 * 0.9" | bc`
  compare_TRange2=`echo "$fRange2 < $TRange2Limit" | bc`
  if  [[ $compare_TRange2 > 0 ]]; then
   fRange2=$TRange2Limit
  fi
  
  iCalAct=$iCalOpt
  echo Enter while loop with Iter $iIter, CalAct $iCalAct, CalOpt $iCalOpt in dir `pwd`

  while [[ $iCalAct -gt 0 ]]; do  
    cd $wdir/$cRun
    echo Current loop with Iter $iIter, CalAct $iCalAct and CalOpt $iCalOpt
    if [[ $iCalOpt = 1 ]] || [[ $iCalAct -gt 1 ]]; then 
      if [[ $iCalOpt > 1 ]]; then  
        echo Execute ./clu_cal_all.sh $cRun $iCalSet   # update local calibration
        cd $wdir
        ./clu_cal_all.sh $cRun $iCalSet               # update local calibration
        cd $wdir/$cRun
      fi
      root -b -q '../ana_digi_cal_all.C('$nEvt',93,-2,-1,1,"'$cRun'",'$iCalSet',1,'$iSel2','$Deadtime',"'$CalIdMode'")'
      # update calibration parameter file, will only be active in next iteration 
      if [[ $iIter -gt -1 ]] && [[ $iCalOpt = 1 ]]; then  # exploratory option when iIter set to 0 
        echo Update Calibration file from ana_digi_cal with Cluster corrections at Iter $iIter
        cp -v tofClust_${cRun}_set${cCalSet}.hst.root ../${cRun}_set${cCalSet}_93_1tofClust.hst.root
        root -b -q '../ana_digi_cal_all.C('$nEvt',93,-2,-1,1,"'$cRun'",'$iCalSet',1,'$iSel2','$Deadtime',"'$CalIdMode'")'
        #echo 20000 > TOffAvOff.res
        #echo 20000 > TOffAvRMS.res
      fi
      root -b -q '../ana_trks.C('$nEvt','$iSel','$iGenCor',"'$cRun'","'$cCalSet2'",'$iSel2','$iTraSetup','$fRange1','$fRange2','$Deadtime',"'$CalIdMode'",1,1,'$iCalSet','$iCalAct','$iTrkPar')'
      #root -l 'ana_trksi.C(-1,10,1,"385.50.5.0","000014500_020",20,1,1.90,7.60,50,"385.50.5.0",1,1)'
      #exit 0 # for debugging
      cp -v New_${CalFile} ${CalFile}  
      
      (( iIter   += 1 ))
    else 
      cd $wdir
      # store current status 
      dLDTres=$dDTres
      dLDTRMSres=$dDTRMSres
      iLCalOpt=$iCalOpt
      echo Store $iIter limits $dLDTres, $dLDTRMSres
      echo exec in `pwd` at iter $iIter, level $iShLev: trk_cal_digi $1 $2 $3 $Deadtime $CalIdMode 1 $7 $8
      trk_cal_digi $1 $2 $3 $Deadtime $CalIdMode 1 $7 $8
      # restore old status
      dL0DTRMSres=$dDTRMSres
      dLDTRMSres=50000  # prepare for next round 
      dDTres=$dLDTres
      dDTRMSres=$dLDTRMSres
      iCalOpt=$iLCalOpt
      echo exec1done at $iIter, $iShLev resume old CalOpt $iCalOpt with limits $dDTres, $dDTRMSres, $dL0DTRMSres
    fi
    (( iCalAct -= 1 ))
    echo Continue while loop with Iter $iIter, ShLev $iShLev, CalAct $iCalAct and CalOpt $iCalOpt
  done
  
  cd $wdir/$cRun
  Tres=`cat TOffAvOff.res`
  TRMSres=`cat TOffAvRMS.res`

  if [[ $Tres = 0 ]]; then
    Tres=1
  fi
  
  if [[ $dDTRMSres -eq 50000 ]]; then 
    TRMSres=5000
  fi
  
  dTdif=`echo "$dDTres - $Tres" | bc`
  compare_result=`echo "$Tres < $dDTres" | bc`

  dTRMSdif=`echo "$dDTRMSres - $TRMSres" | bc`
  compare_RMS=`echo "$TRMSres < $dDTRMSres" | bc`

  echo `date`: at iter=$iter, ShLev=$iShLev got TOff = $Tres, compare to $dDTres, dTdif = $dTdif, result = $compare_result, TRMS = $TRMSres, old $dDTRMSres, dif = $dTRMSdif, result = $compare_RMS 

  ((compare_result += $compare_RMS))
  echo CMPR result_summary: $compare_result 

#  if [ $iter = 1 ]; then 
#    exit 0  # for debugging 
#  fi

  if [[ $compare_result > 0 ]]; then
    if [[ $Tres = 0 ]]; then
      Tres=1
    fi
    dDTres=$Tres
    dDTRMSres=$TRMSres
    echo Stored $iIter, $iShLev new res values $dDTres, $dDTRMSres
    (( dDTRMSres -= 1 ))  # next attempt should be at least 1ps better for continuation
    cp -v New_${CalFile} ${CalFile}  
    cp -v New_${CalFile} ${CalFile}_$iter  
  else
    echo Next iteration $TRMSres -gt $dL0DTRMSres ?
    if [[ $TRMSres -gt $dL0DTRMSres ]]; then
      exit 0 
    fi
    dDTres=0
  fi
  (( iter += 1 ))
done

(( iShLev -= 1 ))
cd $wdir/$cRun
echo Finishing with ShLev $iShLev, Iter = $iIter 
# generate full statistics CalDigi / Hit file 
if [[ $iShLev -eq 0 ]]; then
#  root -b -q '../ana_digi_cal.C(-1,93,1,'$iRef',1,"'$cRun'",'$iCalSet',1,'$iSel2','$Deadtime',"'$CalIdMode'") '
  root -b -q '../ana_digi_cal_all.C(1000000,93,1,'$iRef',1,"'$cRun'",'$iCalSet',1,'$iSel2','$Deadtime',"'$CalIdMode'") '
  cd $wdir
  mv -v slurm-${SLURM_JOB_ID}.out ${outdir}/TrkCalDigi_${cRun}_${iCalSet}_${iSel2}_${CalIdMode}.out
fi

} #end of function body

trk_cal_digi $1 $2 $3 $4 $5 $6 $7 $8

cd $wdir
mv -v slurm-${SLURM_JOB_ID}.out ${outdir}/TrkCal_${cRun}_${iCalSet}_${iTraSetup}_${iTrkPar}.out
