# MeshNet Run Report (2026-02-04)

## Scope
- Wi-Fi AP version validation via RPi (UTP) and containerized scan
- Pages/users sync validation on Heltec devices
- Firmware rollout v1.6.0 to all devices

## Changes Applied
- Firmware bumped to v1.6.0 across nodes.
- Added serial diagnostics and LISTPAGES command.
- Implemented RESP;PAGE per-page chunking for reliable pages sync.
- Backend and gateway updated to send RESP;PAGE packets.

## Deployments
- Laptop Heltec devices: COM5, COM7 flashed to v1.6.0.
- RPi node (/dev/ttyUSB0) flashed to v1.6.0.
- RPi containers rebuilt: backend and lora-gateway.

## Tests Executed
### Wi-Fi AP validation (RPi via UTP)
- Containerized scan (`debian:bookworm-slim` with iw/wireless-tools).
- Result: only MeshNet V1.6.0 SSIDs detected.

### Users/Pages sync
- API push users/pages:
  - POST /api/sync/push/users
  - POST /api/sync/push/pages
- Serial checks:
  - STATUS
  - LISTPAGES
  - REQ;PAGES

## Results
- Users: OK (user `sander` present).
- Pages: OK (team 975 loaded; Heltec no longer reports “Geen pagina gevonden”).
- Wi-Fi AP: OK (no older MeshNet SSIDs detected).

## Evidence (Serial)
- COM5/COM7 STATUS indicates UsersSynced and PagesSynced true.
- LISTPAGES shows team `975` and `Administrator` on node with 975 page.
- Gateway logs show RESP;PAGE chunked responses.

## Open Items
- Overbodige files inventariseren en prefix `del_` toepassen.

## Next Steps
- Complete del_ cleanup.
- Keep periodic AP scan to confirm only v1.6.0 remains.
