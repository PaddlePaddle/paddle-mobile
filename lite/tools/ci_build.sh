#!/bin/bash
# The git version of CI is 2.7.4. This script is not compatible with git version 1.7.1.
set -ex

TESTS_FILE="./lite_tests.txt"
LIBS_FILE="./lite_libs.txt"
CUDNN_ROOT="/usr/local/cudnn"
LITE_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}")/../../" && pwd )"

readonly ADB_WORK_DIR="/data/local/tmp"
readonly common_flags="-DWITH_LITE=ON -DLITE_WITH_LIGHT_WEIGHT_FRAMEWORK=OFF -DWITH_PYTHON=OFF -DWITH_TESTING=ON -DLITE_WITH_ARM=OFF"

readonly THIRDPARTY_TAR=https://paddle-inference-dist.bj.bcebos.com/PaddleLite/third-party-05b862.tar.gz
readonly workspace=$PWD

NUM_CORES_FOR_COMPILE=${LITE_BUILD_THREADS:-8}

# global variables
#whether to use emulator as adb devices,when USE_ADB_EMULATOR=ON we use emulator, else we will use connected mobile phone as adb devices.
USE_ADB_EMULATOR=ON
# Use real android devices, set the device names for adb connection, ignored if USE_ADB_EMULATOR=ON
ADB_DEVICE_LIST=""
# The list of tests which are ignored, use commas to separate them, such as "test_cxx_api,test_mobilenetv1_int8"
TEST_SKIP_LIST=""
# The list of model tests, its format is "<target_name>,<extra_args>,<model_dir>,<data_dir>,<config_dir>", use '-' if an argument is empty, such as
# "test_mobilenetv1_fp32_huawei_kirin_npu,-,mobilenet_v1,ILSVRC2012_small,-;test_mobilenetv2_fp32_huawei_kirin_npu,-,mobilenet_v2_relu,ILSVRC2012_small,-;"
TEST_MODEL_LIST=""
LITE_WITH_COVERAGE=OFF

# if operating in mac env, we should expand the maximum file num
os_name=`uname -s`
if [ ${os_name} == "Darwin" ]; then
   ulimit -n 1024
fi

function prepare_thirdparty {
    if [ ! -d $workspace/third-party -o -f $workspace/third-party-05b862.tar.gz ]; then
        rm -rf $workspace/third-party

        if [ ! -f $workspace/third-party-05b862.tar.gz ]; then
            wget $THIRDPARTY_TAR
        fi
        tar xzf third-party-05b862.tar.gz
    else
        git submodule update --init --recursive
    fi
}

function prepare_opencl_source_code {
    local root_dir=$1
    local build_dir=$2
    # in build directory
    # Prepare opencl_kernels_source.cc file
    GEN_CODE_PATH_OPENCL=$root_dir/lite/backends/opencl
    rm -f GEN_CODE_PATH_OPENCL/opencl_kernels_source.cc
    OPENCL_KERNELS_PATH=$root_dir/lite/backends/opencl/cl_kernel
    mkdir -p ${GEN_CODE_PATH_OPENCL}
    touch $GEN_CODE_PATH_OPENCL/opencl_kernels_source.cc
    python $root_dir/lite/tools/cmake_tools/gen_opencl_code.py $OPENCL_KERNELS_PATH $GEN_CODE_PATH_OPENCL/opencl_kernels_source.cc
}

