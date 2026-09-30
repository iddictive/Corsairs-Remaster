# Changelog

## Experimental Metal build — 2026-09-30

### Distribution

- Published the standalone Apple Silicon / macOS app, public source and pinned build inputs.
- Added the Google Drive app download to the release and project case.
- Player saves and settings remain outside the replaceable application.

### Startup and menu

- Replaced the shell app entry point with a native relocatable launcher.
- Signed nested runtime dependencies and the complete application bundle.
- Corrected the duplicated interface texture path that hid the custom main-menu logo.
- Adjusted social-button text placement, removed the duplicate lower banner, and made the existing version label open the project case.

### Included source

- Native Metal rendering and the reviewed engine/gameplay patch stack.
- Existing rendering and gameplay work is described in the project case and source; this entry does not claim every patch was first implemented on this date.
- The content baseline remains GPK 1.3.2 AT + ReConstruction 1.4.1.

### Validation

Native archive extraction, relocated launcher staging, dependency closure and strict app signature checks passed. The player reached the menu, sea, deck and prison scenes after the startup correction. Broader gameplay and cross-machine validation remain open.
