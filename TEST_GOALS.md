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