# prepare adb devices
# if USE_ADB_EMULATOR=ON , we create adb emulator port_armv8 and port_armv7 for usage, else we will use actual mobilephone according to adbindex.
function prepare_adb_devices {
    port_armv8=5554
    port_armv7=5556
    if [ $USE_ADB_EMULATOR == "ON" ]; then
       prepare_emulator $port_armv8 $port_armv7
       device_armv8=emulator-$port_armv8
       device_armv7=emulator-$port_armv7
    else
       adb_devices=($(adb devices |grep -v devices |grep device | awk -F " " '{print $1}'))
       # adbindex is the env variable registered in ci agent to tell which mobile is to used as adb
       adbindex_pos=`expr ${adbindex} + 1`
       if [ ${adbindex_pos} -gt ${#adb_devices[@]} ]; then
           echo -e "Error: the adb devices on ci agent are not enough, at least ${adbindex_pos} adb devices are needed."
           exit 1
       fi
       echo ${adb_devices[${adbindex}]}
       device_armv8=${adb_devices[${adbindex}]}
       device_armv7=${adb_devices[${adbindex}]}
    fi
}

# for code gen, a source file is generated after a test, but is dependended by some targets in cmake.
# here we fake an empty file to make cmake works.
function prepare_workspace {
    # in build directory
    # 1. Prepare gen_code file
    GEN_CODE_PATH_PREFIX=lite/gen_code
    mkdir -p ./${GEN_CODE_PATH_PREFIX}
    touch ./${GEN_CODE_PATH_PREFIX}/__generated_code__.cc

    # 2.Prepare debug tool
    DEBUG_TOOL_PATH_PREFIX=lite/tools/debug
    mkdir -p ./${DEBUG_TOOL_PATH_PREFIX}
    cp ../${DEBUG_TOOL_PATH_PREFIX}/analysis_tool.py ./${DEBUG_TOOL_PATH_PREFIX}/

    # clone submodule
    # git submodule update --init --recursive
    prepare_thirdparty
}

function check_need_ci {
    git log -1 --oneline | grep "test=develop" || exit -1
}

function check_coverage() {
    bash ../tools/coverage/paddle_lite_coverage.sh
}

function cmake_x86 {
    prepare_workspace
    #cmake ..  -DWITH_GPU=OFF -DWITH_MKLDNN=OFF -DLITE_WITH_X86=ON ${common_flags}
    cmake ..  -DWITH_GPU=OFF -DWITH_MKLDNN=OFF -DLITE_WITH_X86=ON  -DWITH_COVERAGE=$LITE_WITH_COVERAGE ${common_flags}
}

function cmake_opencl {
    prepare_workspace
    # $1: ARM_TARGET_OS in "android" , "armlinux"
    # $2: ARM_TARGET_ARCH_ABI in "armv8", "armv7" ,"armv7hf"
    # $3: ARM_TARGET_LANG in "gcc" "clang"
    cmake .. \
        -DLITE_WITH_OPENCL=ON \
        -DWITH_GPU=OFF \
        -DWITH_MKL=OFF \
        -DWITH_LITE=ON \
        -DLITE_WITH_CUDA=OFF \
        -DLITE_WITH_X86=OFF \
        -DLITE_WITH_ARM=ON \
        -DWITH_ARM_DOTPROD=ON   \
        -DLITE_WITH_LIGHT_WEIGHT_FRAMEWORK=ON \
        -DWITH_TESTING=ON \
        -DLITE_BUILD_EXTRA=ON \
        -DLITE_WITH_LOG=ON \
        -DLITE_WITH_CV=OFF \
        -DARM_TARGET_OS=$1 -DARM_TARGET_ARCH_ABI=$2 -DARM_TARGET_LANG=$3
}

function run_gen_code_test {
    local device=$1
    local gen_code_file_name="__generated_code__.cc"
    local gen_code_file_path="./lite/gen_code/${gen_code_file_path}"
    local adb_work_dir="/data/local/tmp"

    # 1. build test_cxx_api
    make test_cxx_api -j$NUM_CORES_FOR_COMPILE

    # 2. run test_cxx_api_lite in emulator to get opt model 
    local test_cxx_api_lite_path=$(find ./lite -name test_cxx_api)
    adb -s ${device} push "./third_party/install/lite_naive_model" ${adb_work_dir}
    adb -s ${device} push ${test_cxx_api_lite_path} ${adb_work_dir}
    adb -s ${device} shell "${adb_work_dir}/test_cxx_api --model_dir=${adb_work_dir}/lite_naive_model --optimized_model=${adb_work_dir}/lite_naive_model_opt"

    # 3. build test_gen_code
    make test_gen_code -j$NUM_CORES_FOR_COMPILE

    # 4. run test_gen_code_lite in emulator to get __generated_code__.cc
    local test_gen_code_lite_path=$(find ./lite -name test_gen_code)
    adb -s ${device} push ${test_gen_code_lite_path} ${adb_work_dir}
    adb -s ${device} shell "${adb_work_dir}/test_gen_code --optimized_model=${adb_work_dir}/lite_naive_model_opt --generated_code_file=${adb_work_dir}/${gen_code_file_name}"

    # 5. pull __generated_code__.cc down and mv to build real path
    adb -s ${device} pull "${adb_work_dir}/${gen_code_file_name}" .
    mv ${gen_code_file_name} ${gen_code_file_path}

    # 6. build test_generated_code
    make test_generated_code -j$NUM_CORES_FOR_COMPILE
}

# $1: ARM_TARGET_OS in "android" , "armlinux"
# $2: ARM_TARGET_ARCH_ABI in "armv8", "armv7" ,"armv7hf"
# $3: ARM_TARGET_LANG in "gcc" "clang"
function build_opencl {
    os=$1
    abi=$2
    lang=$3

    cur_dir=$(pwd)
    if [[ ${os} == "armlinux" ]]; then
        # TODO(hongming): enable compile armv7 and armv7hf on armlinux, and clang compile
        if [[ ${lang} == "clang" ]]; then
            echo "clang is not enabled on armlinux yet"
            return 0
        fi
        if [[ ${abi} == "armv7hf" ]]; then
            echo "armv7hf is not supported on armlinux yet"
            return 0
        fi
        if [[ ${abi} == "armv7" ]]; then
            echo "armv7 is not supported on armlinux yet"
            return 0
        fi
    fi

    if [[ ${os} == "android" && ${abi} == "armv7hf" ]]; then
        echo "android do not need armv7hf"
        return 0
    fi

    build_dir=$cur_dir/build.lite.${os}.${abi}.${lang}
    mkdir -p $build_dir
    cd $build_dir

    prepare_opencl_source_code $cur_dir $build_dir

    cmake_opencl ${os} ${abi} ${lang}
    make opencl_clhpp -j$NUM_CORES_FOR_COMPILE
    make publish_inference -j$NUM_CORES_FOR_COMPILE
    build $TESTS_FILE
}



# This method is only called in CI.
function cmake_x86_for_CI {
    prepare_workspace # fake an empty __generated_code__.cc to pass cmake.
    cmake ..  -DWITH_GPU=OFF -DWITH_MKLDNN=OFF -DLITE_WITH_X86=ON ${common_flags} -DLITE_WITH_PROFILE=ON -DWITH_MKL=ON \
        -DLITE_BUILD_EXTRA=ON -DWITH_COVERAGE=ON 

    # Compile and execute the gen_code related test, so it will generate some code, and make the compilation reasonable.
    # make test_gen_code -j$NUM_CORES_FOR_COMPILE
    # make test_cxx_api -j$NUM_CORES_FOR_COMPILE
    # ctest -R test_cxx_api
    # ctest -R test_gen_code
    # make test_generated_code -j$NUM_CORES_FOR_COMPILE
}

function cmake_cuda_for_CI {
    prepare_workspace # fake an empty __generated_code__.cc to pass cmake.
    cmake ..  -DLITE_WITH_CUDA=ON -DWITH_MKLDNN=OFF -DLITE_WITH_X86=OFF ${common_flags} -DLITE_WITH_PROFILE=OFF -DWITH_MKL=OFF -DLITE_BUILD_EXTRA=ON -DCUDNN_ROOT=${CUDNN_ROOT} -DWITH_LITE=OFF
}

function cmake_gpu {
    prepare_workspace
    cmake .. " -DWITH_GPU=ON {common_flags} -DLITE_WITH_GPU=ON"
}

function check_style {
    export PATH=/usr/bin:$PATH
    #pre-commit install
    clang-format --version

    if ! pre-commit run -a ; then
        git diff
        exit 1
    fi
}

function build_single {
    #make $1 -j$(expr $(nproc) - 2)
    make $1 -j$NUM_CORES_FOR_COMPILE
}

function build {
    make lite_compile_deps -j$NUM_CORES_FOR_COMPILE
    if [ $LITE_WITH_COVERAGE = "ON" ];then
        make coveralls_generate -j	
    fi 
    # test publish inference lib
    # make publish_inference
}

# It will eagerly test all lite related unittests.
function test_server {
    # Due to the missing of x86 kernels, we skip the following tests temporarily.
    # TODO(xxx) clear the skip list latter
    local skip_list=("test_paddle_api" "test_cxx_api"
                     "test_light_api"
                     "test_apis" "test_model_bin"
                    )
    local to_skip=0
    for _test in $(cat $TESTS_FILE); do
        to_skip=0
        for skip_name in ${skip_list[@]}; do
            if [ $skip_name = $_test ]; then
                echo "to skip " $skip_name
                to_skip=1
            fi
        done

        if [ $to_skip -eq 0 ]; then
            ctest -R $_test -V
        fi
    done
}

function assert_api_spec_approvals() {
    /bin/bash ${LITE_ROOT}/lite/tools/check_api_approvals.sh check_modified_file_nums
    if [ "$?" != 0 ];then
       exit 1
    fi
}

# Build the code and run lite server tests. This is executed in the CI system.
function build_test_server {
    mkdir -p ./build
    cd ./build
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/third_party/install/mklml/lib"
    assert_api_spec_approvals
    cmake_x86_for_CI
    build

    test_server
    test_model_optimize_tool_compile
}

# Build the code and run lite server tests. This is executed in the CI system.
function build_test_coverage {
    mkdir -p ./build
    cd ./build
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/third_party/install/mklml/lib"
    cmake_x86_for_CI
    build

    test_server
}

# The CUDA version of CI is cuda_10.1.243_418.87.00_linux.
# The cuDNN version is cudnn-10.1-linux-x64-v7.5.0.56.
function build_test_cuda_server {
    mkdir -p ./build
    cd ./build
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/third_party/install/mklml/lib"
    cmake_cuda_for_CI
    make -j$NUM_CORES_FOR_COMPILE
    # temporary remove cuda unittest because the ci PR_CI_Paddle-Lite-server-cuda10.1(ubt16-gcc5.4) is in cpu machine and only build.
    # ctest -R "/*_cuda_test" -V
}

function build_test_train {
    mkdir -p ./build
    cd ./build
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/paddle/build/third_party/install/mklml/lib"
    prepare_workspace # fake an empty __generated_code__.cc to pass cmake.
    cmake .. -DWITH_LITE=ON -DWITH_GPU=OFF -DWITH_PYTHON=ON -DLITE_WITH_X86=ON -DLITE_WITH_LIGHT_WEIGHT_FRAMEWORK=OFF -DWITH_TESTING=ON -DWITH_MKL=OFF \
        -DLITE_BUILD_EXTRA=ON \

    make test_gen_code -j$NUM_CORES_FOR_COMPILE
    make test_cxx_api -j$NUM_CORES_FOR_COMPILE
    ctest -R test_cxx_api
    ctest -R test_gen_code
    make test_generated_code -j$NUM_CORES_FOR_COMPILE

    make -j$NUM_CORES_FOR_COMPILE

    find -name "*.whl" | xargs pip2 install
    python ../lite/tools/python/lite_test.py

}

# It will eagerly test all lite related unittests.
function test_xpu {
    # Due to the missing of xpu kernels, we skip the following tests temporarily.
    # TODO(xxx) clear the skip list latter
    local skip_list=("test_paddle_api" "test_cxx_api" "test_googlenet"
                     "test_mobilenetv1_lite_x86" "test_mobilenetv2_lite_x86"
                     "test_inceptionv4_lite_x86" "test_light_api"
                     "test_apis" "test_model_bin"
                    )
    local to_skip=0
    for _test in $(cat $TESTS_FILE); do
        to_skip=0
        for skip_name in ${skip_list[@]}; do
            if [ $skip_name = $_test ]; then
                echo "to skip " $skip_name
                to_skip=1
            fi
        done

        if [ $to_skip -eq 0 ]; then
            ctest -R $_test -V
        fi
    done
}

# Build the code and run lite server tests. This is executed in the CI system.
function build_test_xpu {
    local with_xtcl=$1
    if [[ "${with_xtcl}x" == "x" ]]; then
        with_xtcl=OFF
    fi
    mkdir -p ./build
    cd ./build
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/third_party/install/mklml/lib"
    prepare_workspace
    cmake .. \
        ${common_flags} \
        -DWITH_GPU=OFF \
        -DWITH_MKLDNN=OFF \
        -DLITE_WITH_X86=ON \
        -DWITH_MKL=ON \
        -DLITE_BUILD_EXTRA=ON \
        -DLITE_WITH_XPU=ON \
        -DLITE_WITH_XTCL=$with_xtcl\
        -DXPU_SDK_ROOT="./output"
    make lite_compile_deps -j$NUM_CORES_FOR_COMPILE

    test_xpu
}

function is_available_adb_device {
    local adb_device_name=$1
    if [[ -n "$adb_device_name" ]]; then
        for line in `adb devices | grep -v "List"  | awk '{print $1}'`
        do
            online_device_name=`echo $line | awk '{print $1}'`
            if [[ "$adb_device_name" == "$online_device_name" ]];then
                return 0
            fi
        done
    fi
    return 1
}

function pick_an_available_adb_device {
    local adb_device_list=$1
    local adb_device_names=(${adb_device_list//,/ })
    for adb_device_name in ${adb_device_names[@]}; do
        is_available_adb_device $adb_device_name
        if [[ $? -eq 0 ]]; then
            echo $adb_device_name
            return 0
        fi
    done
    echo ""
    return 1
}

function run_test_case_on_adb_device {
    # Support all of the ADB-supported devices, such as android and armlinux devices(RK1808 EVB)
    local adb_device_name=$1
    local adb_work_dir=$2
    local target_name=$3
    local extra_args=$4
    local model_dir=$5
    local data_dir=$6
    local config_dir=$7

    # Check device is available
    is_available_adb_device $adb_device_name
    if [[ $? -ne 0 ]]; then
        echo "${adb_device_name} not found!"
        exit 1
    fi

     # Be careful!!! Don't delete the root or system directories if the device is rooted.
    if [[ -z "$adb_work_dir" ]]; then
        echo "$adb_work_dir can't be empty!"
        exit 1
    fi
    if [[ "$adb_work_dir" == "/" ]]; then
        echo "$adb_work_dir can't be root dir!"
        exit 1
    fi

    # Copy the executable unit test to the remote device
    local target_path=$(find ./lite -name $target_name)
    if [[ -z "$target_path" ]]; then
        echo "$target_name not found!"
        exit 1
    fi
    adb -s ${adb_device_name} shell "rm -f ${adb_work_dir}/${target_name}"
    adb -s ${adb_device_name} push "${target_path}" ${adb_work_dir}

    # Add the extra arguments into the test command
    local cmd_args=""
    if [[ -n "$extra_args" && "$extra_args" != "-" ]]; then
        cmd_args="${cmd_args} ${extra_args}"
    fi

    # Copy the model files to the remote device
    if [[ -n "$model_dir" && "$model_dir" != "-" ]]; then
        adb -s ${adb_device_name} shell "rm -rf ${adb_work_dir}/${model_dir}"
        adb -s ${adb_device_name} push "./third_party/install/${model_dir}" ${adb_work_dir}
        cmd_args="${cmd_args} --model_dir ./${model_dir}"
    fi

    # Copy the test data files to the remote device
    if [[ -n "$data_dir" && "$data_dir" != "-" ]]; then
        adb -s ${adb_device_name} shell "rm -rf ${adb_work_dir}/${data_dir}"
        adb -s ${adb_device_name} push "./third_party/install/${data_dir}" ${adb_work_dir}
        cmd_args="${cmd_args} --data_dir ./${data_dir}"
    fi

    # Copy the config files to the remote device
    if [[ -n "$config_dir" && "$config_dir" != "-"  ]]; then
        adb -s ${adb_device_name} shell "rm -rf ${adb_work_dir}/${config_dir}"
        adb -s ${adb_device_name} push "./third_party/install/${config_dir}" ${adb_work_dir}
        cmd_args="${cmd_args} --config_dir ./${config_dir}"
    fi
    
    # Run the model on the remote device
    adb -s ${adb_device_name} shell "cd ${adb_work_dir}; export GLOG_v=5; LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./${target_name} ${cmd_args}"
}

function run_all_tests_on_adb_device {
    local adb_device_list=$1
    local test_skip_list=$2
    local test_model_list=$3
    local adb_work_dir=$4
    local sdk_root_dir=$5
    local test_arch_list=$6
    local test_toolchain_list=$7
    local build_targets_func=$8
    local prepare_devices_func=$9

    # Pick the first available adb device from list
    local adb_device_name=$(pick_an_available_adb_device $adb_device_list)
    if [[ -z $adb_device_name ]]; then
        echo "No adb device available!"
        exit 1
    else
        echo "Found a device'${adb_device_name}'."
    fi

    # Run all of unittests and model tests
    local test_archs=(${test_arch_list//,/ })
    local test_toolchains=(${test_toolchain_list//,/ })
    local test_skip_names=(${test_skip_list//,/ })
    local test_model_params=(${test_model_list//;/ })
    for arch in $test_archs; do
        for toolchain in $test_toolchains; do
            # Build all tests and prepare device environment for running tests
            echo "Build tests for MediaTek APU with ${arch}+${toolchain}"
            ${build_targets_func} $arch $toolchain $sdk_root_dir
            ${prepare_devices_func} $adb_device_name $adb_work_dir $arch $toolchain $sdk_root_dir
            # Run all of unit tests and model tests
            for test_name in $(cat $TESTS_FILE); do
                local is_skip=0
                for test_skip_name in ${test_skip_names[@]}; do
                    if [[ "$test_skip_name" == "$test_name" ]]; then
                        echo "skip " $test_name
                        is_skip=1
                        break
                    fi
                done
                if [[ $is_skip -ne 0 ]]; then
                    continue
                fi
                is_skip=0
                for test_model_param in ${test_model_params[@]}; do
                    local slots=(${test_model_param//,/ })
                    if [[ "${slots[0]}" == "$test_name" ]]; then
                        run_test_case_on_adb_device $adb_device_name $adb_work_dir $test_name "${slots[1]}" "${slots[2]}" "${slots[3]}" "${slots[4]}"
                        is_skip=1
                        break
                    fi
                done
                if [[ $is_skip -ne 0 ]]; then
                    continue
                fi
                run_test_case_on_adb_device $adb_device_name $adb_work_dir $test_name
            done
            cd - > /dev/null
        done
    done
}

# Huawei Kirin NPU
function huawei_kirin_npu_prepare_device {
    local adb_device_name=$1
    local adb_work_dir=$2
    local arch=$3
    local toolchain=$4
    local sdk_root_dir=$5

    # Check device is available
    is_available_adb_device $adb_device_name
    if [[ $? -ne 0 ]]; then
        echo "${adb_device_name} not found!"
        exit 1
    fi

    # Only root user can use HiAI runtime libraries in the android shell executables
    adb -s ${adb_device_name} root
    if [[ $? -ne 0 ]]; then
        echo "${adb_device_name} hasn't the root permission!"
        exit 1
    fi

    # Copy the runtime libraries of HiAI DDK to the target device
    local sdk_lib_dir=""
    if [[ ${arch} == "armv8" ]]; then
        sdk_lib_dir="${sdk_root_dir}/lib64"
    elif [[ ${arch} == "armv7" ]]; then
        sdk_lib_dir="${sdk_root_dir}/lib"
    else
        echo "${arch} isn't supported by HiAI DDK!"
        exit 1
    fi
    adb -s ${adb_device_name} push ${sdk_lib_dir}/. ${adb_work_dir}
}

function huawei_kirin_npu_build_targets {
    local arch=$1
    local toolchain=$2
    local sdk_root_dir=$3

    # Build all of tests
    rm -rf ./build
    mkdir -p ./build
    cd ./build
    prepare_workspace
    cmake .. \
        -DWITH_GPU=OFF \
        -DWITH_MKL=OFF \
        -DWITH_LITE=ON \
        -DLITE_WITH_CUDA=OFF \
        -DLITE_WITH_X86=OFF \
        -DLITE_WITH_ARM=ON \
        -DWITH_ARM_DOTPROD=ON   \
        -DLITE_WITH_LIGHT_WEIGHT_FRAMEWORK=ON \
        -DWITH_TESTING=ON \
        -DLITE_BUILD_EXTRA=ON \
        -DLITE_WITH_TRAIN=ON \
        -DANDROID_STL_TYPE="c++_shared" \
        -DLITE_WITH_NPU=ON \
        -DNPU_DDK_ROOT="${sdk_root_dir}" \
        -DARM_TARGET_OS="android" -DARM_TARGET_ARCH_ABI=$arch -DARM_TARGET_LANG=$toolchain
    make lite_compile_deps -j$NUM_CORES_FOR_COMPILE
}

function huawei_kirin_npu_build_and_test {
    run_all_tests_on_adb_device $1 $2 $3 "/data/local/tmp" "$(readlink -f ./hiai_ddk_lib_330)" "armv7" "gcc,clang" huawei_kirin_npu_build_targets huawei_kirin_npu_prepare_device
}

# Rockchip NPU
function rockchip_npu_prepare_device {
    local adb_device_name=$1
    local adb_work_dir=$2
    local arch=$3
    local toolchain=$4
    local sdk_root_dir=$5

    # Check device is available
    is_available_adb_device $adb_device_name
    if [[ $? -ne 0 ]]; then
        echo "${adb_device_name} not found!"
        exit 1
    fi

    # Use high performance mode
    adb -s ${adb_device_name} shell "echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
    adb -s ${adb_device_name} shell "echo 1608000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed"
    adb -s ${adb_device_name} shell "echo userspace > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor"
    adb -s ${adb_device_name} shell "echo 1608000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_setspeed"

    # Copy the runtime libraries of Rockchip NPU to the target device
    local sdk_lib_dir=""
    if [[ ${arch} == "armv8" ]]; then
        sdk_lib_dir="${sdk_root_dir}/lib64"
    elif [[ ${arch} == "armv7" ]]; then
        sdk_lib_dir="${sdk_root_dir}/lib"
    else
        echo "${arch} isn't supported by Rockchip NPU SDK!"
        exit 1
    fi
    adb -s ${adb_device_name} push ${sdk_lib_dir}/. ${adb_work_dir}
}

function rockchip_npu_build_targets {
    local arch=$1
    local toolchain=$2
    local sdk_root_dir=$3

    # Build all of tests
    rm -rf ./build
    mkdir -p ./build
    cd ./build
    prepare_workspace
    cmake .. \
        -DWITH_GPU=OFF \
        -DWITH_MKL=OFF \
        -DWITH_LITE=ON \
        -DLITE_WITH_CUDA=OFF \
        -DLITE_WITH_X86=OFF \
        -DLITE_WITH_ARM=ON \
        -DWITH_ARM_DOTPROD=ON   \
        -DLITE_WITH_LIGHT_WEIGHT_FRAMEWORK=ON \
        -DWITH_TESTING=ON \
        -DLITE_BUILD_EXTRA=ON \
        -DLITE_WITH_TRAIN=ON \
        -DLITE_WITH_RKNPU=ON \
        -DRKNPU_DDK_ROOT="${sdk_root_dir}" \
        -DARM_TARGET_OS="armlinux" -DARM_TARGET_ARCH_ABI=$arch -DARM_TARGET_LANG=$toolchain
    make lite_compile_deps -j$NUM_CORES_FOR_COMPILE
}

function rockchip_npu_build_and_test {
    run_all_tests_on_adb_device $1 $2 $3 "/userdata/bin" "$(readlink -f ./rknpu_ddk)" "armv8" "gcc" rockchip_npu_build_targets rockchip_npu_prepare_device
}

# MediaTek APU
function mediatek_apu_prepare_device {
    local adb_device_name=$1
    local adb_work_dir=$2
    local arch=$3
    local toolchain=$4
    local sdk_root_dir=$5

    # Check device is available
    is_available_adb_device $adb_device_name
    if [[ $? -ne 0 ]]; then
        echo "${adb_device_name} not found!"
        exit 1
    fi

    # Use high performance mode
    adb -s ${adb_device_name} root
    if [[ $? -ne 0 ]]; then
        echo "${adb_device_name} hasn't the root permission!"
        exit 1
    fi
    adb -s ${adb_device_name} shell "echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
    adb -s ${adb_device_name} shell "echo performance > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor"
    adb -s ${adb_device_name} shell "echo performance > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor"
    adb -s ${adb_device_name} shell "echo performance > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor"
    adb -s ${adb_device_name} shell "cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
    adb -s ${adb_device_name} shell "echo 800000 > /proc/gpufreq/gpufreq_opp_freq"
    adb -s ${adb_device_name} shell "echo dvfs_debug 0 > /sys/kernel/debug/vpu/power"
    adb -s ${adb_device_name} shell "echo 0 > /sys/devices/platform/soc/10012000.dvfsrc/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp"
    adb -s ${adb_device_name} shell "echo 0 > /sys/module/mmdvfs_pmqos/parameters/force_step"
    adb -s ${adb_device_name} shell "echo 0 > /proc/sys/kernel/printk"
}

function mediatek_apu_build_targets {
    local arch=$1
    local toolchain=$2
    local sdk_root_dir=$3

    # Build all of tests
    rm -rf ./build
    mkdir -p ./build
    cd ./build
    prepare_workspace
    cmake .. \
        -DWITH_GPU=OFF \
        -DWITH_MKL=OFF \
        -DWITH_LITE=ON \
        -DLITE_WITH_CUDA=OFF \
        -DLITE_WITH_X86=OFF \
        -DLITE_WITH_ARM=ON \
        -DWITH_ARM_DOTPROD=ON   \
        -DLITE_WITH_LIGHT_WEIGHT_FRAMEWORK=ON \
        -DWITH_TESTING=ON \
        -DLITE_BUILD_EXTRA=ON \
        -DLITE_WITH_TRAIN=ON \
        -DLITE_WITH_APU=ON \
        -DAPU_DDK_ROOT="${sdk_root_dir}" \
        -DARM_TARGET_OS="android" -DARM_TARGET_ARCH_ABI=$arch -DARM_TARGET_LANG=$toolchain
    make lite_compile_deps -j$NUM_CORES_FOR_COMPILE
}

function mediatek_apu_build_and_test {
    run_all_tests_on_adb_device $1 $2 $3 "/data/local/tmp" "$(readlink -f ./apu_ddk)" "armv8" "gcc" mediatek_apu_build_targets mediatek_apu_prepare_device
}

function cmake_huawei_ascend_npu {
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/third_party/install/mklml/lib"
    prepare_workspace
    cmake .. \
        ${common_flags} \
        -DWITH_GPU=OFF \
        -DWITH_MKLDNN=OFF \
        -DLITE_WITH_X86=ON \
        -DWITH_MKL=ON \
        -DLITE_BUILD_EXTRA=ON \
        -DLITE_WITH_HUAWEI_ASCEND_NPU=ON \
        -DHUAWEI_ASCEND_NPU_DDK_ROOT="/usr/local/Ascend/ascend-toolkit/latest/x86_64-linux_gcc4.8.5" \
        -DCMAKE_BUILD_TYPE=Release
}

function build_huawei_ascend_npu {
    make lite_compile_deps -j$NUM_CORES_FOR_COMPILE
}

# It will eagerly test all lite related unittests.
function test_huawei_ascend_npu {
    # Due to the missing of ascend kernels, we skip the following tests temporarily.
    # TODO(xxx) clear the skip list latter
    local skip_list=("test_paddle_api" "test_cxx_api" "test_googlenet"
                     "test_mobilenetv1_lite_x86" "test_mobilenetv2_lite_x86"
                     "test_inceptionv4_lite_x86" "test_light_api"
                     "test_apis" "test_model_bin"
                    )
    local to_skip=0
    for _test in $(cat $TESTS_FILE); do
        to_skip=0
        for skip_name in ${skip_list[@]}; do
            if [ $skip_name = $_test ]; then
                echo "to skip " $skip_name
                to_skip=1
            fi
        done

        if [ $to_skip -eq 0 ]; then
            ctest -R $_test -V
        fi
    done
}

# Build the code and run lite server tests. This is executed in the CI system.
function build_test_huawei_ascend_npu {
    cur_dir=$(pwd)

    build_dir=$cur_dir/build.lite.huawei_ascend_npu_test
    mkdir -p $build_dir
    cd $build_dir

    cmake_huawei_ascend_npu
    build_huawei_ascend_npu

    test_huawei_ascend_npu
}

# test_arm_android <some_test_name> <adb_port_number>
function test_arm_android {
    local test_name=$1
    local device=$2
    if [[ "${test_name}x" == "x" ]]; then
        echo "test_name can not be empty"
        exit 1
    fi
    if [[ "${device}x" == "x" ]]; then
        echo "Port can not be empty"
        exit 1
    fi

    echo "test name: ${test_name}"
    adb_work_dir="/data/local/tmp"

    skip_list=("test_model_parser" "test_mobilenetv1" "test_mobilenetv2" "test_resnet50" "test_inceptionv4" "test_light_api" "test_apis" "test_paddle_api" "test_cxx_api" "test_gen_code" "test_mobilenetv1_int8" "test_subgraph_pass" "test_grid_sampler_image_opencl" "test_lrn_image_opencl" "test_pad2d_image_opencl" "test_transformer_with_mask_fp32_arm")
    for skip_name in ${skip_list[@]} ; do
        [[ $skip_name =~ (^|[[:space:]])$test_name($|[[:space:]]) ]] && echo "skip $test_name" && return
    done

    local testpath=$(find ./lite -name ${test_name})

    adb -s ${device} push ${testpath} ${adb_work_dir}
    adb -s ${device} shell "cd ${adb_work_dir} && ./${test_name}"
    adb -s ${device} shell "rm -f ${adb_work_dir}/${test_name}"
}

# test the inference high level api
function test_arm_api {
    local device=$1
    local test_name="test_paddle_api"

    make $test_name -j$NUM_CORES_FOR_COMPILE

    local model_path=$(find . -name "lite_naive_model")
    local remote_model=${adb_work_dir}/paddle_api
    local testpath=$(find ./lite -name ${test_name})

    arm_push_necessary_file $device $model_path $remote_model
    adb -s ${device} shell mkdir -p $remote_model
    adb -s ${device} push ${testpath} ${adb_work_dir}
    adb -s ${device} shell chmod +x "${adb_work_dir}/${test_name}"
    adb -s ${device} shell "${adb_work_dir}/${test_name} --model_dir $remote_model"
}

function test_arm_model {
    local test_name=$1
    local device=$2
    local model_dir=$3

    if [[ "${test_name}x" == "x" ]]; then
        echo "test_name can not be empty"
        exit 1
    fi
    if [[ "${device}x" == "x" ]]; then
        echo "Port can not be empty"
        exit 1
    fi
    if [[ "${model_dir}x" == "x" ]]; then
        echo "Model dir can not be empty"
        exit 1
    fi

    echo "test name: ${test_name}"
    adb_work_dir="/data/local/tmp"

    testpath=$(find ./lite -name ${test_name})
    adb -s ${device} push ${model_dir} ${adb_work_dir}
    adb -s ${device} push ${testpath} ${adb_work_dir}
    adb -s ${device} shell chmod +x "${adb_work_dir}/${test_name}"
    local adb_model_path="${adb_work_dir}/`basename ${model_dir}`"
    adb -s ${device} shell "${adb_work_dir}/${test_name} --model_dir=$adb_model_path"
}

# function _test_model_optimize_tool {
#     local port=$1
#     local remote_model_path=$ADB_WORK_DIR/lite_naive_model
#     local remote_test=$ADB_WORK_DIR/model_optimize_tool
#     local adb="adb -s emulator-${port}"

#     make model_optimize_tool -j$NUM_CORES_FOR_COMPILE
#     local test_path=$(find . -name model_optimize_tool | head -n1)
#     local model_path=$(find . -name lite_naive_model | head -n1)
#     $adb push ${test_path} ${ADB_WORK_DIR}
#     $adb shell mkdir -p $remote_model_path
#     $adb push $model_path/* $remote_model_path
#     $adb shell $remote_test --model_dir $remote_model_path --optimize_out ${remote_model_path}.opt \
#          --valid_targets "arm"
# }

function test_model_optimize_tool_compile {
    cd $workspace
    cd build
    # Compile opt tool
    cmake .. -DWITH_LITE=ON -DLITE_ON_MODEL_OPTIMIZE_TOOL=ON -DWITH_TESTING=OFF -DLITE_BUILD_EXTRA=ON
    make opt -j$NUM_CORES_FOR_COMPILE
    # Check whether opt can transform quantized mobilenetv1 successfully.
    cd lite/api && chmod +x ./opt
    wget --no-check-certificate https://paddlelite-data.bj.bcebos.com/doc_models/MobileNetV1_quant.tar.gz
    tar zxf MobileNetV1_quant.tar.gz
    ./opt --model_dir=./MobileNetV1_quant --valid_targets=arm --optimize_out=quant_mobilenetv1
    if [ ! -f quant_mobilenetv1.nb ]; then
       echo -e "Error! Resulted opt can not tramsform MobileNetV1_quant successfully!"
       exit 1
    fi
}

function _test_paddle_code_generator {
    local device=$1
    local test_name=paddle_code_generator
    local remote_test=$ADB_WORK_DIR/$test_name
    local remote_model=$ADB_WORK_DIR/lite_naive_model.opt
    local adb="adb -s ${device}"

    make paddle_code_generator -j$NUM_CORES_FOR_COMPILE
    local test_path=$(find . -name $test_name | head -n1)

    $adb push $test_path $remote_test
    $adb shell $remote_test --optimized_model $remote_model --generated_code_file $ADB_WORK_DIR/gen_code.cc
}

function cmake_arm {
    prepare_workspace
    # $1: ARM_TARGET_OS in "android" , "armlinux"
    # $2: ARM_TARGET_ARCH_ABI in "armv8", "armv7" ,"armv7hf"
    # $3: ARM_TARGET_LANG in "gcc" "clang"
    cmake .. \
        -DWITH_GPU=OFF \
        -DWITH_MKL=OFF \
        -DWITH_LITE=ON \
        -DLITE_WITH_CUDA=OFF \
        -DLITE_WITH_X86=OFF \
        -DLITE_WITH_ARM=ON \
        -DWITH_ARM_DOTPROD=ON   \
        -DLITE_WITH_LIGHT_WEIGHT_FRAMEWORK=ON \
        -DWITH_TESTING=ON \
        -DLITE_BUILD_EXTRA=ON \
        -DLITE_WITH_TRAIN=ON \
        -DARM_TARGET_OS=$1 -DARM_TARGET_ARCH_ABI=$2 -DARM_TARGET_LANG=$3
}

# $1: ARM_TARGET_OS in "android" , "armlinux"
# $2: ARM_TARGET_ARCH_ABI in "armv8", "armv7" ,"armv7hf"
# $3: ARM_TARGET_LANG in "gcc" "clang"
function build_arm {
    os=$1
    abi=$2
    lang=$3

    cur_dir=$(pwd)
    # TODO(xxx): enable armlinux clang compile
    if [[ ${os} == "armlinux" && ${lang} == "clang" ]]; then
        echo "clang is not enabled on armlinux yet"
        return 0
    fi

    if [[ ${os} == "android" && ${abi} == "armv7hf" ]]; then
        echo "android do not need armv7hf"
        return 0
    fi

    build_dir=$cur_dir/build.lite.${os}.${abi}.${lang}
    mkdir -p $build_dir
    cd $build_dir

    cmake_arm ${os} ${abi} ${lang}
    build $TESTS_FILE

}

# $1: ARM_TARGET_OS in "ios", "ios64"
# $2: ARM_TARGET_ARCH_ABI in "armv7", "armv8"
function build_ios {
    local os=$1
    local abi=$2
    build_dir=build.ios.${os}.${abi}
    echo "building ios target into $build_dir"
    echo "target os: $os"
    echo "target abi: $abi"
    mkdir -p ${build_dir}
    cd ${build_dir}
    GEN_CODE_PATH_PREFIX=lite/gen_code
    mkdir -p ./${GEN_CODE_PATH_PREFIX}
    touch ./${GEN_CODE_PATH_PREFIX}/__generated_code__.cc

    cmake .. \
            -DWITH_GPU=OFF \
            -DWITH_MKL=OFF \
            -DWITH_LITE=ON \
            -DLITE_WITH_CUDA=OFF \
            -DLITE_WITH_X86=OFF \
            -DLITE_WITH_ARM=ON \
            -DWITH_TESTING=OFF \
            -DLITE_WITH_JAVA=OFF \
            -DLITE_WITH_LOG=OFF \
            -DLITE_ON_TINY_PUBLISH=ON \
            -DLITE_WITH_OPENMP=OFF \
            -DWITH_ARM_DOTPROD=OFF \
            -DLITE_WITH_LIGHT_WEIGHT_FRAMEWORK=ON \
            -DARM_TARGET_ARCH_ABI=$abi \
            -DLITE_BUILD_EXTRA=$BUILD_EXTRA \
            -DLITE_WITH_CV=$BUILD_CV \
            -DARM_TARGET_OS=$os

    make publish_inference -j$NUM_PROC
    cd -
}

# $1: ARM_TARGET_OS in "android" , "armlinux"
# $2: ARM_TARGET_ARCH_ABI in "armv8", "armv7" ,"armv7hf"
# $3: ARM_TARGET_LANG in "gcc" "clang"
# $4: android test port
# Note: test must be in build dir
function test_arm {
    os=$1
    abi=$2
    lang=$3
    device=$4

    if [[ ${os} == "armlinux" ]]; then
        # TODO(hongming): enable test armlinux on armv8, armv7 and armv7hf
        echo "Skip test arm linux yet. armlinux must in another docker"
        return 0
    fi

    if [[ ${os} == "android" && ${abi} == "armv7hf" ]]; then
        echo "android do not need armv7hf"
        return 0
    fi

    # prepare for CXXApi test
    local adb="adb -s ${device}"
    $adb shell mkdir -p /data/local/tmp/lite_naive_model_opt

    echo "test file: ${TESTS_FILE}"
    for _test in $(cat $TESTS_FILE); do
        test_arm_android $_test $device
    done

    # test finally
    test_arm_api $device

    # _test_model_optimize_tool $port
    # _test_paddle_code_generator $port
}

function prepare_emulator {
    local port_armv8=$1
    local port_armv7=$2

    adb kill-server
    adb devices | grep emulator | cut -f1 | while read line; do adb -s $line emu kill; done
    # start android armv8 and armv7 emulators first
    echo n | avdmanager create avd -f -n paddle-armv8 -k "system-images;android-24;google_apis;arm64-v8a"
    echo -ne '\n' | ${ANDROID_HOME}/emulator/emulator -avd paddle-armv8 -noaudio -no-window -gpu off -port ${port_armv8} &
    sleep 1m
    if [[ "${port_armv7}x" != "x" ]]; then
        echo n | avdmanager create avd -f -n paddle-armv7 -k "system-images;android-24;google_apis;armeabi-v7a"
        echo -ne '\n' | ${ANDROID_HOME}/emulator/emulator -avd paddle-armv7 -noaudio -no-window -gpu off -port ${port_armv7} &
        sleep 1m
    fi
}

function arm_push_necessary_file {
    local device=$1
    local testpath=$2
    local adb_work_dir=$3

    adb -s ${device} push ${testpath} ${adb_work_dir}
}


function test_opencl {
    os=$1
    abi=$2
    lang=$3
    device=$4

    if [[ ${os} == "armlinux" ]]; then
        # TODO(hongming): enable test armlinux on armv8, armv7 and armv7hf
        echo "Skip test arm linux yet. armlinux must in another docker"
        return 0
    fi

    if [[ ${os} == "android" && ${abi} == "armv7hf" ]]; then
        echo "android do not need armv7hf"
        return 0
    fi

    # prepare for CXXApi test
    local adb="adb -s ${device}"
    $adb shell mkdir -p /data/local/tmp/lite_naive_model_opt

    # opencl test should be marked with `opencl`
    opencl_test_mark="opencl"

    for _test in $(cat $TESTS_FILE); do
        # tell if this test is marked with `opencl`
        if [[ $_test == *$opencl_test_mark* ]]; then
            test_arm_android $_test $device
        fi
    done

}

function build_test_arm_opencl {
    ########################################################################
    cur=$PWD
    # job 1-4 must be in one runner
    prepare_adb_devices

    # job 1
    build_opencl "android" "armv8" "gcc"
    adb -s $device_armv8 shell 'rm -rf /data/local/tmp/*'
    run_gen_code_test ${device_armv8}
    test_opencl "android" "armv8" "gcc" ${device_armv8}
    cd $cur

    # job 2
    build_opencl "android" "armv7" "gcc"
    adb -s $device_armv7 shell 'rm -rf /data/local/tmp/*'
    run_gen_code_test ${device_armv7}
    test_opencl "android" "armv7" "gcc" ${device_armv7}
    cd $cur

    echo "Done"
}

# We split the arm unittest into several sub-tasks to parallel and reduce the overall CI timetime.
# sub-task1
function build_test_arm_subtask_android {
    ########################################################################
    # job 1-4 must be in one runner
    prepare_adb_devices

    # job 1
    build_arm "android" "armv8" "gcc"
    adb -s $device_armv8 shell 'rm -rf /data/local/tmp/*'
    run_gen_code_test ${device_armv8}
    test_arm "android" "armv8" "gcc" ${device_armv8}
    cd -

    # job 2
    #build_arm "android" "armv8" "clang"
    #run_gen_code_test ${port_armv8}
    #test_arm "android" "armv8" "clang" ${port_armv8}
    #cd -

    # job 3
    build_arm "android" "armv7" "gcc"
    adb -s $device_armv7 shell 'rm -rf /data/local/tmp/*'
    run_gen_code_test ${device_armv7}
    test_arm "android" "armv7" "gcc" ${device_armv7}
    cd -

    # job 4
    #build_arm "android" "armv7" "clang"
    #run_gen_code_test ${port_armv7}
    #test_arm "android" "armv7" "clang" ${port_armv7}
    #cd -

    if [ $USE_ADB_EMULATOR == "ON" ]; then
        adb devices | grep emulator | cut -f1 | while read line; do adb -s $line emu kill; done
    fi
    echo "Done"
}

# sub-task2
function build_test_arm_subtask_armlinux {
    cur=$PWD
    # job 5
    build_arm "armlinux" "armv8" "gcc"
    test_arm "armlinux" "armv8" "gcc" $device_armv8
    cd $cur

    # job 6
    build_arm "armlinux" "armv7" "gcc"
    test_arm "armlinux" "armv7" "gcc" $device_armv8
    cd $cur

    # job 7
    build_arm "armlinux" "armv7hf" "gcc"
    test_arm "armlinux" "armv7hf" "gcc" $device_armv8
    cd $cur

    echo "Done"
}

# sub-task3
# this task will test IOS compiling, which requires cmake_version>=3.15
function build_test_arm_subtask_ios {
    cur=$PWD
    # job 8
    build_ios "ios" "armv7"
    cd $cur

    # job 9
    build_ios "ios64" "armv8"
    cd $cur

    echo "Done"
}

# this method need to invoke `build_test_arm_subtask_android` first.
function build_test_arm_subtask_model {
    # We just test following single one environment to limit the CI time.
    local os=android
    local abi=armv8
    local lang=gcc

    local test_name=$1
    local model_name=$2

    cur_dir=$(pwd)
    build_dir=$cur_dir/build.lite.${os}.${abi}.${lang}
    cd $build_dir
    make $test_name -j$NUM_CORES_FOR_COMPILE

    # prepare adb devices
    prepare_adb_devices
    adb -s $device_armv8 shell 'rm -rf /data/local/tmp/*'

    # just test the model on armv8
    test_arm_model $test_name $device_armv8 "./third_party/install/$model_name"

    if [ $USE_ADB_EMULATOR == "ON" ]; then
        adb devices | grep emulator | cut -f1 | while read line; do adb -s $line emu kill; done
    fi
    echo "Done"
    cd -
}


# this test load a model, optimize it and check the prediction result of both cxx and light APIS.
function test_arm_predict_apis {
    local device=$1
    local workspace=$2
    local naive_model_path=$3
    local api_test_path=$(find . -name "test_apis")
    # the model is pushed to ./lite_naive_model
    adb -s ${device} push ${naive_model_path} ${workspace}
    adb -s ${device} push $api_test_path ${workspace}

    # test cxx_api first to store the optimized model.
    adb -s ${device} shell ./test_apis --model_dir ./lite_naive_model --optimized_model ./lite_naive_model_opt
}


# Build the code and run lite arm tests. This is executed in the CI system.
function build_test_arm {
    ########################################################################
    # job 1-4 must be in one runner
    build_test_arm_subtask_android
    build_test_arm_subtask_armlinux
}

function mobile_publish {
    # only check os=android abi=armv8 lang=gcc now
    local os=android
    local abi=armv8
    local lang=gcc

    # Install java sdk tmp, remove this when Dockerfile.mobile update
    apt-get install -y --no-install-recommends default-jdk

    cur_dir=$(pwd)
    build_dir=$cur_dir/build.lite.${os}.${abi}.${lang}
    mkdir -p $build_dir
    cd $build_dir

    cmake .. \
        -DWITH_GPU=OFF \
        -DWITH_MKL=OFF \
        -DWITH_LITE=ON \
        -DLITE_WITH_CUDA=OFF \
        -DLITE_WITH_X86=OFF \
        -DLITE_WITH_ARM=ON \
        -DLITE_WITH_LIGHT_WEIGHT_FRAMEWORK=ON \
        -DWITH_TESTING=OFF \
        -DLITE_WITH_JAVA=ON \
        -DLITE_WITH_LOG=OFF \
        -DLITE_ON_TINY_PUBLISH=ON \
        -DARM_TARGET_OS=${os} -DARM_TARGET_ARCH_ABI=${abi} -DARM_TARGET_LANG=${lang}

    make publish_inference -j$NUM_CORES_FOR_COMPILE
    cd - > /dev/null
}

############################# MAIN #################################
function print_usage {
    echo -e "\nUSAGE:"
    echo
    echo "----------------------------------------"
    echo -e "cmake_x86: run cmake with X86 mode"
    echo -e "cmake_cuda: run cmake with CUDA mode"
    echo -e "--arm_os=<os> --arm_abi=<abi> cmake_arm: run cmake with ARM mode"
    echo
    echo -e "build: compile the tests"
    echo -e "--test_name=<test_name> build_single: compile single test"
    echo
    echo -e "test_server: run server tests"
    echo -e "--test_name=<test_name> --adb_port_number=<adb_port_number> test_arm_android: run arm test"
    echo "----------------------------------------"
    echo
}

function main {
    # Parse command line.
    for i in "$@"; do
        case $i in
            --tests=*)
                TESTS_FILE="${i#*=}"
                shift
                ;;
            --test_name=*)
                TEST_NAME="${i#*=}"
                shift
                ;;
            --arm_os=*)
                ARM_OS="${i#*=}"
                shift
                ;;
            --arm_abi=*)
                ARM_ABI="${i#*=}"
                shift
                ;;
            --arm_lang=*)
                ARM_LANG="${i#*=}"
                shift
                ;;
            --arm_port=*)
                ARM_PORT="${i#*=}"
                shift
                ;;
            --use_adb_emulator=*)
                USE_ADB_EMULATOR="${i#*=}"
                shift
                ;;
            --adb_device_list=*)
                ADB_DEVICE_LIST="${i#*=}"
                if [[ -n $ADB_DEVICE_LIST && $USE_ADB_EMULATOR != "OFF" ]]; then
                     set +x
                     echo
                     echo -e "Need to set USE_ADB_EMULATOR=OFF if '--adb_device_list' is specified."
                     echo
                     exit 1
                fi
                shift
                ;;
            --test_skip_list=*)
                TEST_SKIP_LIST="${i#*=}"
                shift
                ;;
            --test_model_list=*)
                TEST_MODEL_LIST="${i#*=}"
                shift
                ;;
            --lite_with_coverage=*)
                LITE_WITH_COVERAGE="${i#*=}"
                shift
                ;;
            build)
                build $TESTS_FILE
                build $LIBS_FILE
                shift
                ;;
            build_single)
                build_single $TEST_NAME
                shift
                ;;
            cmake_x86)
                cmake_x86
                shift
                ;;
            cmake_opencl)
                cmake_opencl $ARM_OS $ARM_ABI $ARM_LANG
                shift
                ;;
            cmake_cuda)
                cmake_cuda
                shift
                ;;
            cmake_arm)
                cmake_arm $ARM_OS $ARM_ABI $ARM_LANG
                shift
                ;;
            build_opencl)
                build_opencl $ARM_OS $ARM_ABI $ARM_LANG
                shift
                ;;
            build_arm)
                build_arm $ARM_OS $ARM_ABI $ARM_LANG
                shift
                ;;
            test_server)
                test_server
                shift
                ;;
            test_arm)
                test_arm $ARM_OS $ARM_ABI $ARM_LANG $ARM_PORT
                shift
                ;;
            test_arm_android)
                test_arm_android $TEST_NAME $ARM_PORT
                shift
                ;;
            test_huawei_ascend_npu)
                test_huawei_ascend_npu
                shift
                ;;
            build_test_cuda_server)
                build_test_cuda_server
                shift
                ;;
            build_test_server)
                build_test_server
                shift
                ;;
            build_check_coverage)
                build_test_coverage
                check_coverage
                shift
                ;;
            build_test_xpu)
                build_test_xpu OFF
                shift
                ;;
            build_test_xpu_with_xtcl)
                build_test_xpu ON
                shift
                ;;
            huawei_kirin_npu_build_and_test)
                huawei_kirin_npu_build_and_test $ADB_DEVICE_LIST $TEST_SKIP_LIST $TEST_MODEL_LIST
                shift
                ;;
            rockchip_npu_build_and_test)
                rockchip_npu_build_and_test $ADB_DEVICE_LIST $TEST_SKIP_LIST $TEST_MODEL_LIST
                shift
                ;;
            mediatek_apu_build_and_test)
                mediatek_apu_build_and_test $ADB_DEVICE_LIST $TEST_SKIP_LIST $TEST_MODEL_LIST
                shift
                ;;
            build_test_huawei_ascend_npu)
                build_test_huawei_ascend_npu
                shift
                ;;
            build_test_train)
                build_test_train
                shift
                ;;
            build_test_arm)
                build_test_arm
                shift
                ;;
            build_test_npu)
                build_test_npu $TEST_NAME
                shift
                ;;
            build_test_arm_opencl)
                build_test_arm_opencl
                build_test_arm_subtask_model test_mobilenetv1 mobilenet_v1
                build_test_arm_subtask_model test_mobilenetv2 mobilenet_v2_relu
                shift
                ;;
            build_test_arm_subtask_android)
                build_test_arm_subtask_android
                build_test_arm_subtask_model test_mobilenetv1 mobilenet_v1
                build_test_arm_subtask_model test_mobilenetv1_int8 MobileNetV1_quant
                build_test_arm_subtask_model test_mobilenetv2 mobilenet_v2_relu
                build_test_arm_subtask_model test_resnet50 resnet50
                build_test_arm_subtask_model test_inceptionv4 inception_v4_simple
                build_test_arm_subtask_model test_transformer_with_mask_fp32_arm transformer_with_mask_fp32
                shift
                ;;
            build_test_arm_subtask_armlinux)
                build_test_arm_subtask_armlinux
                shift
                ;;
            build_test_arm_subtask_ios)
                build_test_arm_subtask_ios
                shift
                ;;
            check_style)
                check_style
                shift
                ;;
            check_need_ci)
                check_need_ci
                shift
                ;;
            mobile_publish)
                mobile_publish
                shift
                ;;
            *)
                # unknown option
                print_usage
                exit 1
                ;;
        esac
    done
}

main $@
