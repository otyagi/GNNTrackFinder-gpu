Jupyter Notebooks
=================

Jupyter Notebooks offer a large quality of life and are becoming more important with machine learning etc. Natively PandaRoot is a collection of C++ libraries which usually are collected and called through ROOT, mostly by macros.

ROOT by itself features python bindings as well as an own C++ Kernel. With the ``%%cpp`` magic it is also possible to mix C++ code into python code. See the introduction to PyROOT_. 

.. _PyROOT: https://root.cern.ch/notebooks/HowTos/HowTo_ROOT-Notebooks.html

Using the Kernels without installation
######################################

It it is enough to do the following::

  source <build>/config.sh -p
  export JUPYTER_CONFIG_DIR=$SIMPATH/etc/root/notebook
  export JUPYTER_PATH=$SIMPATH/etc/root/notebook
  jupyter notebook

You now can use ROOT and CbmRoot classes in the Python3 kernel or the RootC++ kernel. Here are two example notebooks to start off with.

Examples
########

- <source>/docs/jupyter/notebooks/CbmRoot.ipynb

Install the Kernel
##################

In some cases it is beneficial to install the Kernel to automatically be used in the system. A simple 'jupyter notebook' command in a fresh shell would allow them to be used. Also VSCode with the jupyter extension would be able to spawn such notebooks inside the integrated environment.

The downside is that the Kernels are pointing to one specific installation of CbmRoot and its dependencies and managing multiple installations may become cumbersome.

As a prerequisit one has to install the metakernel package in python (``pip3 install metakernel``).
To install the Kernels simply copy the folders from ``<build>/jupyter/kernels`` to the appropriate place:

- ``~/.local/share/jupyter/kernels`` (Linux)
- ``~/Library/Jupyter/kernels`` (Mac)
- ``%APPDATA%\jupyter\kernels`` (Win)



