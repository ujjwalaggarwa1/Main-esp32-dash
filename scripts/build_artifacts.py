#!/usr/bin/env python3
"""
Automated Build Packaging Script for Master Helper (ESP32-C3 SuperMini)
Compiles firmware and exports release binaries and version manifest to compiled/ and compile/ directories.
"""

import os
import sys
import json
import shutil
import subprocess
from datetime import datetime, timezone

WORKSPACE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BUILD_DIR = os.path.join(WORKSPACE_ROOT, ".pio", "build", "esp32-c3-devkitm-1")
COMPILED_DIR = os.path.join(WORKSPACE_ROOT, "compiled")
COMPILE_DIR = os.path.join(WORKSPACE_ROOT, "compile")

# Default manifest settings
RAW_GITHUB_BASE = "https://raw.githubusercontent.com/ujjwalaggarwa1/Main-esp32-dash/main/compiled"

def find_pio_executable():
    pio_path = shutil.which("pio")
    if pio_path:
        return pio_path
    
    user_home = os.path.expanduser("~")
    cand = os.path.join(user_home, ".platformio", "penv", "Scripts", "pio.exe")
    if os.path.exists(cand):
        return cand
        
    return "pio"

def get_git_commit_hash():
    try:
        res = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=WORKSPACE_ROOT,
            capture_output=True,
            text=True,
            check=True
        )
        return res.stdout.strip()
    except Exception:
        return "c3-v100-" + datetime.now(timezone.utc).strftime("%Y%m%d%H%M")

def get_firmware_version():
    app_config_path = os.path.join(WORKSPACE_ROOT, "src", "app_config.h")
    if os.path.exists(app_config_path):
        with open(app_config_path, "r", encoding="utf-8") as f:
            for line in f:
                if "#define FIRMWARE_CURRENT_VERSION" in line:
                    parts = line.split('"')
                    if len(parts) >= 2:
                        return parts[1]
    return "1.0.0"

def build_firmware():
    pio_exe = find_pio_executable()
    print(f"[*] Invoking PlatformIO compiler via: {pio_exe}")
    cmd = [pio_exe, "run"]
    res = subprocess.run(cmd, cwd=WORKSPACE_ROOT)
    if res.returncode != 0:
        print("[!] Build failed with exit code:", res.returncode)
        sys.exit(res.returncode)
    print("[+] PlatformIO compilation successful!")

def package_artifacts():
    os.makedirs(COMPILED_DIR, exist_ok=True)
    os.makedirs(COMPILE_DIR, exist_ok=True)

    artifacts = [
        ("firmware.bin", "Application firmware binary"),
        ("bootloader.bin", "Second-stage bootloader binary"),
        ("partitions.bin", "4MB Dual-OTA partition table binary")
    ]

    copied_files = []
    for filename, desc in artifacts:
        src = os.path.join(BUILD_DIR, filename)
        if os.path.exists(src):
            dst1 = os.path.join(COMPILED_DIR, filename)
            dst2 = os.path.join(COMPILE_DIR, filename)
            shutil.copy2(src, dst1)
            shutil.copy2(src, dst2)
            copied_files.append((filename, os.path.getsize(src)))
            print(f"[+] Exported {desc}: {filename} ({os.path.getsize(src)} bytes)")
        else:
            print(f"[!] Warning: Artifact {src} not found!")

    # Generate version.json
    version = get_firmware_version()
    commit_hash = get_git_commit_hash()
    
    manifest = {
        "version": version,
        "hash": commit_hash,
        "bin_url": f"{RAW_GITHUB_BASE}/firmware.bin",
        "notes": f"Automated release for ESP32-C3 SuperMini compiled on {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S UTC')}"
    }

    manifest_json = json.dumps(manifest, indent=2)

    for target_dir in [COMPILED_DIR, COMPILE_DIR]:
        manifest_path = os.path.join(target_dir, "version.json")
        with open(manifest_path, "w", encoding="utf-8") as f:
            f.write(manifest_json)
        print(f"[+] Generated manifest: {manifest_path}")

    print("\n[=== Artifact Packaging Complete ===]")
    print(f"Version:      {version}")
    print(f"Commit Hash:  {commit_hash}")
    print(f"Release URL:  {manifest['bin_url']}")
    print(f"Outputs in:   {COMPILED_DIR} & {COMPILE_DIR}")

def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--package-only":
        package_artifacts()
    else:
        build_firmware()
        package_artifacts()

if __name__ == "__main__":
    main()
