# AMD Tools
source ~/apps/xilinx.sh

# MPI
export PATH=/home/pedro.rosso/TRETS/mpi/bin:$PATH
export LD_LIBRARY_PATH=/home/pedro.rosso/TRETS/mpi/lib:$LD_LIBRARY_PATH
export LIBRARY_PATH=/home/pedro.rosso/TRETS/mpi/lib:$LIBRARY_PATH
export CPATH=/home/pedro.rosso/TRETS/mpi/include:$CPATH

# OMPCNET
export LD_LIBRARY_PATH=/home/pedro.rosso/TRETS/comm_test/ompcnet/lib:$LD_LIBRARY_PATH
export LIBRARY_PATH=/home/pedro.rosso/TRETS/comm_test/ompcnet/lib:$LIBRARY_PATH
export CPATH=/home/pedro.rosso/TRETS/comm_test/ompcnet/include:$CPATH
