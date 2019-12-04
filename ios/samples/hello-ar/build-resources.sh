#/usr/bin/env/bash

set -e

# Compile resources.

# The hello-ar app requires four resources:
# 1. The cube mesh
# 2. The Filament material definition
# 3. The Filament camera feed material definition
# 4. The Filament shadow plane material definition
# 5. The IBL image
# These will be compiled into the final binary via the resgen tool.

filamesh_path="../../../out/release/filament/bin/filamesh"
matc_path="../../../out/release/filament/bin/matc"
resgen_path="../../../out/release/filament/bin/resgen"
cmgen_path="../../../out/release/filament/bin/cmgen"

# Ensure that the required tools are present in the out/ directory.
# These can be built by running ./build.sh -p desktop -i release at Filament's root directory.

if [[ ! -e "${filamesh_path}" ]]; then
  echo "No filamesh binary could be found in ../../../out/release/filament/bin/."
  echo "Ensure Filament has been built/installed before building this app."
  exit 1
fi

if [[ ! -e "${matc_path}" ]]; then
  echo "No matc binary could be found in ../../../out/release/filament/bin/."
  echo "Ensure Filament has been built/installed before building this app."
  exit 1
fi

if [[ ! -e "${resgen_path}" ]]; then
  echo "No resgen binary could be found in ../../../out/release/filament/bin/."
  echo "Ensure Filament has been built/installed before building this app."
  exit 1
fi

if [[ ! -e "${cmgen_path}" ]]; then
  echo "No cmgen binary could be found in ../../../out/release/filament/bin/."
  echo "Ensure Filament has been built/installed before building this app."
  exit 1
fi

# The filamesh tool converts the .obj file into a Filament-specific filamesh file that has been
# optimized for loading into Filament.
mkdir -p "${PROJECT_DIR}/generated/"
"${filamesh_path}" \
    "${PROJECT_DIR}/../../../assets/models/rounded_cube/rounded_cube.obj" \
    "${PROJECT_DIR}/generated/cube.filamesh"

# The matc tool compiles the clear_coat.mat, shadow_plane.mat, and camera_feed.mat materials into
# Filament material BLOBs.

"${matc_path}" \
    --api all \
    --platform mobile \
    -o "${PROJECT_DIR}/generated/clear_coat.filamat" \
    "${PROJECT_DIR}/Materials/clear_coat.mat"

"${matc_path}" \
    --api all \
    --platform mobile \
    -o "${PROJECT_DIR}/generated/shadow_plane.filamat" \
    "${PROJECT_DIR}/Materials/shadow_plane.mat"

"${matc_path}" \
    --api all \
    --platform mobile \
    -o "${PROJECT_DIR}/generated/camera_feed.filamat" \
    "${PROJECT_DIR}/Materials/camera_feed.mat"

# cmgen consumes an HDR environment map and generates two mipmapped KTX files (IBL and skybox)
"${cmgen_path}" \
    --deploy="${PROJECT_DIR}/generated" \
    --format=ktx --size=256 --extract-blur=0.1 \
    "${PROJECT_DIR}/../../../third_party/environments/venetian_crossroads_2k.hdr"

# The resgen tool generates an assembly file, resources.apple.S that gets compiled and linked
# into the final binary. It contains all the resources consumed by the app.
"${resgen_path}" \
    --deploy="${PROJECT_DIR}/generated" \
    "${PROJECT_DIR}/generated/cube.filamesh" \
    "${PROJECT_DIR}/generated/clear_coat.filamat" \
    "${PROJECT_DIR}/generated/shadow_plane.filamat" \
    "${PROJECT_DIR}/generated/camera_feed.filamat" \
    "${PROJECT_DIR}/generated/venetian_crossroads_2k/venetian_crossroads_2k_ibl.ktx"

# FilamentApp.cpp and Resources.S include files generated by resgen.
# Touch them to force Xcode to recompile.
touch "${PROJECT_DIR}/hello-ar/FilamentArView/FullScreenTriangle.cpp"
touch "${PROJECT_DIR}/hello-ar/FilamentArView/FilamentApp.cpp"
touch "${PROJECT_DIR}/hello-ar/SupportFiles/Resources.S"
