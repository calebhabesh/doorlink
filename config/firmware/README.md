# Firmware Build Configurations

Keep `sdkconfig.defaults` and `partitions.csv` at the project root for the normal ESP-IDF build. The small `sdkconfig.*.defaults` files here are optional overlays for production, microphone, wake, and camera experiments. Pass an overlay after the root defaults file with `SDKCONFIG_DEFAULTS`, as shown in [Firmware Architecture](../../docs/firmware-architecture.md).

The full `sdkconfig.revc-camera-*` files are archived configurations from Rev C camera bring-up. They are retained for reference and are not part of the default build.
