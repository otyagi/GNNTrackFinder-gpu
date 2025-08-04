# Copyright (C) 2015 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# First commited by Maxim Zyzak

# #!
# run KF Particle Finder

MAINDIR=`pwd`

function DrawParticle {
  ParticleNameOutput="$(root -l -b -q "../GetParticleName.C($1)" | cat)"
  ParticleName="$(echo $ParticleNameOutput | sed 's/^.* //')"
  CURDIR="Signal"$i"_"$ParticleName
  mkdir $CURDIR
  cd $CURDIR
  echo `pwd`
  
  cp $MAINDIR/CbmKFParticleFinderQa.root .
  cp $MAINDIR/DrawParticlesFitQA.C .
  root -l -b -q "DrawParticlesFitQA.C($1)" > fit.log
  
  mkdir Daughters
  mv *Daughters.pdf Daughters
  
  rm -rf CbmKFParticleFinderQa.root DrawParticlesFitQA.C
  cd ../
}

WORKDIR="FitQAPlots"
rm -rf $WORKDIR
mkdir $WORKDIR

cd $WORKDIR

PID=""
for i in {0..74}
do
  DrawParticle $i &> /dev/null &
  PID=$PID" "$!
done
wait $PID

cd $MAINDIR

echo -e "\007"

