# NetView Network Monitor

A lightweight, modern network traffic monitor for Windows that displays real-time upload/download statistics in a sleek overlay widget.

## Features

- **Real-time Monitoring**: Live network traffic statistics updated every second
- **Clean UI**: Modern dark-themed overlay widget
- **Lightweight**: Minimal resource usage using Windows IP Helper API
- **No Admin Required**: Works with standard user permissions
- **System-wide Monitoring**: Tracks all network interfaces (except loopback)

## Technical Details

### Architecture

This version uses the **IP Helper API** (`iphlpapi.dll`) instead of ETW (Event Tracing for Windows):

**Advantages:**
- ✅ No administrator privileges required
- ✅ No driver signing needed
- ✅ Much simpler codebase
- ✅ OS-maintained accuracy (kernel does the counting)
- ✅ Very low CPU usage

**Trade-offs:**
- ⚠️ System-wide statistics only (not per-process)
- ⚠️ Polling-based (1-second intervals)
- ⚠️ Shows total traffic across all network interfaces

### Components

- **DataManager**: Uses `GetIfTable2()` to poll network interface statistics
- **MonitorWidget**: Renders the overlay UI with GDI
- **UIHelper**: Provides modern UI drawing utilities
- **main.cpp**: Application entry point

## Building

### Requirements

- Windows 7 or later
- CMake 3.20+
- Visual Studio 2019 or later (with C++ desktop development)

### Build Steps

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Run
./bin/NetView.exe
```

## How It Works

The application polls `GetIfTable2()` from the IP Helper API every second to retrieve cumulative byte counters (`InOctets` and `OutOctets`) for all active network interfaces. It then calculates the delta between readings to determine the transfer rate.

This approach is:
- **Accurate**: Matches what Windows Task Manager and Resource Monitor show
- **Reliable**: Uses the same APIs Windows itself uses internally
- **Efficient**: No event processing overhead, just simple counter reads

## Comparison to ETW Version

| Feature | IP Helper API | ETW |
|---------|--------------|-----|
| Admin Required | ❌ No | ✅ Yes |
| Per-Process Stats | ❌ No | ✅ Yes |
| Accuracy | ✅ Excellent | ⚠️ Can drop events |
| Complexity | ✅ Simple | ❌ Complex |
| CPU Usage | ✅ Minimal | ⚠️ Higher |
| Code Size | ~400 lines | ~800 lines |

## License

This is a demonstration project. Feel free to use and modify as needed.

## Notes

- The widget appears in the bottom-right corner of your screen
- Hover over it to see the highlight effect
- The green indicator shows "MONITORING" status
- Cumulative totals are shown since application start
- Rate shows bytes/second averaged over the last second
