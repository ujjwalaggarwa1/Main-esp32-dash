import os
import shutil
import hashlib
import json
import re

Import("env")

def post_build_action(source, target, env):
    project_dir = env.subst("$PROJECT_DIR")
    build_dir = env.subst("$BUILD_DIR")
    firmware_bin = os.path.join(build_dir, "firmware.bin")
    
    if not os.path.exists(firmware_bin):
        return

    compiled_dir = os.path.join(project_dir, "compiled")
    os.makedirs(compiled_dir, exist_ok=True)
    
    dest_bin = os.path.join(compiled_dir, "firmware.bin")
    shutil.copyfile(firmware_bin, dest_bin)
    
    # Calculate SHA256 of the binary (first 16 hex chars)
    hasher = hashlib.sha256()
    with open(dest_bin, "rb") as f:
        hasher.update(f.read())
    bin_hash = hasher.hexdigest()[:16]
    
    # Read current version from src/app_config.h
    version = "1.0.0"
    config_h = os.path.join(project_dir, "src", "app_config.h")
    if os.path.exists(config_h):
        with open(config_h, "r", encoding="utf-8") as f:
            content = f.read()
            m = re.search(r'#define\s+FIRMWARE_CURRENT_VERSION\s+"([^"]+)"', content)
            if m:
                version = m.group(1)
                
    # Read existing bin_url if already set
    version_json_path = os.path.join(compiled_dir, "version.json")
    bin_url = "https://raw.githubusercontent.com/<username>/<repo>/main/compiled/firmware.bin"
    if os.path.exists(version_json_path):
        try:
            with open(version_json_path, "r", encoding="utf-8") as f:
                old_data = json.load(f)
                if "bin_url" in old_data and "<username>" not in old_data["bin_url"]:
                    bin_url = old_data["bin_url"]
        except Exception:
            pass

    manifest = {
        "version": version,
        "hash": bin_hash,
        "bin_url": bin_url,
        "notes": f"Automated build: firmware v{version} (hash: {bin_hash})"
    }
    
    with open(version_json_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        
    print(f"\n[PostBuild] Synced to compiled/: firmware.bin & version.json (v{version}, hash: {bin_hash})\n")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_build_action)
