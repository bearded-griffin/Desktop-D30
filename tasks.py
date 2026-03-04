from invoke import task
import os
import shutil
import platform
import subprocess
from pathlib import Path

# --- CONFIGURATION ---
PROJECT_NAME = "Desktop-D30"
BUILD_DIR = Path("build")
BUILD_DIR_DEBUG = Path("build_debug")
BUILD_DIR_RELEASE = Path("build_release")
BUILD_DIR_APPIMAGE = Path("build_appimage")
BUILD_DIR_WIN_CROSS = Path("build_win_cross")
COVERAGE_DIR = Path("coverage_report")

def get_nproc():
    try:
        return os.cpu_count() or 1
    except:
        return 1

@task
def clean(c):
    """Nuke all build directories."""
    dirs_to_clean = [
        BUILD_DIR, 
        BUILD_DIR_DEBUG, 
        BUILD_DIR_RELEASE, 
        BUILD_DIR_APPIMAGE, 
        BUILD_DIR_WIN_CROSS,
        COVERAGE_DIR
    ]
    for d in dirs_to_clean:
        if d.exists():
            print(f"Cleaning {d}...")
            shutil.rmtree(d)
    
    # Clean AppImages in root
    for f in Path(".").glob("Desktop-D30*.AppImage"):
        print(f"Removing {f}...")
        f.unlink()

@task(help={"type": "Build type (Debug or Release)"})
def configure(c, type="Debug"):
    """Configure CMake."""
    build_dir = BUILD_DIR_DEBUG if type.lower() == "debug" else BUILD_DIR_RELEASE
    build_dir.mkdir(parents=True, exist_ok=True)
    
    generator = "Ninja" if platform.system() != "Windows" else "MinGW Makefiles"
    
    print(f"Configuring {PROJECT_NAME} ({type})...")
    c.run(f"cmake -S . -B {build_dir} -G {generator} -DCMAKE_BUILD_TYPE={type}")

@task(configure)
def build(c, type="Debug"):
    """Compile the project."""
    build_dir = BUILD_DIR_DEBUG if type.lower() == "debug" else BUILD_DIR_RELEASE
    print(f"Building {PROJECT_NAME} ({type})...")
    c.run(f"cmake --build {build_dir} --parallel {get_nproc()}")

@task(pre=[lambda c: build(c, type="Debug")])
def test(c):
    """Run all unit tests (Debug mode)."""
    test_exe = BUILD_DIR_DEBUG / "tests" / "run_tests"
    if platform.system() == "Windows":
        test_exe = test_exe.with_suffix(".exe")
    
    print("Running tests...")
    c.run(str(test_exe))

@task(pre=[clean])
def coverage(c):
    """Generate code coverage report (HTML)."""
    # Build with coverage
    build(c, type="Debug")
    test(c)
    
    print("Collecting coverage data...")
    COVERAGE_DIR.mkdir(exist_ok=True)
    
    c.run(f"lcov --capture --directory {BUILD_DIR_DEBUG} "
          f"--output-file {COVERAGE_DIR}/coverage.info "
          f"--ignore-errors mismatch "
          f"--exclude '{BUILD_DIR_DEBUG}/_deps/*' "
          f"--exclude '/usr/*'")
    
    print("Generating HTML report...")
    c.run(f"genhtml {COVERAGE_DIR}/coverage.info --output-directory {COVERAGE_DIR}")
    
    index_path = COVERAGE_DIR / "index.html"
    print(f"Coverage report generated in {COVERAGE_DIR}")
    
    # Try to open
    if platform.system() == "Linux":
        c.run(f"xdg-open {index_path}", warn=True)
    elif platform.system() == "Darwin":
        c.run(f"open {index_path}", warn=True)

@task
def release(c):
    """Build and package a release (DEB and TGZ)."""
    # Clean and build
    if BUILD_DIR_RELEASE.exists():
        shutil.rmtree(BUILD_DIR_RELEASE)
    
    BUILD_DIR_RELEASE.mkdir(parents=True, exist_ok=True)
    
    print("Configuring Release build...")
    c.run(f"cmake -S . -B {BUILD_DIR_RELEASE} -DCMAKE_BUILD_TYPE=Release")
    
    print("Compiling...")
    c.run(f"cmake --build {BUILD_DIR_RELEASE} --parallel {get_nproc()}")
    
    print("Packaging (DEB;TGZ)...")
    with c.cd(str(BUILD_DIR_RELEASE)):
        c.run("cpack -G 'DEB;TGZ'")
    
    print(f"Release packages available in {BUILD_DIR_RELEASE}")

