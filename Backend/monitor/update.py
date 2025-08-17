import shutil
import os

# === Update these paths ===
bin_src = r"C:\Users\Test-August2023\AppData\Local\arduino\sketches\7B03D7D7FDDC0CD685DB6C9B053B154D\firmware.ino.bin"
django_static_dir = r"C:\Users\Test-August2023\Desktop\airquality\monitor\static"

# === Versioning logic ===
version_file = os.path.join(django_static_dir, "version.txt")
firmware_dst = os.path.join(django_static_dir, "firmware.ino.bin")

# Step 1: Read current version
if os.path.exists(version_file):
    with open(version_file, "r") as vf:
        current_version = vf.read().strip()
else:
    current_version = "1.0.0"  # Default version if the file doesn't exist

# Step 2: Auto-increment patch version
major, minor, patch = map(int, current_version.split("."))
patch += 1
new_version = f"{major}.{minor}.{patch}"

# Step 3: Copy firmware and write version
shutil.copy2(bin_src, firmware_dst)
with open(version_file, "w") as vf:
    vf.write(new_version)

print(f"✅ Firmware updated to version {new_version} and copied to static directory.")
