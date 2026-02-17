# NetView Network Monitor - Complete Installation Project

## 🎯 Project Overview

This is a **complete, production-ready Windows application** with professional installer integration. The project includes:

- ✅ Network monitoring application with process control
- ✅ Professional Inno Setup installer
- ✅ Automatic startup registration (via Windows Task Scheduler)
- ✅ Embedded application icon
- ✅ License agreement with mandatory acceptance
- ✅ First-run experience
- ✅ Complete uninstaller
- ✅ Admin elevation (install only, not runtime)

---

## 📁 Project Structure

```
NetViewInstaller/
├── src/                           # Application source code
│   ├── main.cpp                   # Entry point with first-run logic
│   ├── AutoStartManager.h/.cpp    # Auto-start registration
│   ├── MonitorWidget.h/.cpp       # Main overlay widget
│   ├── ProcessManager.h/.cpp      # Process detection & blocking
│   ├── ProcessListWindow.h/.cpp   # Process control UI
│   ├── DataManager.h/.cpp         # Network statistics
│   ├── UIHelper.h/.cpp            # UI utilities
│   └── resource.rc                # Icon resource file
│
├── resources/                     # Images and icons
│   ├── ICON_REQUIREMENTS.md       # Instructions for adding icons
│   ├── app_icon.ico              # ⚠️ YOU MUST ADD THIS
│   ├── installer_banner.bmp      # Optional: Welcome banner
│   └── installer_logo.bmp        # Optional: Sidebar logo
│
├── installer/                     # Inno Setup configuration
│   └── NetView_Setup.iss          # Complete installer script
│
├── docs/                          # Documentation
│   ├── LICENSE.txt                # EULA shown during install
│   ├── README_INSTALLER.txt       # Pre-install information
│   └── README.txt                 # User guide (installed)
│
├── CMakeLists.txt                 # Build configuration
├── build.bat                      # Application build script
├── build_installer.bat            # Installer build script
└── PROJECT_README.md              # This file
```

---

## 🚀 Quick Start Guide

### Step 1: Add Application Icon

**CRITICAL**: Before building, you must add an application icon.

1. Create or download a network-themed icon
2. Convert to `.ico` format with multiple sizes (16, 32, 48, 256)
3. Save as: `resources/app_icon.ico`

**Icon Resources:**
- Free icons: https://www.flaticon.com (search "network monitor")
- Converter: https://convertico.com/
- See: `resources/ICON_REQUIREMENTS.md` for details

### Step 2: Build the Application

```batch
build.bat
```

This will:
- Configure CMake project
- Compile with Visual Studio
- Embed the icon
- Create: `build/bin/Release/NetView.exe`

### Step 3: Build the Installer

First, install **Inno Setup 6**:
- Download: https://jrsoftware.org/isinfo.php
- Install to default location

Then run:
```batch
build_installer.bat
```

This creates: `output/NetView_Setup_v1.0.0.exe`

---

## 📋 Installer Features

### Installation Flow

1. **Welcome Page**
   - Shows app icon
   - Professional branding

2. **Information Page**
   - Displays comprehensive README
   - Explains features and requirements
   
3. **License Agreement**
   - Shows EULA
   - User must accept to continue

4. **Terms Acceptance Page** ✨
   - **Mandatory checkbox**: "I have read and accept the terms"
   - Install button **disabled** until checked
   - Prevents accidental installation

5. **Installation Directory**
   - Default: `C:\Program Files\NetView`
   - User can change

6. **Ready to Install**
   - Summary of settings
   - Startup option checkbox (enabled by default)

7. **Installing**
   - Progress bar
   - File extraction
   - Registry configuration
   - Auto-start setup

8. **Finish**
   - Option to launch NetView immediately
   - Application starts and shows first-run dialog

### What the Installer Does

✅ **One-Time Admin Elevation**
- Requests admin privileges during installation
- Configures system-level components
- After install, app runs without UAC prompts

✅ **Auto-Start Configuration**
- Creates Windows Scheduled Task
- Name: "NetViewMonitor"
- Trigger: User logon
- Runs with highest privileges
- No UAC prompts on startup

