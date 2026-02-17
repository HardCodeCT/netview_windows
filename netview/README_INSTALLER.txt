═══════════════════════════════════════════════════════════════
  NETVIEW NETWORK MONITOR - INSTALLATION INFORMATION
═══════════════════════════════════════════════════════════════

Thank you for choosing NetView!

WHAT IS NETVIEW?
────────────────
NetView is a lightweight network monitoring application that helps you:
  • Monitor real-time network traffic (incoming/outgoing data)
  • Track which processes are using your network connection
  • Block specific processes from accessing the network
  • Maintain persistent blocking rules across system reboots

SYSTEM REQUIREMENTS
───────────────────
  • Windows 7 or later (64-bit)
  • Administrator privileges (required only during installation)
  • Active network adapter (WiFi or Ethernet)
  • ~20 MB of disk space

WHAT THIS INSTALLER DOES
─────────────────────────
1. Installs NetView to: C:\Program Files\NetView\
2. Requests administrator privileges (ONE TIME ONLY)
3. Configures NetView to start automatically on system boot
4. Creates a scheduled task for automatic startup
5. Launches NetView for the first time

AFTER INSTALLATION
───────────────────
  • NetView will start automatically when you log in
  • A small widget appears at the bottom-right of your screen
  • The widget shows real-time network statistics
  • Click the "Active: X" card to control process network access
  • No UAC prompts during normal operation

USING NETVIEW
─────────────
1. The monitor widget displays:
   - Data In: Total incoming data
   - Data Out: Total outgoing data
   - Transfer rates in real-time
   
2. Click the white "Active" card to open Process Control window

3. In the Process Control window:
   - View all network-active processes
   - Click "Block" (turns red) to block a process
   - Click "Active" (turns blue) to allow a process
   - Blocked processes persist after reboot

PRIVACY & SECURITY
──────────────────
  ✓ NetView ONLY monitors LOCAL network activity
  ✓ NO data is sent to external servers
  ✓ NO personal information is collected
  ✓ All configuration stored locally on your computer
  ✓ Open architecture - you can verify the code

ADMINISTRATOR PRIVILEGES EXPLAINED
───────────────────────────────────
NetView requires administrator privileges ONLY during installation to:
  • Install files to Program Files directory
  • Create a scheduled task for auto-start
  • Configure network monitoring components

After installation, NetView runs with NORMAL user privileges and will
NOT trigger UAC prompts every time it starts.

AUTO-START CONFIGURATION
────────────────────────
NetView uses Windows Task Scheduler to start automatically. This is
the industry-standard method used by security and system tools.

To disable auto-start:
  1. Open Task Scheduler
  2. Find "NetViewMonitor" task
  3. Disable or delete the task

UNINSTALLING NETVIEW
────────────────────
To completely remove NetView:
  1. Open "Add or Remove Programs" (Programs and Features)
  2. Find "NetView" in the list
  3. Click "Uninstall"

The uninstaller will remove:
  ✓ All application files
  ✓ Auto-start configuration
  ✓ Scheduled tasks
  ✓ Network blocking rules
  ✓ Application settings

TROUBLESHOOTING
───────────────
Problem: Widget not visible
Solution: Check if hidden behind other windows, or restart NetView

Problem: Blocking not working
Solution: Run as administrator for system processes

Problem: High CPU usage
Solution: Reduce number of monitored processes

SUPPORT
───────
For help, documentation, and updates:
  Website: https://netview.app
  Support: https://netview.app/support
  Email: support@netview.app

═══════════════════════════════════════════════════════════════
  PLEASE READ THE LICENSE AGREEMENT ON THE NEXT SCREEN
═══════════════════════════════════════════════════════════════

By continuing with this installation, you acknowledge that you have
read this information and agree to the terms of the license agreement.

Click "Next" to continue the installation.
