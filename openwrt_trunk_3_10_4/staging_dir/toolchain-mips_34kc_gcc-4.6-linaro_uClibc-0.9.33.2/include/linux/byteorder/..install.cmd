cmd_/home/shizhai/zhongwang/trunk/build_dir/toolchain-mips_34kc_gcc-4.6-linaro_uClibc-0.9.33.2/linux-dev//include/linux/byteorder/.install := /bin/bash scripts/headers_install.sh /home/shizhai/zhongwang/trunk/build_dir/toolchain-mips_34kc_gcc-4.6-linaro_uClibc-0.9.33.2/linux-dev//include/linux/byteorder   /home/shizhai/zhongwang/trunk/build_dir/toolchain-mips_34kc_gcc-4.6-linaro_uClibc-0.9.33.2/linux-3.10.15/include/uapi/linux/byteorder/big_endian.h   /home/shizhai/zhongwang/trunk/build_dir/toolchain-mips_34kc_gcc-4.6-linaro_uClibc-0.9.33.2/linux-3.10.15/include/uapi/linux/byteorder/little_endian.h ; for F in ; do echo "\#include <asm-generic/$$F>" > /home/shizhai/zhongwang/trunk/build_dir/toolchain-mips_34kc_gcc-4.6-linaro_uClibc-0.9.33.2/linux-dev//include/linux/byteorder/$$F; done; touch /home/shizhai/zhongwang/trunk/build_dir/toolchain-mips_34kc_gcc-4.6-linaro_uClibc-0.9.33.2/linux-dev//include/linux/byteorder/.install