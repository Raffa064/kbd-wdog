
# ThinkPad L14 Gen 1 Keyboard Watchdog

## Background

On my **ThinkPad L14 Gen 1**, after installing Linux, I started experiencing a strange hardware issue where the internal keyboard would randomly stop responding.

When the problem occurs:

* The last pressed key gets stuck and keeps repeating.
* The keyboard becomes completely unresponsive.
* Keyboard status LEDs (Caps Lock, Num Lock, etc.) stop reacting.
* The device behaves as if the keyboard had been physically disconnected.

The only reliable way to recover is to completely power off the laptop and turn it on again.

Unfortunately, neither `journalctl`, `dmesg`, nor other system logs provide any useful information about the failure.

The only report of a similar issue I could find is this Arch Linux forum thread:

* [https://bbs.archlinux.org/viewtopic.php?id=274456](https://bbs.archlinux.org/viewtopic.php?id=274456)

## About This Project

This project is **not a fix** for the underlying hardware/driver problem.

Instead, it provides a simple watchdog workaround that monitors the TrackPoint physical buttons. When a specific button combination is detected, the script reloads the `atkbd` driver, which restores keyboard functionality without requiring a full reboot.

This allows recovery from the keyboard lockup while keeping the system running.

## Disclaimer

This is only a workaround for an unresolved issue affecting some ThinkPad L14 Gen 1 systems running Linux. The root cause remains unknown.
