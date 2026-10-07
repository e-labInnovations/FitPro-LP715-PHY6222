# Build environment for PHY6222 firmware: ARM GCC + the Phyplus PHY62x2 SDK.
# gcc-arm-none-eabi is packaged for both amd64 and arm64, so no platform pin is
# needed — the image runs natively on Apple Silicon, Linux and Windows hosts.
FROM debian:bookworm-slim

RUN apt-get update && \
	apt-get install -y --no-install-recommends \
		gcc-arm-none-eabi libnewlib-arm-none-eabi make python3 git ca-certificates && \
	rm -rf /var/lib/apt/lists/*

# The SDK comes from amir1387aht/phy6222_smartwatch, pinned so builds are
# reproducible. Only its sdk/ directory is kept; the app code there is for a
# different board (ST7735 panel) and our own scaffold lives in lib/core.
ARG SDK_REPO=https://github.com/amir1387aht/phy6222_smartwatch
ARG SDK_COMMIT=02cc0c961d926506ee577454d7bc1286bdb3d803
RUN git clone -q $SDK_REPO /tmp/src && \
	git -C /tmp/src checkout -q $SDK_COMMIT && \
	mv /tmp/src/sdk /opt/phy6222_sdk && \
	rm -rf /tmp/src && \
	# The repo was built on Windows; some includes differ from the files only in case.
	cp /opt/phy6222_sdk/components/osal/include/OSAL.h /opt/phy6222_sdk/components/osal/include/osal.h

WORKDIR /src

ENV SDK=/opt/phy6222_sdk
