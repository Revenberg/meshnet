# MeshNet Deploy & Test Doelen (verwerkbaar plan)

## Werkwijze (verplicht)
- Werk sequentieel door de takenlijst.
- Toon voortgang per (sub)taak en registreer resultaat.
- Valideer elke taak op gewenst resultaat.
  - **Succes:** ga door met de volgende taak.
  - **Niet-succes:** herstel, test opnieuw, en start daarna **vanaf Deploy** opnieuw.
- Documenteer acties zodat deze eenvoudig opnieuw uitgevoerd kunnen worden

## Takenlijst (kort en uitvoerbaar)
1. **Push local software naar GIT**
  - Maak commit-bericht (samenvatting van wijzigingen).
  - Commit lokaal en push naar remote.
2. **PULL laatste software versie op RPI**
  - SSH naar RPI en pull laatste wijzigingen.
3. **Versie bump** (minor) in alle relevante firmware/config-bestanden.
4. **Deploy backend (RPI)**: actuele Docker containers via SSH.
5. **Deploy Heltec devices**:
   - Lokaal via laptop (autodetect poorten).
   - RPI via SSH.
6. **Validatie**: firmwareversies, AP-SSID, serial logs, sync status.
7. **Testen**: end-to-end API + kritieke flows.
8. **Extra features**: UI + sync stats + wifi scan.

## Deploy
- Verhoog de versie = nieuwe **laatste versie** (minor change upgrade) overal.
  - Heltec versie
  - WIFI AP info versie
- Deploy actuele versies van Docker containers op RPI via SSH.
- **Deploy‑stap (verplicht):** maak test‑nodes aan in DB als de test suite 404 geeft op MeshNode‑1/2/3 (zie sectie “Test‑nodes aanmaken”).
- Deploy op alle Heltec devices.
  - Op de RPI: via SSH.
  - Op de laptop: autodetect poorten.
- **Actie (verplicht):** start serial monitor om versies te valideren en log te bewaren. Monitor tot de versies voor de aangesloten serial poorten in de log is verschenen.
  - Indien versie niet correct: **altijd** locate + reflash voor die node uitvoeren en daarna opnieuw valideren tot alle nodes **laatste versie** tonen.
- **Actie (verplicht):** deployment verslag met nodes (macadres en versie)

### Voortgang monitoren (PowerShell)
- **Terminal output** toont live voortgang en tijdsduur per stap.
- **Logbestanden** staan in `MeshNet/logs/`.
- Scripts met zichtbare voortgang:
  - `FLASH_ALL_CONNECTED.ps1` (autodetect + progress + log)
  - `scripts/flash_with_timing.ps1` (build + upload + timing + log)

### Voortgangsjabloon (per taak)
- Status: **IN PROGRESS / OK / FAIL**
- Actie: *wat is uitgevoerd*
- Resultaat: *wat is aangetoond (log/versie/response)*
- Herstel: *wat is aangepast bij fail*

### RPI
Op de RPI zijn **geen sudo** commands nodig voor Docker of het schrijven van de Heltec devices.

### RPI deployment (stappen)
1. SSH naar RPI en update repo:
  - `cd ~/meshnet`
  - `git pull`
2. Zorg dat `docker-compose.yml` bestaat in `~/meshnet`.
3. Start containers:
  - `docker compose up -d`
  - **Wifi-scanner (expliciet):**
    - `docker compose -f ~/meshnet/rpi/docker/docker-compose.yml --project-directory ~/meshnet/rpi/docker up -d wifi-scanner`
4. Bij container‑naam conflict:
  - `docker rm -f meshnet-mysql meshnet-mqtt meshnet-backend meshnet-lora-gateway meshnet-caddy ghostnet_web`
  - `docker compose up -d`
5. Controleer status:
  - `docker ps`

### NetworkManager check (wifi‑scanner connect‑checks)
1. Controleer of NetworkManager draait:
  - `systemctl is-active NetworkManager`
  - `systemctl is-enabled NetworkManager`
2. Als niet actief of niet enabled, voer uit (met sudo):
  - `sudo systemctl start NetworkManager`
  - `sudo systemctl enable NetworkManager`

### Test-nodes aanmaken (voor test suite)
Voer éénmalig uit wanneer de API tests 404 geven op MeshNode-1/2/3.
- `docker exec -i meshnet-mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet <<'SQL'`
- `INSERT IGNORE INTO nodes (nodeId, macAddress, functionalName, version, isActive, createdAt, updatedAt)`
- `VALUES`
- `  ('72d67530-dac6-4666-885c-160cb36579ee','00:00:00:00:aa:01','MeshNode-1','test',true,NOW(),NOW()),`
- `  ('26b80c3a-a7e2-4634-957a-51f7b777de72','00:00:00:00:aa:02','MeshNode-2','test',true,NOW(),NOW()),`
- `  ('d1ec1f02-0e0b-4763-94d5-984e93c11bde','00:00:00:00:aa:03','MeshNode-3','test',true,NOW(),NOW());`
- `SQL`

## Test taken
- Alle backend API-endpoints end-to-end testen (HTTP + validatie).
- Kritieke flows valideren: users, auth, nodes, pages, sync, broadcast, acks/pings/messages.
- Dekking: **>= 80%** op backend route-logica en node-router/modules/nodes.

### Backend (RPI)
- server.js routes
- node-router.js routes (/api/host)
- modules/nodes routes (/api/nodes/*)

### Device (Heltec)
- LoRa sync (REQ/RESP), ACK/PONG
- Web UI sync status + team page rendering

### Meetpunten
- HTTP statuscodes en response shape per endpoint
- Validatie foutpaden (missing fields, 404, 401)
- Broadcast flow: DB insert + relay attempt
- Sync payloads: users/pages
- Coverage target: >= 80% (lijnen/functies)

### testscripts:
- Maak **10 teams** via API endpoints.
- Maak per team **3–8 users** via API endpoints.
- Maak **10 virtuele Heltec nodes** (alleen in database).
- Maak pagina’s voor alle groepen/nodes.
- Valideer dat echte Heltec devices **alleen eigen pagina’s** en **alle users** ontvangen.

### Resultaat
- Testrapport met status per endpoint
- Coverage report >= 80%

### Test Playbook
Zie [TEST_PLAYBOOK.md](TEST_PLAYBOOK.md) voor zelf-uitvoerbare stappen, verwachte uitkomsten en herstelacties.

