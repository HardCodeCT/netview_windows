# 🚀 NetView - Quick Start Guide

## ⚡ 3-Minute Setup

### Step 1: Get an Icon (1 minute)

You need an application icon before building. Choose one option:

**Option A - Download Free Icon:**
1. Go to: https://www.flaticon.com
2. Search: "network monitor" or "wifi signal"
3. Download any icon you like
4. Go to: https://convertico.com/
5. Upload your downloaded PNG
6. Select sizes: 16, 32, 48, 256
7. Download as `.ico` file
8. Save as: `NetViewInstaller/resources/app_icon.ico`

**Option B - Use This Simple Network Icon URL:**
```
https://cdn-icons-png.flaticon.com/512/2099/2099058.png
```
1. Download that PNG
2. Convert at https://convertico.com/
3. Save to `NetViewInstaller/resources/app_icon.ico`

**⚠️ CRITICAL**: The file MUST be named exactly `app_icon.ico` and placed in `resources/` folder.

---

### Step 2: Build Application (1 minute)

Open Command Prompt in the `NetViewInstaller` folder and run:

```batch
build.bat
```

**What this does:**
- Configures CMake project
- Compiles C++ code with Visual Studio
- Embeds your icon
- Creates `build/bin/Release/NetView.exe`

**Expected output:**
```
[1/4] Cleaning previous build...
[2/4] Creating build directory...
[3/4] Configuring project with CMake...
[4/4] Building Release configuration...

Build completed successfully!
Executable: build\bin\Release\NetView.exe
```

**If you get errors:**
- Make sure you have Visual Studio installed
- Run from "Developer Command Prompt for VS"
- Or install "Visual Studio Build Tools"

---

### Step 3: Create Installer (1 minute)

First, install **Inno Setup** (if you haven't):
1. Download: https://jrsoftware.org/isdl.php
2. Install with default settings

Then run:

```batch
build_installer.bat
```

**What this does:**
- Compiles Inno Setup script
- Packages NetView.exe with resources
- Creates `output/NetView_Setup_v1.0.0.exe`

**Expected output:**
```
[1/2] Compiling installer script...
[2/2] Installer created successfully!

Location: output\NetView_Setup_v1.0.0.exe
```

---

## ✅ You're Done!

Your installer is ready: `output/NetView_Setup_v1.0.0.exe`

### Test It:

1. Double-click `NetView_Setup_v1.0.0.exe`
2. Accept UAC prompt (admin required)
3. Read the information
4. Accept the license
5. **Check the mandatory checkbox** ← Required!
6. Click "Install"
7. Launch NetView
8. Widget appears bottom-right!

---

## 📦 What You Get

### The Installer Provides:

✅ **Professional Installation Experience**
- Welcome page with branding
- Information screen (explains what NetView does)
- License agreement (legal protection)
- **Mandatory acceptance checkbox** (prevents accidental install)
- Progress bar during installation

✅ **Smart Permissions**
- Admin required ONLY during installation
- After install, runs without UAC prompts
- Uses Windows Task Scheduler for auto-start

✅ **Auto-Start on Boot**
- Configures scheduled task
- Starts automatically when Windows boots
- No manual launching needed

✅ **First-Run Experience**
- Welcome dialog on first launch
- Explains how to use NetView
- Confirms installation success

✅ **Complete Uninstaller**
- Removes all files
- Deletes scheduled task
- Cleans registry entries
- Removes configuration

---

## 🎯 What the App Does

### Monitor Widget (Bottom-Right)
- Shows real-time network stats
- Data In / Data Out totals
- Transfer rates (KB/s, MB/s)
- Active process count

### Process Control Window
- Opens when you click "Active: X" card
- Lists all network-active processes
- Block/Active buttons for each process
- Changes persist after reboot

### Button States
- **Blue** = Process allowed (Active)
- **Red** = Process blocked
- **Grey** = Inactive state

---

## 🎨 Optional: Customize Installer

### Change App Name/Branding

Edit `installer/NetView_Setup.iss`:

```pascal
#define MyAppName "YourAppName"
#define MyAppPublisher "Your Company"
#define MyAppURL "https://yourwebsite.com"
```

### Add Installer Banner Images

Create these files:
- `resources/installer_banner.bmp` (497 x 314 px)
- `resources/installer_logo.bmp` (55 x 58 px)

Then rebuild installer.

### Update License

Edit `docs/LICENSE.txt` with your terms.

---

## 🐛 Common Issues

### "Icon file not found"
**Solution:** Place `app_icon.ico` in `resources/` folder

### "CMake not found"
**Solution:** Install CMake from https://cmake.org

### "Visual Studio not found"
**Solution:** 
- Install Visual Studio 2019+ Community (free)
- OR install "Build Tools for Visual Studio"
- OR run from "Developer Command Prompt"

### "Inno Setup not found"
**Solution:** Install from https://jrsoftware.org/isinfo.php

### Installer shows "NetView.exe not found"
**Solution:** Run `build.bat` first to build the application

---

## 📋 File Checklist

Before building, ensure you have:

```
NetViewInstaller/
├── resources/
│   └── app_icon.ico          ← ⚠️ REQUIRED
├── src/                      ← ✅ Provided
├── installer/                ← ✅ Provided
├── docs/                     ← ✅ Provided
├── CMakeLists.txt            ← ✅ Provided
├── build.bat                 ← ✅ Provided
└── build_installer.bat       ← ✅ Provided
```

**Only missing**: `app_icon.ico` (you must add this!)

---

## 🎓 Next Steps

### For Development
- Read `PROJECT_README.md` for full documentation
- Customize app features in `src/` files
- Modify installer in `installer/NetView_Setup.iss`

### For Distribution
- Test installer on clean Windows VM
- Create user documentation
- Set up support channels
- Distribute `output/NetView_Setup_v1.0.0.exe`

### For Customization
- Change colors in source files
- Add new features
- Modify UI layout
- Update documentation

---

## ✨ Pro Tips

1. **Test on Clean VM**: Always test installer on fresh Windows install
2. **Version Numbers**: Update version in CMakeLists.txt AND NetView_Setup.iss
3. **Branding**: Use consistent colors across icon, banner, and UI
4. **Documentation**: Keep README.txt updated with new features
5. **Uninstaller**: Test uninstall process thoroughly

---

## 🆘 Still Need Help?

### Resources Provided
- `PROJECT_README.md` - Complete documentation
- `resources/ICON_REQUIREMENTS.md` - Icon specifications
- `docs/LICENSE.txt` - Legal template
- `docs/README.txt` - User guide

### External Resources
- CMake Docs: https://cmake.org/documentation/
- Inno Setup Help: https://jrsoftware.org/ishelp/
- Visual Studio: https://docs.microsoft.com/en-us/visualstudio/

---

## 🎉 Summary

**To build NetView:**
1. Add icon to `resources/app_icon.ico`
2. Run `build.bat`
3. Run `build_installer.bat`
4. Done! Your installer is in `output/`

**Total time: 3-5 minutes** ⚡

The project is 100% complete and production-ready. Just add your icon and build!

---

**Happy Building! 🚀**
