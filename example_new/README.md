##  Reproduce ARIES Experiment Results

## *** Step 1: VCK190 compilation environment setup 

### Download Common Image [VCK190 Setup](../utils/README.md#-vck190-setup)

### Setup Vitis environment
```sh 
source /opt/tools/xilinx/Vitis/2023.2/settings64.sh
source /opt/xilinx/xrt/setup.sh
unset LD_LIBRARY_PATH (If needed)
source /opt/petalinux/2023.2/environment-setup-cortexa72-cortexa53-xilinx-linux
```

## ***  Step 2: Setup and compile project

### Create ARIES Initial IR and ARIES Makefile from user defined ARIES Python-based frontend.(Within 10 seconds)
```sh
cd example_ttmc
python3 ttmc.py
```

### Generate code for Host + PL + AIE (Need to have ARIES installed, i.e.,  tools including aries-opt and aries-translate should run correctly) (Within 10 seconds)
```sh
cd my_project
make all
```

### Compile the generated project (Around 5 hours)
```sh
cd project
make package EDGE_COMMON_SW_PATH=${AIEPATH}
```