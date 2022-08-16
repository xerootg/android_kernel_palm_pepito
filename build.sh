export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
mkdir -p out
make O=out clean
make O=out mrproper
make O=out <defconfig_name>
make O=out -j$(nproc --all)