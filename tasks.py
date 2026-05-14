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

@task
def configure(c, build_type="Debug"):
    """Configure CMake."""
    build_dir = BUILD_DIR_DEBUG if build_type.lower() == "debug" else BUILD_DIR_RELEASE
    build_dir.mkdir(parents=True, exist_ok=True)
    
    generator = "Ninja" if platform.system() != "Windows" else "MinGW Makefiles"
    
    print(f"Configuring {PROJECT_NAME} ({build_type})...")
    c.run(f"cmake -S . -B {build_dir} -G {generator} -DCMAKE_BUILD_TYPE={build_type}")

@task(help={"build_type": "Build type (Debug or Release)"})
def build(c, build_type="Debug"):
    """Compile the project."""
    configure(c, build_type=build_type)
    build_dir = BUILD_DIR_DEBUG if build_type.lower() == "debug" else BUILD_DIR_RELEASE
    print(f"Building {PROJECT_NAME} ({build_type})...")
    c.run(f"cmake --build {build_dir} --parallel {get_nproc()}")

@task
def test(c):
    """Run all unit tests (Debug mode)."""
    build(c, build_type="Debug")
    test_exe = BUILD_DIR_DEBUG / "tests" / "run_tests"
    if platform.system() == "Windows":
        test_exe = test_exe.with_suffix(".exe")
    
    print("Running tests...")
    c.run(str(test_exe))

@task(pre=[clean])
def re(c, build_type="Debug"):
    """Clean and then build."""
    build(c, build_type=build_type)

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
    
    print("Installing linuxdeploy dependencies...")
    c.run("sudo apt-get update && sudo apt-get install -y libfuse2 libxcb1 libxkbcommon0 libdbus-1-3", warn=True)
    
    print("Configuring for AppImage...")
    c.run(f"cmake -S . -B {BUILD_DIR_APPIMAGE} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=OFF")
    
    print("Compiling...")
    c.run(f"cmake --build {BUILD_DIR_APPIMAGE} --parallel {get_nproc()}")
    
    # Get the absolute path to Desktop-D30.desktop before changing directories
    desktop_file = Path(".").absolute() / "Desktop-D30.desktop"
    
    with c.cd(str(BUILD_DIR_APPIMAGE)):
        print("Installing to AppDir...")
        c.run(f"cmake --install . --prefix AppDir")
        
        # Copy the .desktop file to AppDir BEFORE running linuxdeploy
        print("Copying .desktop file to AppDir...")
        
        # Verify the desktop file exists
        if not desktop_file.exists():
            raise FileNotFoundError(f"Desktop file not found at {desktop_file}. "
                                    f"Please ensure Desktop-D30.desktop exists in the project root.")
        
        # Create the XDG standard directory structure
        appdir_share = Path("AppDir/share/applications")
        appdir_share.mkdir(parents=True, exist_ok=True)
        
        # Copy to both locations:
        # 1. Standard XDG location for linuxdeploy to discover
        shutil.copy(str(desktop_file), str(appdir_share / "Desktop-D30.desktop"))
        print(f"  ✓ Copied to AppDir/share/applications/Desktop-D30.desktop")
        
        # 2. AppDir root for linuxdeploy-plugin-appimage compatibility
        shutil.copy(str(desktop_file), "AppDir/Desktop-D30.desktop")
        print(f"  ✓ Copied to AppDir/Desktop-D30.desktop")
        
        # Download linuxdeploy
        ld_url = "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
        ld_plugin_url = "https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage"
        
        if not Path("linuxdeploy-x86_64.AppImage").exists():
            print("Downloading linuxdeploy...")
            c.run(f"wget -N {ld_url}")
        if not Path("linuxdeploy-plugin-appimage-x86_64.AppImage").exists():
            print("Downloading linuxdeploy-plugin-appimage...")
            c.run(f"wget -N {ld_plugin_url}")
            
        c.run("chmod +x linuxdeploy*.AppImage")
        
        print("Generating AppImage...")
        env = os.environ.copy()
        env.update({"ARCH": "x86_64", "OUTPUT": "Desktop-D30-x86_64.AppImage"})
        c.run("./linuxdeploy-x86_64.AppImage --appdir AppDir --output appimage", env=env)
        
        # Move back to root
        print("Checking for generated AppImage...")
        appimage_files = list(Path(".").glob("Desktop-D30*.AppImage"))
        if appimage_files:
            print(f"Found: {appimage_files[0]}, moving to root...")
            shutil.move(str(appimage_files[0]), "..")
        else:
            print("ERROR: No AppImage file was generated!")
        
    print(f"AppImage available in root directory.")

@task
def docs(c, serve=False):
    """Build or serve documentation."""
    if serve:
        print("Serving documentation...")
        c.run("mkdocs serve")
    else:
        print("Building documentation...")
        c.run("mkdocs build")

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
    if os.getuid() != 0:
        print("Error: The 'install' task must be run as root (sudo inv install).")
        return

    # Build first
    build(c, build_type="Release")
    
    print("Installing via CMake...")
    c.run(f"cmake --install {BUILD_DIR_RELEASE} --prefix /opt/Desktop-D30")
    
    print("Performing Post-Install steps...")
    opt_dir = Path("/opt/Desktop-D30")
    
    # Permissions
    c.run(f"chmod -R 755 {opt_dir}")
    
    print("Creating Launcher...")
    launcher = Path("/usr/local/bin/Desktop-D30")
    with open(launcher, "w") as f:
        f.write("#!/bin/bash\n")
        f.write(f"cd {opt_dir}/bin\n")
        f.write("./Desktop-D30\n")
    c.run(f"chmod +x {launcher}")
    
    print("Registering Desktop Icon...")
    shutil.copy("Desktop-D30.desktop", "/usr/share/applications/")
    
    print("Setting Bluetooth Permissions...")
    c.run(f"setcap 'cap_net_raw,cap_net_admin+eip' {opt_dir}/bin/Desktop-D30")
    
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
def format(c):
    """Format source code using clang-format."""
    print("Formatting code...")
    c.run("find src include tests -name '*.cpp' -o -name '*.h' | xargs clang-format -i")

@task
def lint(c):
    """Lint source code using clang-tidy."""
    # Ensure compile_commands.json exists
    if not (BUILD_DIR_DEBUG / "compile_commands.json").exists():
        configure(c, build_type="Debug")
    
    print("Linting code...")
    # Copy compile_commands.json to root for clang-tidy if needed, 
    # or just point to it. Clang-tidy likes it in the same dir or a parent.
    c.run("find src -name '*.cpp' | xargs clang-tidy -p build_debug")

@task(help={"build_type": "Build type (Debug or Release)"})
def run(c, build_type="Debug"):
    """Run the application."""
    build_dir = BUILD_DIR_DEBUG if build_type.lower() == "debug" else BUILD_DIR_RELEASE
    exe = build_dir / "Desktop-D30"
    if platform.system() == "Windows":
        exe = exe.with_suffix(".exe")
    
    if not exe.exists():
        build(c, build_type=build_type)
        
    print(f"Launching {exe}...")
    c.run(str(exe))
