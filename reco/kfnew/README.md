# CbmRoot-Specific Representation of the KF framework

## Introduction 

The CbmKf Library contains a set of different Kalman-Filter based fitting utilities, which are specific for the CBM experiment.

**The existing KF-libary will be later replaced with this new one.** Components of the CbmKf library will be placed in the `cbm::kf`
 namespace (**NOTE:** components of the KfCore are kept in a different namespace **`cbm::algo::kf`**).


## Library main classes

### **`cbm::kf::SetupFactory`**

The purpose of the class is to provide a common initialization routine for the kf::Setup instances for different data reconstruction
units of the CBM experiment.