@task
def appimage(c):
    """Build and package as an AppImage."""
    if BUILD_DIR_APPIMAGE.exists():
        shutil.rmtree(BUILD_DIR_APPIMAGE)
    BUILD_DIR_APPIMAGE.mkdir(parents=True, exist_ok=True)
    
    print("Configuring for AppImage...")
    c.run(f"cmake -S . -B {BUILD_DIR_APPIMAGE} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=OFF")
    
    print("Compiling...")
    c.run(f"cmake --build {BUILD_DIR_APPIMAGE} --parallel {get_nproc()}")
    
    with c.cd(str(BUILD_DIR_APPIMAGE)):
        print("Installing to AppDir...")
        c.run(f"make install DESTDIR=AppDir") # CMake install uses make under the hood if not specified
        
        # Download linuxdeploy
        ld_url = "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
        ld_plugin_url = "https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage"
        
        if not Path("linuxdeploy-x86_64.AppImage").exists():
            c.run(f"wget -N {ld_url}")
        if not Path("linuxdeploy-plugin-appimage-x86_64.AppImage").exists():
            c.run(f"wget -N {ld_plugin_url}")
            
        c.run("chmod +x linuxdeploy*.AppImage")
        
        print("Generating AppImage...")
        env = {"ARCH": "x86_64", "OUTPUT": "Desktop-D30-x86_64.AppImage"}
        c.run("./linuxdeploy-x86_64.AppImage --appdir AppDir --output appimage", env=env)
        
        # Move back to root
        appimage_file = list(Path(".").glob("Desktop-D30*.AppImage"))[0]
        shutil.move(str(appimage_file), "..")
        
    print(f"AppImage available in root directory.")

@task
def cross_windows(c):
    """Cross-compile for Windows from Linux using MinGW."""
    if BUILD_DIR_WIN_CROSS.exists():
        shutil.rmtree(BUILD_DIR_WIN_CROSS)
    BUILD_DIR_WIN_CROSS.mkdir(parents=True, exist_ok=True)
    
    print("Configuring for Windows Cross-Compilation...")
    c.run(f"cmake -S . -B {BUILD_DIR_WIN_CROSS} -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake -DCMAKE_BUILD_TYPE=Release")
    
    print("Compiling for Windows...")
    c.run(f"cmake --build {BUILD_DIR_WIN_CROSS} --parallel {get_nproc()}")
    
    print(f"Windows build available in {BUILD_DIR_WIN_CROSS}")

@task
def install(c):
    """Build and install to the system (requires sudo)."""
    # Check for root
    if os.getuid() != 0:
        print("Error: The 'install' task must be run as root (sudo inv install).")
        return

    # Build first
    build(c, type="Release")
    
    print("Installing to /opt/Desktop-D30...")
    opt_dir = Path("/opt/Desktop-D30")
    opt_dir.mkdir(parents=True, exist_ok=True)
    
    shutil.copy(BUILD_DIR_RELEASE / "Desktop-D30", opt_dir)
    if opt_dir / "assets" in [p for p in opt_dir.iterdir() if p.is_dir()]:
        shutil.rmtree(opt_dir / "assets")
    shutil.copytree("assets", opt_dir / "assets")
    
    # Permissions
    c.run(f"chmod -R 755 {opt_dir}")
    
    print("Creating Launcher...")
    launcher = Path("/usr/local/bin/Desktop-D30")
    with open(launcher, "w") as f:
        f.write("#!/bin/bash\n")
        f.write(f"cd {opt_dir}\n")
        f.write("./Desktop-D30\n")
    c.run(f"chmod +x {launcher}")
    
    print("Registering Desktop Icon...")
    shutil.copy("Desktop-D30.desktop", "/usr/share/applications/")
    
    print("Setting Bluetooth Permissions...")
    c.run(f"setcap 'cap_net_raw,cap_net_admin+eip' {opt_dir}/Desktop-D30")
    
    print("Installation complete!")

@task
def metadata(c):
    """Run all asset metadata generation scripts."""
    scripts = [
        "scripts/generate_metadata.py",
        "scripts/generate_borders.py"
    ]
    for script in scripts:
        print(f"Running {script}...")
        c.run(f"python3 {script}")

@task
def run(c, type="Debug"):
    """Run the application."""
    build_dir = BUILD_DIR_DEBUG if type.lower() == "debug" else BUILD_DIR_RELEASE
    exe = build_dir / "Desktop-D30"
    if platform.system() == "Windows":
        exe = exe.with_suffix(".exe")
    
    if not exe.exists():
        build(c, type=type)
        
    print(f"Launching {exe}...")
    c.run(str(exe))
