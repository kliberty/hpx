# Copyright (c) 2020 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

source $SPACK_ROOT/share/spack/setup-env.sh

export CRAYPE_LINK_TYPE=dynamic

module load daint-mc
module load Boost
module load hwloc/.2.0.3
# We load the intel module last, since the Boost module otherwise loads the gnu
# module again
module switch PrgEnv-gnu PrgEnv-intel
spack load cmake@3.19.1
spack load ninja@1.10.2

export CXX=`which CC`
export CC=`which cc`

configure_extra_options="-DCMAKE_BUILD_TYPE=Debug"
configure_extra_options+=" -DHPX_WITH_MAX_CPU_COUNT=128"
configure_extra_options+=" -DHPX_WITH_MALLOC=system"
configure_extra_options+=" -DHPX_WITH_COMPILER_WARNINGS=ON"
configure_extra_options+=" -DHPX_WITH_COMPILER_WARNINGS_AS_ERRORS=ON"
configure_extra_options+=" -DHPX_WITH_SPINLOCK_DEADLOCK_DETECTION=ON"
configure_extra_options+=" -DHWLOC_ROOT=${EBROOTHWLOC}"
