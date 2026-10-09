# Build environment for PHY6222 firmware: ARM GCC and make, nothing else.
# The SDK is not in the image: it lives in this repo (sdk/phy6222) and reaches
# the container through the repo mount, so SDK patches need no image rebuild.
#
# gcc-arm-none-eabi is packaged for both amd64 and arm64, so no platform pin is
# needed — the image runs natively on Apple Silicon, Linux and Windows hosts.
FROM debian:bookworm-slim

# Debian's toolchain ships libraries for every ARM core (~750 MB). The PHY6222
# is a Cortex-M0, which only ever links thumb/v6-m/nofp, so everything else
# (including the default ARM-mode libraries at the top level) is deleted in the
# same layer — along with C++, LTO and the docs.
RUN set -e; \
	apt-get update; \
	apt-get install -y --no-install-recommends gcc-arm-none-eabi libnewlib-arm-none-eabi make; \
	rm -rf /var/lib/apt/lists/* /usr/share/doc /usr/share/man /usr/share/info /usr/share/locale; \
	gcc=/usr/lib/gcc/arm-none-eabi/*; \
	newlib=/usr/lib/arm-none-eabi/newlib; \
	for d in $gcc $newlib; do \
		rm -rf $d/arm; \
		find $d/thumb -mindepth 1 -maxdepth 1 ! -name v6-m -exec rm -rf {} +; \
		rm -f $d/*.a $d/*.o; \
	done; \
	rm -rf $gcc/cc1plus $gcc/lto1 $gcc/plugin /usr/bin/arm-none-eabi-lto-dump \
		/usr/bin/arm-none-eabi-c++ /usr/bin/arm-none-eabi-g++; \
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -print-multi-directory | grep -qx thumb/v6-m/nofp

WORKDIR /src
