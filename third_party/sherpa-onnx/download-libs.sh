#!/usr/bin/env bash
#
# 拉取 sherpa-onnx 的 Android arm64-v8a 预编译库。
#
# 这两个 .so 不入版本库（.gitignore 全局排除 *.so），首次构建前在构建机上跑一次。
# 详见同目录 README.md —— 特别是「为什么是两个 .so 而不是一个」。
#
# 用法（在仓库根目录或任意位置均可）：
#     bash third_party/sherpa-onnx/download-libs.sh
#
# 幂等：已存在且校验通过时直接跳过，不重复下载。

set -euo pipefail

SHERPA_ONNX_VERSION="1.13.7"
ABI="arm64-v8a"

# 非静态链接 ONNX Runtime 的产物 —— 唯一含 C API 库的那份（见 README.md）
ARCHIVE_NAME="sherpa-onnx-v${SHERPA_ONNX_VERSION}-android.tar.bz2"
ARCHIVE_URL="https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_ONNX_VERSION}/${ARCHIVE_NAME}"
ARCHIVE_SHA256="7208b26f5109777d1c3c22a45f04665419d6f3ea81f6811a2dfd5ff2d7622e6e"
ARCHIVE_SIZE=45287000

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB_DIR="${SCRIPT_DIR}/jniLibs/${ABI}"

LIB_C_API="libsherpa-onnx-c-api.so"
LIB_ORT="libonnxruntime.so"

declare -A EXPECTED_SHA256=(
    ["${LIB_C_API}"]="73dba26ddf63e47e6c6e8b7c663d50c834b267ec0bb1ada1bc8e472e390bc41a"
    ["${LIB_ORT}"]="dc5e4c172b1be9e530c6a62ad8f1be3e0a911cabdee6195abf28dab72477e194"
)

die() { echo "错误: $*" >&2; exit 1; }
info() { echo "[sherpa-onnx] $*"; }

sha256_of() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    else
        die "找不到 sha256sum 或 shasum，无法校验下载内容"
    fi
}

verify_lib() {
    local path="$1" name="$2" want="${EXPECTED_SHA256[$2]}"
    [[ -f "${path}" ]] || return 1
    local got
    got="$(sha256_of "${path}")"
    if [[ "${got}" != "${want}" ]]; then
        info "校验失败，将重新下载: ${name}"
        info "  期望 ${want}"
        info "  实际 ${got}"
        rm -f "${path}"
        return 1
    fi
    return 0
}

# ---- 已就绪则直接退出 ----
if verify_lib "${LIB_DIR}/${LIB_C_API}" "${LIB_C_API}" \
   && verify_lib "${LIB_DIR}/${LIB_ORT}" "${LIB_ORT}"; then
    info "预编译库已就绪且校验通过: ${LIB_DIR}"
    exit 0
fi

mkdir -p "${LIB_DIR}"

# ---- 下载 ----
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT
ARCHIVE="${TMP_DIR}/${ARCHIVE_NAME}"

info "下载 ${ARCHIVE_NAME} (${ARCHIVE_SIZE} B) ..."
if command -v curl >/dev/null 2>&1; then
    curl -fL --retry 3 --retry-delay 2 -o "${ARCHIVE}" "${ARCHIVE_URL}"
elif command -v wget >/dev/null 2>&1; then
    wget -O "${ARCHIVE}" "${ARCHIVE_URL}"
else
    die "找不到 curl 或 wget"
fi

# ---- 校验归档 ----
ACTUAL_SIZE="$(wc -c < "${ARCHIVE}" | tr -d ' ')"
[[ "${ACTUAL_SIZE}" == "${ARCHIVE_SIZE}" ]] \
    || die "归档大小不符: 期望 ${ARCHIVE_SIZE}，实际 ${ACTUAL_SIZE}"

ACTUAL_SHA="$(sha256_of "${ARCHIVE}")"
[[ "${ACTUAL_SHA}" == "${ARCHIVE_SHA256}" ]] \
    || die "归档 sha256 不符: 期望 ${ARCHIVE_SHA256}，实际 ${ACTUAL_SHA}"

# ---- 只取需要的两个 .so ----
info "解压 arm64-v8a 的两个 .so ..."
tar -xjf "${ARCHIVE}" -C "${LIB_DIR}" --strip-components=2 \
    "jniLibs/${ABI}/${LIB_C_API}" \
    "jniLibs/${ABI}/${LIB_ORT}"

# ---- 逐个校验 ----
for name in "${LIB_C_API}" "${LIB_ORT}"; do
    verify_lib "${LIB_DIR}/${name}" "${name}" \
        || die "${name} 校验失败"
done

info "完成: ${LIB_DIR}"
ls -l "${LIB_DIR}"