✅ **First-Run Experience**
- Detects first launch after installation
- Shows welcome dialog
- Validates permissions
- Initializes configuration

✅ **Clean Uninstallation**
- Removes all files
- Deletes scheduled task
- Cleans registry entries
- Removes application data

---

## 🔧 Technical Details

### Auto-Start Implementation

**Method**: Windows Task Scheduler (Industry Standard)

**Advantages:**
- No UAC prompts on startup
- Runs with elevated privileges if needed
- User can easily manage via Task Scheduler
- Survives Windows updates
- Reliable and secure

**Alternative methods NOT used:**
- ❌ Registry Run key (limited privileges)
- ❌ Startup folder (user-level only)
- ❌ Windows Service (overkill for UI app)

### Admin Privileges Strategy

**Installation Phase** (One-Time Admin Required):
- Install files to Program Files
- Create scheduled task
- Register system components
- Configure firewall (if needed)

**Runtime Phase** (No Admin Required):
- App launches via scheduled task
- Already has necessary privileges
- No UAC prompts to user
- Seamless user experience

**Exception**: Blocking system processes may require additional elevation.

### Icon Integration

The application icon appears in:
- ✅ Window title bars
- ✅ Taskbar
- ✅ Task Manager
- ✅ File Explorer (exe file)
- ✅ Installer window
- ✅ Desktop shortcut (if created)
- ✅ Start Menu entry

Icon is embedded via:
1. `resource.rc` references `app_icon.ico`
2. CMake includes resource file in build
3. Installer uses same icon
4. Consistent branding throughout

---

## 🎨 Customization

### Change Application Details

Edit `installer/NetView_Setup.iss`:

```pascal
#define MyAppName "NetView"           ; Change app name
#define MyAppVersion "1.0.0"          ; Update version
#define MyAppPublisher "NetView Software"  ; Your company
#define MyAppURL "https://netview.app"    ; Your website
```

### Modify License Text

Edit `docs/LICENSE.txt` with your license terms.

### Update README Content

Edit `docs/README_INSTALLER.txt` - shown before installation
Edit `docs/README.txt` - installed with application

### Add Installer Branding

Create and add:
- `resources/installer_banner.bmp` (497 x 314 px)
- `resources/installer_logo.bmp` (55 x 58 px)

See `resources/ICON_REQUIREMENTS.md` for specifications.

---

## 📊 Build Requirements

### Software Required

1. **CMake 3.20+**
   - Download: https://cmake.org/download/
   - Add to PATH during installation

2. **Visual Studio 2019+**
   - Community Edition (free): https://visualstudio.microsoft.com/
   - Workload: "Desktop development with C++"
   - Or just Build Tools (lighter)

3. **Inno Setup 6** (for installer)
   - Download: https://jrsoftware.org/isinfo.php
   - Install to default location

### System Requirements

- Windows 10/11 (for development)
- Target: Windows 7+ (64-bit)
- Disk space: ~500 MB (build files)
- RAM: 4 GB minimum

---

## 🐛 Troubleshooting

### Build Errors

**"Icon file not found"**
- Ensure `resources/app_icon.ico` exists
- Check path in `src/resource.rc`
- Verify file permissions

**"CMake configuration failed"**
- Verify CMake is in PATH
- Run from Visual Studio Developer Command Prompt
- Delete `build/` folder and retry

**"Linker error - taskschd.lib"**
- Ensure Visual Studio C++ workload installed
- Update to latest Windows SDK
- Check CMakeLists.txt library list

### Installer Errors

**"Inno Setup not found"**
- Install Inno Setup 6
- Verify installation path in `build_installer.bat`
- Update path if installed elsewhere

**"NetView.exe not found"**
- Build application first with `build.bat`
- Check `build/bin/Release/` directory
- Ensure Release configuration built

### Runtime Issues

**"Auto-start not working"**
- Open Task Scheduler
- Verify "NetViewMonitor" task exists
- Check task triggers and actions
- Run task manually to test

**"UAC prompts on every start"**
- Task should run with highest privileges
- Verify task configuration
- May need to recreate scheduled task

**"Widget not visible"**
- Check screen resolution
- Look for hidden windows
- Restart NetView from Start Menu

