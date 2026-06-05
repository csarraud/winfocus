#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────
#  build.sh — Script de compilation de WinFocus
# ─────────────────────────────────────────────────────────
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== WinFocus — Build Script ==="
echo

# ── Vérification des dépendances ──────────────────────────
check_dep() {
    if ! pkg-config --exists "$1" 2>/dev/null; then
        echo "  ✗  Dépendance manquante : $1"
        echo "     → sudo apt install ${2:-$1}"
        MISSING=1
    else
        echo "  ✓  $1 ($(pkg-config --modversion "$1"))"
    fi
}

MISSING=0
echo "── Vérification des dépendances ──"
check_dep glfw3   "libglfw3-dev"
check_dep gl      "libgl-dev"
check_dep x11     "libx11-dev"
echo

if [ "$MISSING" = "1" ]; then
    echo "Installez les dépendances manquantes, puis relancez ce script."
    echo
    echo "  sudo apt install libglfw3-dev libgl-dev libx11-dev"
    exit 1
fi

# ── Vérification des sources ImGui ───────────────────────
if [ ! -f "imgui/imgui.cpp" ]; then
    echo "── Téléchargement d'ImGui ──"
    if command -v git &>/dev/null; then
        git clone --depth=1 --branch v1.90.1 \
            https://github.com/ocornut/imgui.git imgui
        echo "  ✓  ImGui cloné"
    else
        echo "  ✗  git non trouvé."
        echo "     Téléchargez ImGui manuellement :"
        echo "     https://github.com/ocornut/imgui/archive/refs/tags/v1.90.1.tar.gz"
        echo "     Extrayez dans ./imgui/"
        exit 1
    fi
else
    echo "── ImGui déjà présent ✓ ──"
fi
echo

# ── Compilation ───────────────────────────────────────────
echo "── Compilation ──"
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -j"$(nproc)"
cd ..
echo
echo "══════════════════════════════════"
echo "  ✓  Build terminé !"
echo "     Lancement : ./build/winfocus"
echo "══════════════════════════════════"
