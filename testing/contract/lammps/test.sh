#!/bin/bash

set -x
set -e

source $(dirname $(readlink -f ${BASH_SOURCE}))/setup.sh

mkdir -p ${test_dir}
cd ${test_dir}
cp -v ${source_dir}/../adios2-source/testing/contract/lammps/{adios2_config.xml,check_results.sh,in.test} .


mpiexec -np 4 --oversubscribe ${install_dir}/bin/lmp -in in.test

./check_results.sh
