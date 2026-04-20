# OpenMatrix — agent notes

ESP32-S3 firmware (PlatformIO, Arduino framework). Prefer matching existing patterns in `src/`, `include/`, and `lib/` before adding new abstractions.

## After firmware-related changes

When edits affect the binary or device behavior, follow the project skill **post-change upload and monitor**:

- Skill path: `.cursor/skills/post-change-upload-monitor/SKILL.md`
- It covers when to flash, when to run a short serial check, port-busy handling, retry limits, and reporting.

## Firmware automation scripts (agents only)

`.cursor/scripts/fw.ps1` lives next to other Cursor project tooling. It exists **for you (the agent)** to run from the **repo root** when verifying firmware changes—**not** as a documented workflow for the human author. Prefer these over ad-hoc `pio` calls so uploads, timed monitors, and port cleanup stay consistent and monitors always exit.

**Upload already compiles**—do not run **`build` immediately before `upload`** (duplicate work). Use **`build`** only when a compile-only check is needed without flashing.

When a change is trivial and **no flash is required**, do **not** run build or upload (see the skill)—that only wastes time. For substantive firmware edits, **upload is usually needed**.

| Action | Command |
|--------|---------|
| Clean build artifacts | `pwsh -File .cursor/scripts/fw.ps1 clean` |
| Compile only (no flash) | `pwsh -File .cursor/scripts/fw.ps1 build` |
| Upload (builds as part of upload) | `pwsh -File .cursor/scripts/fw.ps1 upload` |
| Serial monitor (default **15 s** then stop) | `pwsh -File .cursor/scripts/fw.ps1 monitor` |
| Custom duration / port | `pwsh -File .cursor/scripts/fw.ps1 monitor -Seconds 20 -Port COM5` |
| Stop serial monitor processes (port busy) | `pwsh -File .cursor/scripts/fw.ps1 free-serial` |

If `pwsh` is not installed, use `powershell -File .cursor/scripts/fw.ps1 ...` instead.

Typical verify when flashing: **`upload`** → **`monitor`** when the skill says so. If **upload** fails because the port is in use, run **`free-serial`**, then retry **`upload`** once.
