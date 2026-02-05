# MeshNet Test Doelen

## Doel
- Alle backend API-endpoints end-to-end testen (HTTP + validatie)
- Kritieke flows valideren: users, auth, nodes, pages, sync, broadcast, acks/pings/messages
- Dekking > 80% op backend route-logica en node-router/modules/nodes

## Scope
### Backend (RPI)
- server.js routes
- node-router.js routes (/api/host)
- modules/nodes routes (/api/nodes/*)

### Device (Heltec)
- LoRa sync (REQ/RESP), ACK/PONG
- Web UI sync status + team page rendering

## Meetpunten
- HTTP statuscodes en response shape per endpoint
- Validatie foutpaden (missing fields, 404, 401)
- Broadcast flow: DB insert + relay attempt
- Sync payloads: users/pages
- Coverage target: >= 80% (lijnen/functies)

## Resultaat
- Testrapport met status per endpoint
- Coverage report >= 80%

## Test Playbook
Zie [TEST_PLAYBOOK.md](TEST_PLAYBOOK.md) voor zelf-uitvoerbare stappen, verwachte uitkomsten en herstelacties.

## TODO / Taken
- Implementeer nieuwe Heltec AP flows (home/login/admin).
- Maak nieuwe firmware release: **1.8.0**.
- Deploy op alle Heltec devices.
- Update testscripts:
	- Maak **10 teams** via API endpoints.
	- Maak per team **3–8 users** via API endpoints.
	- Maak **10 virtuele Heltec nodes** (alleen in database).
	- Maak pagina’s voor alle groepen/nodes.
	- Test dat echte Heltec devices **alleen eigen pagina’s** en **alle users** ontvangen.
- Voeg in Docker web UI bij node-overzicht **geladen users/pages** toe.
	- Pas webserver UI aan.
	- Pas database + API endpoint aan.
	- Pas gateway aan.
	- Pas Heltec (LoRa) functionaliteit aan.
	- Stats ophalen via **on-demand request** (knop per node) om data verzoeken te beperken.
