#!/bin/bash
# Copyright (C) 2020 PI-UHd,GSI
# SPDX-License-Identifier: GPL-3.0-only
# First commited by Norbert Herrmann

# shell script to initialize clusterizer calibrations
#SBATCH -J calall
#SBATCH -D /lustre/cbm/users/nh/CBM/cbmroot/trunk/macro/beamtime/hd2020
#SBATCH --time=6-00:00:00
#SBATCH --mem=2000
#SBATCH --partition=long
cRun=$1

echo 'Initialize clusterizer calibration for run '$cRun

iCalSet=$2
((iTmp  = $iCalSet ))
((iBRef = $iTmp % 1000))
((iTmp  = $iTmp - $iBRef))
((iSet  = $iTmp / 1000))
((iMRef = $iTmp % 1000000))
((iMRef = $iMRef / 1000))
((iTmp  = $iTmp - $iMRef))
((iDut  = $iTmp / 1000000))
echo Calib setup is ${iCalSet}, iSet=$iSet, iDut=$iDut, iMRef=$iMRef, iBRef=$iBRef
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
#iSet=0
#lastOpt=''
nEvi0=100000  # start value
nEvi1=10000    # increment 

if [ -e /lustre ]; then
source /lustre/cbm/users/nh/CBM/cbmroot/trunk/build6/config.sh 
wdir=/lustre/cbm/users/nh/CBM/cbmroot/trunk/macro/beamtime/hd2020
outdir=/lustre/cbm/users/nh/CBM/cbmroot/trunk/macro/beamtime/hd2020/${RunId}
else 
wdir=`pwd`
outdir=${wdir}/${RunId}
fi
mkdir ${outdir}

cd  ${wdir}
mkdir ${cRun}
cp rootlogon.C ${cRun}
cp .rootrc ${cRun}
cd ${cRun}

# Global variables, for for-loops
iRestart=0
#iRestart=12
iStep=0
iStepLast=0
iCalSel0=0
iCalSel1=1
# ************************** Starting while Loop ***************************** #

(( nEvi = nEvi0 + 10*nEvi1 ))
optList=""
optList=`echo " $nEvi,93,1,$iMRef,0 "`$optList 
icalmod=3
for icallev in 8  6  4  2  
do
    (( nEvi = nEvi0 + (icallev-1)*nEvi1 ))
    optList=`echo " $nEvi,$icallev$icalmod,$iCalSel0,$iDut,0 "`$optList
    optList=`echo " $nEvi,$icallev$icalmod,$iCalSel1,$iMRef,0 "`$optList
    optList=`echo " $nEvi,$icallev$icalmod,$iCalSel0,99,50 "`$optList 
    #optList=`echo " $nEvi,$icallev$icalmod,$iCalSel1,99,50 "`$optList
done
 optList=`echo " $nEvi,0,0,99,50 "`$optList      # start Init1
 echo optList:  $optList

for inOpt in $optList
do  
    echo step ${iStep} with option $inOpt
    ((iStepLast = ${iStep}))
    ((iStep += 1))

    mkdir Init${iStep}
    cp rootlogon.C Init${iStep}
    cp .rootrc Init${iStep}
    cd Init${iStep}

    if [[ ${lastOpt:+1} ]] ; then
	# echo last round was done with $lastOpt, extract 2. and 3. word
	i1=`expr index $inOpt , `
	i2=($i1+3)
	#echo `expr index $inOpt , ` = $i1
	cMode=${inOpt:$i1:2}
	cSel=${inOpt:$i2:1}
	echo next iteration: cMode=$cMode, cSel=$cSel 
	if [[ ${cSel} = "-" ]];then 
	    cSel=${inOpt:$i2:2}
	    echo cSel=$cSel 
	    cSel="0"
	fi
	#copy calibration file 
	if (($iStep > $iRestart)) ; then
	    cp -v ../Init${iStepLast}/tofClust_${cRun}_set${cCalSet}.hst.root ${cRun}_set${cCalSet}_${cMode}_${cSel}tofClust.hst.root
	fi
    fi 

    lastOpt=$inOpt
    # generate new calibration file
    if (($iStep > $iRestart)) ; then 
	root -b -q '../../ana_digi_cal.C('$inOpt',"'${cRun}'",'${iCalSet}',0,'${iBRef}') '

	cp -v tofClust_${cRun}_set${cCalSet}.hst.root ../${cRun}_set${cCalSet}_${cMode}_${cSel}tofClust.hst.root
	cp *pdf ../
	#./screenshot.sh
	cd .. 
	rm ../${cRun}_set${cCalSet}_${cMode}_${cSel}tofClust.hst.root
	ln -s ./${cRun}/${cRun}_set${cCalSet}_${cMode}_${cSel}tofClust.hst.root ../${cRun}_set${cCalSet}_${cMode}_${cSel}tofClust.hst.root
	echo Init step $iStep with mode ${cMode}, option $inOpt  finished
    else 
	cd ..
	echo Init step $iStep with mode ${cMode}, option $inOpt  skipped
    fi   
done

cd  ${wdir}
mv -v slurm-${SLURM_JOB_ID}.out ${outdir}/InitCalib_${cRun}_${cCalSet}.out
