# MeshNet Test Playbook (Self-Run)

## Doel
Deze playbook beschrijft exacte stappen, verwachte uitkomsten en herstelacties zodat tests volledig zelfstandig kunnen worden uitgevoerd, gevalideerd en hersteld.

## Benodigdheden
- RPI4 (ghostnet) met containers: backend, lora-gateway, mysql
- Laptop met Heltec devices op COM5 en COM7
- Firmware build output: MeshNet/build

## Kerncommando’s
### Backend (RPI)
- Push users: `POST http://127.0.0.1:3001/api/sync/push/users`
- Push pages: `POST http://127.0.0.1:3001/api/sync/push/pages`
- Sync pages (pull): `GET http://127.0.0.1:3001/api/sync/pages`

### Serial (Heltec)
Gebruik serial input commando’s:
- `REQ;USERS` → users sync request
- `REQ;PAGES` → pages sync request
- `STATUS` → shows UsersSynced, PagesSynced, StoredPages, UserCount

## Test 1: Page update → sync → serial verify
1. Update een pagina via API of DB (bijv. update pages.content voor team 975).
2. Trigger pages push: `POST /api/sync/push/pages`.
3. Open serial monitor op device (COM5/COM7).
4. Verwacht:
   - `[PAGE-SYNC] Receiving multipart pages payload` of `single payload`
   - `[PAGE-SYNC] Stored page for team: 975 ...`
   - `[PAGE-SYNC] Stored pages total: >= 1`
5. Valideer UI: team page zichtbaar + juiste update tijd.

## Test 2: User update → sync → serial verify
1. Update user via API of DB (username/team/sha256).
2. Trigger users push: `POST /api/sync/push/users`.
3. Verwacht:
   - `[USER-SYNC] PART x/y received`
   - User count on node > 0
4. Controleer login via Heltec web UI.

## Test 3: Serial refresh requests
1. Open serial monitor.
2. Stuur `REQ;USERS` → verwacht `[SYNC] Requesting users: ...`.
3. Stuur `REQ;PAGES` → verwacht `[SYNC] Requesting pages: ...`.
4. Stuur `STATUS` → verwacht actuele flags + counts.

## Test 4: Ping + ACK observatie
1. Check docker logs: `docker logs --tail 200 meshnet-lora-gateway`.
2. Verwacht PING/PONG verkeer en ACKs in logs.
3. Op serial verwacht `[PING]` en `PONG` berichten.

## Validatiecriteria
- UsersSynced en PagesSynced = true.
- StoredPages >= 1 wanneer pages aanwezig zijn.
- UI toont alle team links (975, Administrator, Default Group).

## Herstelacties
- Geen pages? Controleer dat pages payload multipart wordt verstuurd en ontvangen.
- Alleen Administrator? Controleer dat pages payload niet te groot is en entries correct gescheiden zijn.
- Geen users? Controleer users multipart delivery en retry delays.

## Iteratieve fix-loop
1. Observeer logs.
2. Pas code aan indien nodig.
3. Bump versie.
4. Build + flash alle devices.
5. Herhaal tests tot alle criteria ok zijn.