---

## 📦 Distribution

### For End Users

Distribute: `output/NetView_Setup_v1.0.0.exe`

**Installation Requirements:**
- Windows 7 or later (64-bit)
- Administrator privileges (for installation)
- ~20 MB disk space

**User Experience:**
1. Double-click installer
2. UAC prompt (one time)
3. Read information
4. Accept license
5. Check acceptance checkbox
6. Click Install
7. Wait for installation
8. Launch NetView
9. Widget appears bottom-right

### For Developers

Provide:
- Source code (this entire project)
- Build instructions (this README)
- Icon resources
- Documentation

---

## 🔐 Security Considerations

### Permissions

- ✅ Installer requests admin (one time)
- ✅ App runs with user privileges
- ✅ Elevated task for auto-start
- ✅ No permanent elevation

### Privacy

- ✅ No data sent to external servers
- ✅ All monitoring is local
- ✅ No telemetry or analytics
- ✅ User data stays on machine

### Network Blocking

- ⚠️ Can block any process from network
- ⚠️ User must understand implications
- ⚠️ System processes may require admin

---

## 📝 Version History

### v1.0.0 (Current)
- Initial release
- Network monitoring
- Process control
- Auto-start support
- Professional installer
- First-run experience
- Complete documentation

---

## 🎯 Post-Installation Behavior

### First Launch
1. App detects first run via registry
2. Registers auto-start task
3. Creates configuration directory
4. Shows welcome dialog
5. Starts monitoring

### Subsequent Launches
1. Starts automatically on logon
2. No UAC prompts
3. Widget appears immediately
4. Loads blocked processes
5. Begins monitoring

### User Workflow
1. View network stats on widget
2. Click "Active" card for process list
3. Block/unblock processes as needed
4. Changes persist automatically
5. Survives reboots

---

## 🆘 Support

### For Build Issues
- Check Visual Studio version
- Verify all dependencies installed
- Review error messages carefully
- Delete build folder and retry

### For Installer Issues
- Confirm Inno Setup installed
- Check file paths in .iss script
- Verify exe was built successfully
- Review compiler output

### For Runtime Issues
- Check Windows Event Viewer
- Verify Task Scheduler entries
- Test with admin privileges
- Check firewall settings

---

## 🎓 Learning Resources

### Technologies Used

**Application:**
- Win32 API (UI and windows)
- IP Helper API (network monitoring)
- Task Scheduler COM API (auto-start)
- Windows Resource Files (.rc)

**Installer:**
- Inno Setup scripting (Pascal-like)
- Windows Installer best practices
- UAC elevation handling
- Scheduled task creation

**Build System:**
- CMake (cross-platform builds)
- Visual Studio toolchain
- Resource compilation

### References

- Inno Setup Documentation: https://jrsoftware.org/ishelp/
- Task Scheduler API: https://docs.microsoft.com/en-us/windows/win32/taskschd/
- CMake Tutorial: https://cmake.org/cmake/help/latest/guide/tutorial/
- Win32 API Reference: https://docs.microsoft.com/en-us/windows/win32/api/

---

## ✅ Final Checklist

Before distribution:

- [ ] Add `app_icon.ico` to `resources/`
- [ ] Optionally add installer images
- [ ] Customize company name in installer
- [ ] Update version numbers
- [ ] Review and customize LICENSE.txt
- [ ] Build application (`build.bat`)
- [ ] Test application manually
- [ ] Build installer (`build_installer.bat`)
- [ ] Test installer on clean VM
- [ ] Verify auto-start works
- [ ] Verify uninstaller works
- [ ] Test on target Windows versions
- [ ] Create distribution package

---

## 🎉 You're Ready!

This project provides everything needed for a professional Windows application with installer. Just add your icon and build!

**Next Steps:**
1. Add `resources/app_icon.ico`
2. Run `build.bat`
3. Run `build_installer.bat`
4. Test `output/NetView_Setup_v1.0.0.exe`
5. Distribute to users!

**Questions?** Review the documentation in `docs/` or check `resources/ICON_REQUIREMENTS.md`.

---

**Copyright © 2026 NetView Software**
