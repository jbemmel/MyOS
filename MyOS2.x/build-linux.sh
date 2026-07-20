#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

build_dir=${BUILD_DIR:-build/debug}
generated_dir=build/gen_src

mkdir -p "$build_dir" "$generated_dir/util"

command -v javac >/dev/null 2>&1 || {
  echo "javac is required to build the XSLT helper" >&2
  exit 1
}
javac -d "$generated_dir" buildtools/src/util/Capitalizer.java

classpath="buildtools/lib/saxon7.jar:$generated_dir"
generate() {
  local stylesheet=$1 input_dir=$2
  while IFS= read -r -d '' input; do
    java -cp "$classpath" net.sf.saxon.Transform \
      -o "build/${stylesheet}.out" "$input" "xslt/$stylesheet"
  done < <(find "$input_dir" -maxdepth 1 -name '*.xml' -print0)
}

generate cg_interface.xsl xml/Interfaces
generate cg_device.xsl xml/Devices
generate cg_component.xsl xml/Components
generate cg_service.xsl xml/Services
generate cg_driver.xsl xml/Drivers
generate cg_container.xsl xml/Containers
java -cp "$classpath" net.sf.saxon.Transform \
  -o build/cg_config.xsl.out xml/myosconfig.xml xslt/cg_config.xsl
# Sources use both historical Windows spellings; Linux is case-sensitive.
cp "$generated_dir/myosconfig.h" "$generated_dir/MyOSConfig.h"

cxx=${CXX:-g++}
jobs=${JOBS:-$(nproc)}
case $jobs in
  ''|*[!0-9]*|0) echo "JOBS must be a positive integer" >&2; exit 2 ;;
esac
common_flags=(
  -m32 -std=gnu++98 -O -DDEBUG -DMYOS -mregparm=3 -fno-rtti
  -fomit-frame-pointer -fno-use-cxa-atexit -fno-stack-protector -fexceptions
  -fnon-call-exceptions -fmessage-length=0 -falign-functions=8
  -nostdinc -nostdinc++ -fno-threadsafe-statics -fno-pie -fpermissive
  -fno-access-control
  -Wno-write-strings -Isrc/headers -Isrc -Ibuild/gen_src
)

sources=()
while IFS= read -r -d '' source; do sources+=("$source"); done \
  < <(find src -name '*.cpp' ! -path 'src/Tests/*' -print0)
while IFS= read -r -d '' source; do sources+=("$source"); done \
  < <(find src/Tests -maxdepth 1 -name '*.cpp' -print0)
while IFS= read -r -d '' source; do sources+=("$source"); done \
  < <(find src/Tests/WebServer -name '*.cpp' -print0)
while IFS= read -r -d '' source; do sources+=("$source"); done \
  < <(find "$generated_dir" -name '*.cpp' -print0)

declare -A object_sources=()
for source in "${sources[@]}"; do
  object_name=$(basename "${source%.cpp}").o
  if [[ -n ${object_sources[$object_name]+x} ]]; then
    echo "Object name collision: ${object_sources[$object_name]} and $source" >&2
    exit 1
  fi
  object_sources[$object_name]=$source
done

compile_source() {
  local source=$1
  local object="$build_dir/$(basename "${source%.cpp}").o"
  echo "CXX $source"
  "$cxx" "${common_flags[@]}" -c "$source" -o "$object"
}

pids=()
for source in "${sources[@]}"; do
  compile_source "$source" &
  pids+=("$!")
  if ((${#pids[@]} == jobs)); then
    compile_failed=0
    for pid in "${pids[@]}"; do wait "$pid" || compile_failed=1; done
    ((compile_failed == 0)) || exit 1
    pids=()
  fi
done
compile_failed=0
for pid in "${pids[@]}"; do wait "$pid" || compile_failed=1; done
((compile_failed == 0)) || exit 1

archive_objects=()
for object in "$build_dir"/*.o; do
  case $(basename "$object") in
    DoDecompress.o|BootContext.o|MyOSMain.o|ZipContext.o) ;;
    *) archive_objects+=("$object") ;;
  esac
done
rm -f "$build_dir/libAllobjs.a"
ar rcs "$build_dir/libAllobjs.a" "${archive_objects[@]}"
(
  cd "$build_dir"
  ld -m elf_i386 -T ../../buildtools/MyOS.linkscript -Map MyOS.map \
    --gc-sections --strip-discarded -Bstatic -e _MyOSmain -o kernel.elf \
    --whole-archive libAllobjs.a --no-whole-archive
)
size "$build_dir/kernel.elf"

objcopy --only-section=UNCOMPRESSED -O binary \
  "$build_dir/kernel.elf" "$build_dir/kernel.bin.loader"
objcopy -g -S --remove-section=UNCOMPRESSED -R .bss -R .stack -O binary \
  "$build_dir/kernel.elf" "$build_dir/kernel.bin.core"
gzip -9 -f "$build_dir/kernel.bin.core"
cp src/Init/bootphase2 "$build_dir/kernel.bin"
dd if="$build_dir/kernel.bin.loader" of="$build_dir/kernel.bin" \
  oflag=append conv=notrunc status=none
dd if="$build_dir/kernel.bin.core.gz" of="$build_dir/kernel.bin" \
  oflag=append conv=notrunc status=none

echo "Built MyOS2.x kernel: $build_dir/kernel.elf and $build_dir/kernel.bin"
