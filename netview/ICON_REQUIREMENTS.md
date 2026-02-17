# ICON AND IMAGE REQUIREMENTS

## Required Files

Place the following files in the `resources/` directory:

### 1. Application Icon (REQUIRED)
**File:** `app_icon.ico`
**Location:** `resources/app_icon.ico`
**Specifications:**
- Format: .ICO file
- Recommended sizes: 16x16, 32x32, 48x48, 256x256
- Transparency: Supported
- Purpose: 
  - Application window icon
  - Executable file icon
  - Installer icon
  - Task Manager icon

**How to create:**
- Use an online converter: https://convertio.co/png-ico/
- Or use GIMP, Photoshop, or Icon Workshop
- Include multiple sizes in one .ico file

---

### 2. Installer Banner (OPTIONAL)
**File:** `installer_banner.bmp`
**Location:** `resources/installer_banner.bmp`
**Specifications:**
- Format: .BMP (bitmap)
- Dimensions: 497 x 314 pixels
- Color depth: 24-bit
- Purpose: Large banner on installer welcome page

**Design guidelines:**
- Use NetView branding
- Include logo and tagline
- Clean, professional appearance
- Light background recommended

---

### 3. Installer Logo (OPTIONAL)
**File:** `installer_logo.bmp`
**Location:** `resources/installer_logo.bmp`
**Specifications:**
- Format: .BMP (bitmap)
- Dimensions: 55 x 58 pixels
- Color depth: 24-bit
- Purpose: Small logo on installer sidebar

**Design guidelines:**
- Simple icon or logo mark
- High contrast
- Recognizable at small size

---

## Quick Start (If You Don't Have Images Yet)

The project will build without the optional images, but **app_icon.ico is required**.

### Option 1: Use Placeholder Icon
Download a free icon from:
- https://www.flaticon.com (search "network monitor")
- https://icons8.com (search "network")
- Convert to .ICO format

### Option 2: Create Your Own
1. Design a 256x256 PNG with your logo
2. Use online converter: https://convertico.com/
3. Select multiple sizes: 16, 32, 48, 256
4. Download and save as `app_icon.ico`

---

## File Structure

```
NetViewInstaller/
├── resources/
│   ├── app_icon.ico           ← REQUIRED: Application icon
│   ├── installer_banner.bmp   ← OPTIONAL: Installer welcome banner
│   └── installer_logo.bmp     ← OPTIONAL: Installer sidebar logo
├── src/
│   ├── resource.rc            ← References app_icon.ico
│   └── ...
└── installer/
    └── NetView_Setup.iss      ← References all images
```

---

## Where Icons Appear

### app_icon.ico
- ✓ Window title bar (Monitor Widget)
- ✓ Window title bar (Process Control)
- ✓ Executable file in Windows Explorer
- ✓ Task Manager process list
- ✓ Installer window icon
- ✓ Taskbar (when app is running)
- ✓ Desktop shortcut (if created)
- ✓ Start Menu entry

### installer_banner.bmp
- ✓ Installer welcome page (large image)
- ✓ Installer finish page

### installer_logo.bmp
- ✓ Installer sidebar (all pages)
- ✓ Small logo next to page titles

---

## Testing Icons

After placing app_icon.ico:
1. Run: `build.bat`
2. Check: `build\bin\Release\NetView.exe`
3. Right-click NetView.exe → Properties
4. Verify icon appears correctly

---

## Troubleshooting

**Icon not appearing:**
- Ensure app_icon.ico is in `resources/` folder
- Rebuild project completely (delete `build/` folder first)
- Check resource.rc file references correct path
- Clear Windows icon cache (if needed)

**Installer images not showing:**
- Verify .bmp files are correct dimensions
- Ensure 24-bit color depth (not 32-bit)
- Check file paths in NetView_Setup.iss

**Build error - icon not found:**
- Verify `resources/app_icon.ico` exists
- Check path separators in resource.rc (use `\\`)
- Ensure file is readable (not locked)

---

## Recommendations

### For Professional Look:
1. **App Icon**: Create a distinctive icon that represents network monitoring
   - Use blue/green colors (networking theme)
   - Include signal/wave elements
   - Keep it simple and recognizable

2. **Installer Banner**: Professional branding
   - Include company logo
   - Tagline: "Network Monitor & Process Control"
   - Clean, modern design

3. **Consistency**: Use same color scheme across all images

### For Quick Testing:
1. Use any network-themed icon from free icon sites
2. Convert to .ico with multiple sizes
3. Skip optional installer images initially
4. Add professional images before final release

---

## Need Help?

If you need help creating icons:
1. **Fiverr** - Professional icon design ($5-20)
2. **Free Tools** - GIMP, Paint.NET, Krita
3. **Online Generators** - favicon.io, realfavicongenerator.net
4. **Icon Packs** - flaticon.com, icons8.com (free with attribution)

---

## Current Status

⚠️ **REQUIRED ACTION**: Place `app_icon.ico` in `resources/` folder before building.

Optional: Add `installer_banner.bmp` and `installer_logo.bmp` for professional installer appearance.

The project is fully configured and ready to build once the icon is provided.
