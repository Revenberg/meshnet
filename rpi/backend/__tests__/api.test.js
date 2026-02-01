const request = require('supertest');
const bcryptjs = require('bcryptjs');
const axios = require('axios');

jest.mock('axios', () => ({
  post: jest.fn(async () => ({ data: { message: 'ok' } }))
}));

jest.mock('mysql2/promise', () => ({
  createPool: jest.fn(() => mockPool)
}));

const mockPool = {
  query: jest.fn(async (sql, params) => {
    if (sql.includes('SELECT * FROM nodes WHERE isActive = true')) {
      return [[{ nodeId: 'NODE1', isActive: true }]];
    }
    if (sql.includes('SELECT * FROM nodes WHERE nodeId = ?')) {
      return [[{ nodeId: params[0], isActive: true }]];
    }
    if (sql.includes('SELECT u.userId')) {
      return [[{ userId: 'U1', username: 'sander', email: 's@x', groupId: 1, isActive: 1, passwordSha256: 'hash', team: 'Red Team' }]];
    }
    if (sql.includes('SELECT * FROM users WHERE userId = ?')) {
      return [[{ userId: params[0], username: 'sander', email: 's@x', groupId: 1, isActive: 1, passwordHash: await bcryptjs.hash('test', 10), passwordSha256: 'hash' }]];
    }
    if (sql.includes('SELECT * FROM users WHERE username = ? AND isActive = true')) {
      return [[{ userId: 'U1', username: params[0], email: 's@x', groupId: 1, isActive: 1, passwordHash: await bcryptjs.hash('test', 10), passwordSha256: 'hash' }]];
    }
    if (sql.includes('SELECT u.username, u.passwordSha256')) {
      return [[{ username: 'sander', passwordSha256: 'hash', name: 'Red Team' }]];
    }
    if (sql.includes('SELECT g.name AS team, p.content')) {
      return [[{ team: 'Red Team', html: '<p>Red</p>' }]];
    }
    if (sql.includes('SELECT * FROM `groups` WHERE id = ?')) {
      return [[{ id: params[0], name: 'Red Team' }]];
    }
    if (sql.includes('SELECT * FROM `groups`')) {
      return [[{ id: 1, name: 'Red Team' }]];
    }
    if (sql.includes('SELECT * FROM pages WHERE isActive = true')) {
      return [[{ pageId: 'P1', isActive: true }]];
    }
    if (sql.includes('FROM pages') && sql.includes('WHERE nodeId = ?')) {
      return [[{ pageId: 'P1', nodeId: params[0], isActive: true, title: 'T', content: '<p>x</p>', imageUrl: null, refreshInterval: 30, updatedAt: new Date() }]];
    }
    if (sql.includes('SELECT * FROM pages')) {
      return [[{ pageId: 'P1' }]];
    }
    if (sql.includes('SELECT nodeIdFrom')) {
      return [[{ nodeIdFrom: 'A', nodeIdTo: 'B', signalStrength: -70, hops: 1, lastSeen: new Date() }]];
    }
    if (sql.includes('SELECT nodeId FROM nodes LIMIT 1')) {
      return [[{ nodeId: 'NODE1' }]];
    }
    if (sql.includes('SELECT DISTINCT teamName')) {
      return [[{ teamName: 'Red Team', username: 'sander', firstSeen: new Date(), lastSeen: new Date() }]];
    }
    if (sql.includes('SELECT COUNT(DISTINCT connectionId)')) {
      return [[{ total: 1 }]];
    }
    if (sql.includes('SELECT COUNT(DISTINCT teamName)')) {
      return [[{ total: 1 }]];
    }
    if (sql.includes('SELECT nodeId, COUNT(*)')) {
      return [[{ nodeId: 'NODE1', connectionCount: 1, uniqueTeams: 1 }]];
    }
    return [[]];
  }),
  execute: jest.fn(async (sql, params) => {
    if (sql.includes('SELECT nodeId, macAddress FROM nodes')) {
      return [[{ nodeId: 'NODE1', macAddress: 'AA:BB:CC' }]];
    }
    if (sql.includes('SELECT broadcastId')) {
      return [[{ broadcastId: 'B1', username: 'u', title: 't', content: 'c', ttl: 60, createdAt: new Date(), expiresAt: new Date(), status: 'active' }]];
    }
    return [[], []];
  }),
  getConnection: jest.fn(async () => ({
    execute: mockPool.execute,
    release: jest.fn()
  }))
};

const app = require('../src/server');
const nodeConnections = require('../src/modules/nodes/node-connections');
const nodeRouter = require('../src/node-router');
app.setDbPoolForTests(mockPool);

describe('MeshNet Backend API', () => {
  test('GET /health', async () => {
    const res = await request(app).get('/health');
    expect(res.status).toBe(200);
    expect(res.body.status).toBe('ok');
  });

  test('GET /api/nodes', async () => {
    const res = await request(app).get('/api/nodes');
    expect(res.status).toBe(200);
    expect(res.body.length).toBeGreaterThan(0);
  });

  test('GET /api/nodes/:nodeId', async () => {
    const res = await request(app).get('/api/nodes/NODE1');
    expect(res.status).toBe(200);
    expect(res.body.nodeId).toBe('NODE1');
  });

  test('POST /api/nodes', async () => {
    const res = await request(app).post('/api/nodes').send({ functionalName: 'N1', version: '0.9.4' });
    expect(res.status).toBe(200);
    expect(res.body.success).toBe(true);
  });

  test('POST /api/nodes/register', async () => {
    const res = await request(app).post('/api/nodes/register').send({ nodeId: 'NODE1' });
    expect(res.status).toBe(200);
    expect(res.body.status).toBe('registered');
  });

  test('POST /api/nodes/register validation', async () => {
    const res = await request(app).post('/api/nodes/register').send({});
    expect(res.status).toBe(400);
  });

  test('POST /api/nodes/:nodeId/update', async () => {
    const res = await request(app).post('/api/nodes/NODE1/update').send({ functionalName: 'N1' });
    expect(res.status).toBe(200);
    expect(res.body.status).toBe('updated');
  });

  test('PUT /api/nodes/:nodeId', async () => {
    const res = await request(app).put('/api/nodes/NODE1').send({ functionalName: 'N1' });
    expect(res.status).toBe(200);
  });

  test('DELETE /api/nodes/:nodeId', async () => {
    const res = await request(app).delete('/api/nodes/NODE1');
    expect(res.status).toBe(200);
  });

  test('Users CRUD', async () => {
    const create = await request(app).post('/api/users').send({ username: 'sander', password: 'test', groupId: 1 });
    expect(create.status).toBe(200);
    expect(create.body.success).toBe(true);

    const list = await request(app).get('/api/users');
    expect(list.status).toBe(200);

    const getOne = await request(app).get('/api/users/U1');
    expect(getOne.status).toBe(200);

    const update = await request(app).put('/api/users/U1').send({ username: 'sander2' });
    expect(update.status).toBe(200);

    const del = await request(app).delete('/api/users/U1');
    expect(del.status).toBe(200);
  });

  test('Users validation errors', async () => {
    const badCreate = await request(app).post('/api/users').send({ username: 'x' });
    expect(badCreate.status).toBe(400);

    mockPool.query.mockImplementationOnce(async () => [[]]);
    const missing = await request(app).get('/api/users/NOPE');
    expect(missing.status).toBe(404);
  });

  test('Auth login ok + bad', async () => {
    const ok = await request(app).post('/api/auth/login').send({ username: 'sander', password: 'test' });
    expect(ok.status).toBe(200);
    expect(ok.body.success).toBe(true);

    mockPool.query.mockImplementationOnce(async () => [[{ userId: 'U1', username: 'sander', email: 'a', groupId: 1, passwordHash: await bcryptjs.hash('other', 10), isActive: 1 }]]);
    const bad = await request(app).post('/api/auth/login').send({ username: 'sander', password: 'test' });
    expect(bad.status).toBe(401);
  });

  test('Auth validation errors', async () => {
    const bad = await request(app).post('/api/auth/login').send({ username: 'sander' });
    expect(bad.status).toBe(400);
  });

  test('Logout', async () => {
    const res = await request(app).delete('/api/auth/logout/U1');
    expect(res.status).toBe(200);
  });

  test('Sync endpoints', async () => {
    const users = await request(app).get('/api/sync/users');
    expect(users.status).toBe(200);
    const pages = await request(app).get('/api/sync/pages');
    expect(pages.status).toBe(200);
  });

  test('ACK/Ping/Message', async () => {
    const ack = await request(app).post('/api/acks').send({ msgId: 'M1', nodeId: 'NODE1' });
    expect(ack.status).toBe(200);
    const ping = await request(app).post('/api/pings').send({ nodeId: 'NODE1' });
    expect(ping.status).toBe(200);
    const msg = await request(app).post('/api/messages').send({ nodeId: 'NODE1' });
    expect(msg.status).toBe(200);
  });

  test('ACK/Ping/Message validation errors', async () => {
    const ack = await request(app).post('/api/acks').send({ nodeId: 'NODE1' });
    expect(ack.status).toBe(400);
    const ping = await request(app).post('/api/pings').send({});
    expect(ping.status).toBe(400);
    const msg = await request(app).post('/api/messages').send({});
    expect(msg.status).toBe(400);
  });

  test('Groups CRUD', async () => {
    const list = await request(app).get('/api/groups');
    expect(list.status).toBe(200);
    const create = await request(app).post('/api/groups').send({ name: 'Red', description: 'x' });
    expect(create.status).toBe(200);
    const getOne = await request(app).get('/api/groups/1');
    expect(getOne.status).toBe(200);
    const update = await request(app).put('/api/groups/1').send({ name: 'Red2' });
    expect(update.status).toBe(200);
    const del = await request(app).delete('/api/groups/1');
    expect(del.status).toBe(200);
  });

  test('Groups 404', async () => {
    mockPool.query.mockImplementationOnce(async () => [[]]);
    const res = await request(app).get('/api/groups/404');
    expect(res.status).toBe(404);
  });

  test('Groups error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/groups');
    expect(res.status).toBe(500);
  });

  test('Pages CRUD + list', async () => {
    const list = await request(app).get('/api/pages');
    expect(list.status).toBe(200);
    const all = await request(app).get('/api/pages/all');
    expect(all.status).toBe(200);
    const byNode = await request(app).get('/api/pages/NODE1');
    expect(byNode.status).toBe(200);
    const create = await request(app).post('/api/pages').send({ title: 'T', content: '<p>x</p>' });
    expect(create.status).toBe(200);
    const update = await request(app).put('/api/pages/P1').send({ title: 'T2' });
    expect(update.status).toBe(200);
    const del = await request(app).delete('/api/pages/P1');
    expect(del.status).toBe(200);
  });

  test('Pages create without nodes', async () => {
    mockPool.query.mockImplementationOnce(async () => [[]]);
    const res = await request(app).post('/api/pages').send({ title: 'T', content: '<p>x</p>' });
    expect(res.status).toBe(400);
  });

  test('Pages error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/pages');
    expect(res.status).toBe(500);
  });

  test('Topology', async () => {
    const res = await request(app).get('/api/topology');
    expect(res.status).toBe(200);
  });

  test('Topology error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/topology');
    expect(res.status).toBe(500);
  });

  test('Broadcasts', async () => {
    const create = await request(app).post('/api/broadcast').send({ username: 'sander', content: 'hi' });
    expect(create.status).toBe(201);
    axios.post.mockRejectedValueOnce(new Error('relay fail'));
    const createWarn = await request(app).post('/api/broadcast').send({ username: 'sander', content: 'hi' });
    expect(createWarn.status).toBe(201);
    const list = await request(app).get('/api/broadcasts');
    expect(list.status).toBe(200);
  });

  test('Broadcast validation', async () => {
    const res = await request(app).post('/api/broadcast').send({ content: 'hi' });
    expect(res.status).toBe(400);
  });
});

describe('Node host API', () => {
  beforeAll(() => {
    nodeRouter.setDbPoolForTests(mockPool, false);
  });

  test('Host endpoints create pool with env overrides', async () => {
    const mysql = require('mysql2/promise');
    const prevEnv = {
      DB_HOST: process.env.DB_HOST,
      DB_PORT: process.env.DB_PORT,
      DB_USER: process.env.DB_USER,
      DB_PASSWORD: process.env.DB_PASSWORD,
      DB_NAME: process.env.DB_NAME
    };

    process.env.DB_HOST = 'dbhost';
    process.env.DB_PORT = '3307';
    process.env.DB_USER = 'meshuser';
    process.env.DB_PASSWORD = 'meshpass';
    process.env.DB_NAME = 'meshdb';

    mysql.createPool.mockClear();
    nodeRouter.setDbPoolForTests(null, false);

    const res = await request(app).get('/api/host/node/NODE1/pages');
    expect(res.status).toBe(200);
    expect(mysql.createPool).toHaveBeenCalled();

    nodeRouter.setDbPoolForTests(mockPool, false);
    process.env.DB_HOST = prevEnv.DB_HOST;
    process.env.DB_PORT = prevEnv.DB_PORT;
    process.env.DB_USER = prevEnv.DB_USER;
    process.env.DB_PASSWORD = prevEnv.DB_PASSWORD;
    process.env.DB_NAME = prevEnv.DB_NAME;
  });

  test('Host endpoints create pool with defaults', async () => {
    const mysql = require('mysql2/promise');
    const prevEnv = {
      DB_HOST: process.env.DB_HOST,
      DB_PORT: process.env.DB_PORT,
      DB_USER: process.env.DB_USER,
      DB_PASSWORD: process.env.DB_PASSWORD,
      DB_NAME: process.env.DB_NAME
    };

    delete process.env.DB_HOST;
    delete process.env.DB_PORT;
    delete process.env.DB_USER;
    delete process.env.DB_PASSWORD;
    delete process.env.DB_NAME;

    mysql.createPool.mockClear();
    nodeRouter.setDbPoolForTests(null, false);

    const res = await request(app).get('/api/host/node/NODE1/pages');
    expect(res.status).toBe(200);
    expect(mysql.createPool).toHaveBeenCalled();

    nodeRouter.setDbPoolForTests(mockPool, false);
    process.env.DB_HOST = prevEnv.DB_HOST;
    process.env.DB_PORT = prevEnv.DB_PORT;
    process.env.DB_USER = prevEnv.DB_USER;
    process.env.DB_PASSWORD = prevEnv.DB_PASSWORD;
    process.env.DB_NAME = prevEnv.DB_NAME;
  });

  test('GET /api/host/node/:nodeId/pages', async () => {
    const res = await request(app).get('/api/host/node/NODE1/pages');
    expect(res.status).toBe(200);
  });

  test('GET /api/host/node/:nodeId/pages default refresh', async () => {
    mockPool.query.mockImplementationOnce(async () => [[{ pageId: 'P1', title: 'T', content: '', imageUrl: '', refreshInterval: 0, updatedAt: new Date() }]]);
    const res = await request(app).get('/api/host/node/NODE1/pages');
    expect(res.status).toBe(200);
    expect(res.body.pages[0].hasContent).toBe(false);
    expect(res.body.pages[0].hasImage).toBe(false);
    expect(res.body.pages[0].refreshInterval).toBe(30);
  });

  test('GET /api/host/node/:nodeId/pages error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/host/node/NODE1/pages');
    expect(res.status).toBe(500);
  });

  test('GET /api/host/node/:nodeId/pages/:pageId', async () => {
    mockPool.query.mockImplementationOnce(async () => [[{ pageId: 'P1', nodeId: 'NODE1', title: 'T', content: '<p>x</p>', imageUrl: null, refreshInterval: 30 }]]);
    const res = await request(app).get('/api/host/node/NODE1/pages/P1');
    expect(res.status).toBe(200);
  });

  test('GET /api/host/node/:nodeId/pages/:pageId default cache', async () => {
    mockPool.query.mockImplementationOnce(async () => [[{ pageId: 'P1', nodeId: 'NODE1', title: 'T', content: '<p>x</p>', imageUrl: null, refreshInterval: 0 }]]);
    const res = await request(app).get('/api/host/node/NODE1/pages/P1');
    expect(res.status).toBe(200);
    expect(res.headers['cache-control']).toContain('max-age=30');
  });

  test('GET /api/host/node/:nodeId/pages/:pageId 404', async () => {
    mockPool.query.mockImplementationOnce(async () => [[]]);
    const res = await request(app).get('/api/host/node/NODE1/pages/NOPE');
    expect(res.status).toBe(404);
  });

  test('GET /api/host/node/:nodeId/pages/:pageId error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/host/node/NODE1/pages/P1');
    expect(res.status).toBe(500);
  });

  test('GET /api/host/node/:nodeId/pages/:pageId/json', async () => {
    mockPool.query.mockImplementationOnce(async () => [[{ pageId: 'P1', nodeId: 'NODE1', title: 'T', content: '<p>x</p>', imageUrl: null, refreshInterval: 30 }]]);
    const res = await request(app).get('/api/host/node/NODE1/pages/P1/json');
    expect(res.status).toBe(200);
  });

  test('GET /api/host/node/:nodeId/pages/:pageId/json 404', async () => {
    mockPool.query.mockImplementationOnce(async () => [[]]);
    const res = await request(app).get('/api/host/node/NODE1/pages/NOPE/json');
    expect(res.status).toBe(404);
  });

  test('GET /api/host/node/:nodeId/pages/:pageId/json error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/host/node/NODE1/pages/P1/json');
    expect(res.status).toBe(500);
  });

  test('GET /api/host/node/:nodeId/info', async () => {
    mockPool.query.mockImplementationOnce(async () => [[{ nodeId: 'NODE1', functionalName: 'N1' }]]);
    const res = await request(app).get('/api/host/node/NODE1/info');
    expect(res.status).toBe(200);
  });

  test('GET /api/host/node/:nodeId/info 404', async () => {
    mockPool.query.mockImplementationOnce(async () => [[]]);
    const res = await request(app).get('/api/host/node/NODE1/info');
    expect(res.status).toBe(404);
  });

  test('GET /api/host/node/:nodeId/info error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/host/node/NODE1/info');
    expect(res.status).toBe(500);
  });

  test('POST /api/host/node/:nodeId/heartbeat', async () => {
    const res = await request(app).post('/api/host/node/NODE1/heartbeat').send({ battery: 100 });
    expect(res.status).toBe(200);
  });

  test('POST /api/host/node/:nodeId/heartbeat error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/host/node/NODE1/heartbeat').send({ battery: 100 });
    expect(res.status).toBe(500);
  });

  test('POST /api/host/node/:nodeId/heartbeat with signal and connections', async () => {
    const res = await request(app)
      .post('/api/host/node/NODE1/heartbeat')
      .send({ battery: 80, signalStrength: -70, connectedNodes: 3 });
    expect(res.status).toBe(200);
  });

  test('POST /api/host/node/:nodeId/heartbeat zero values', async () => {
    const res = await request(app)
      .post('/api/host/node/NODE1/heartbeat')
      .send({ battery: 0, signalStrength: 0, connectedNodes: 0 });
    expect(res.status).toBe(200);
  });

  test('POST /api/host/node/:nodeId/connection', async () => {
    const res = await request(app).post('/api/host/node/NODE1/connection').send({ username: 'sander', teamName: 'Red Team' });
    expect(res.status).toBe(200);
  });

  test('POST /api/host/node/:nodeId/connection with MAC', async () => {
    const res = await request(app).post('/api/host/node/AA:BB:CC:DD:EE:FF/connection').send({ username: 'sander', teamName: 'Red Team' });
    expect(res.status).toBe(200);
  });

  test('POST /api/host/node/:nodeId/connection insert warning', async () => {
    mockPool.query
      .mockImplementationOnce(async () => { throw new Error('insert fail'); })
      .mockImplementationOnce(async () => [[]]);
    const res = await request(app).post('/api/host/node/NODE1/connection').send({ username: 'sander', teamName: 'Red Team' });
    expect(res.status).toBe(200);
  });

  test('POST /api/host/node/:nodeId/connection error', async () => {
    mockPool.query
      .mockImplementationOnce(async () => [[]])
      .mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/host/node/NODE1/connection').send({ username: 'sander', teamName: 'Red Team' });
    expect(res.status).toBe(500);
  });

  test('POST /api/host/node/:nodeId/connection validation', async () => {
    const res = await request(app).post('/api/host/node/NODE1/connection').send({ username: 'sander' });
    expect(res.status).toBe(400);
  });

  test('POST /api/host/node/:nodeId/connection validation missing username', async () => {
    const res = await request(app).post('/api/host/node/NODE1/connection').send({ teamName: 'Red Team' });
    expect(res.status).toBe(400);
  });

  test('GET /api/host/node/:nodeId/connections', async () => {
    const res = await request(app).get('/api/host/node/NODE1/connections');
    expect(res.status).toBe(200);
  });

  test('GET /api/host/node/:nodeId/connections empty fallback', async () => {
    mockPool.query.mockImplementationOnce(async () => [null]);
    const res = await request(app).get('/api/host/node/NODE1/connections');
    expect(res.status).toBe(200);
    expect(res.body.connections).toEqual([]);
  });

  test('GET /api/host/node/:nodeId/connections error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/host/node/NODE1/connections');
    expect(res.status).toBe(500);
  });

  test('Host endpoints db unavailable', async () => {
    nodeRouter.setDbPoolForTests(null, true);
    const pages = await request(app).get('/api/host/node/NODE1/pages');
    expect(pages.status).toBe(503);
    const pageHtml = await request(app).get('/api/host/node/NODE1/pages/P1');
    expect(pageHtml.status).toBe(503);
    const pageJson = await request(app).get('/api/host/node/NODE1/pages/P1/json');
    expect(pageJson.status).toBe(503);
    const info = await request(app).get('/api/host/node/NODE1/info');
    expect(info.status).toBe(503);
    const heartbeat = await request(app).post('/api/host/node/NODE1/heartbeat').send({ battery: 50 });
    expect(heartbeat.status).toBe(503);
    const connection = await request(app).post('/api/host/node/NODE1/connection').send({ username: 'sander', teamName: 'Red Team' });
    expect(connection.status).toBe(503);
    const list = await request(app).get('/api/host/node/NODE1/connections');
    expect(list.status).toBe(503);
    nodeRouter.setDbPoolForTests(mockPool, false);
  });
});

describe('Nodes module endpoints', () => {
  test('Nodes module create pool with defaults', async () => {
    const mysql = require('mysql2/promise');
    const prevEnv = {
      DB_HOST: process.env.DB_HOST,
      DB_PORT: process.env.DB_PORT,
      DB_USER: process.env.DB_USER,
      DB_PASSWORD: process.env.DB_PASSWORD,
      DB_NAME: process.env.DB_NAME
    };

    delete process.env.DB_HOST;
    delete process.env.DB_PORT;
    delete process.env.DB_USER;
    delete process.env.DB_PASSWORD;
    delete process.env.DB_NAME;

    mysql.createPool.mockClear();
    nodeConnections.setDbPoolForTests(null, false);

    const res = await request(app)
      .post('/api/nodes/connection')
      .send({ nodeId: 'NODE1', username: 'sander', teamName: 'Red Team' });
    expect(res.status).toBe(200);
    expect(mysql.createPool).toHaveBeenCalled();

    nodeConnections.setDbPoolForTests(mockPool, false);
    process.env.DB_HOST = prevEnv.DB_HOST;
    process.env.DB_PORT = prevEnv.DB_PORT;
    process.env.DB_USER = prevEnv.DB_USER;
    process.env.DB_PASSWORD = prevEnv.DB_PASSWORD;
    process.env.DB_NAME = prevEnv.DB_NAME;
  });

  test('Nodes module setDbPoolForTests default param', async () => {
    nodeConnections.setDbPoolForTests(mockPool);
    const res = await request(app).get('/api/nodes/NODE1/connections');
    expect(res.status).toBe(200);
  });

  test('POST /api/nodes/connection', async () => {
    const res = await request(app).post('/api/nodes/connection').send({ nodeId: 'NODE1', username: 'sander', teamName: 'Red Team' });
    expect(res.status).toBe(200);
  });

  test('POST /api/nodes/connection validation', async () => {
    const res = await request(app).post('/api/nodes/connection').send({ nodeId: 'NODE1' });
    expect(res.status).toBe(400);
  });

  test('POST /api/nodes/connection error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app)
      .post('/api/nodes/connection')
      .send({ nodeId: 'NODE1', username: 'sander', teamName: 'Red Team' });
    expect(res.status).toBe(500);
  });

  test('POST /api/nodes/connection db unavailable', async () => {
    nodeConnections.setDbPoolForTests(null, true);
    const res = await request(app).post('/api/nodes/connection').send({ nodeId: 'NODE1', username: 'sander', teamName: 'Red Team' });
    expect(res.status).toBe(503);
    nodeConnections.setDbPoolForTests(mockPool, false);
  });

  test('GET /api/nodes/:nodeId/connections', async () => {
    const res = await request(app).get('/api/nodes/NODE1/connections');
    expect(res.status).toBe(200);
  });

  test('GET /api/nodes/:nodeId/connections empty fallback', async () => {
    mockPool.query.mockImplementationOnce(async () => [null]);
    const res = await request(app).get('/api/nodes/NODE1/connections');
    expect(res.status).toBe(200);
    expect(res.body.connections).toEqual([]);
  });

  test('GET /api/nodes/:nodeId/connections error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/nodes/NODE1/connections');
    expect(res.status).toBe(500);
  });

  test('GET /api/nodes/:nodeId/connections db unavailable', async () => {
    nodeConnections.setDbPoolForTests(null, true);
    const res = await request(app).get('/api/nodes/NODE1/connections');
    expect(res.status).toBe(503);
    nodeConnections.setDbPoolForTests(mockPool, false);
  });

  test('GET /api/nodes/stats/connections', async () => {
    const res = await request(app).get('/api/nodes/stats/connections');
    expect(res.status).toBe(200);
  });

  test('GET /api/nodes/stats/connections error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/nodes/stats/connections');
    expect(res.status).toBe(500);
  });

  test('GET /api/nodes/stats/connections db unavailable', async () => {
    nodeConnections.setDbPoolForTests(null, true);
    const res = await request(app).get('/api/nodes/stats/connections');
    expect(res.status).toBe(503);
    nodeConnections.setDbPoolForTests(mockPool, false);
  });

  test('GET /dashboard/nodes', async () => {
    const res = await request(app).get('/dashboard/nodes');
    expect(res.status).toBe(200);
  });

  test('GET /dashboard/nodes init pool path', async () => {
    const mysql = require('mysql2/promise');
    mysql.createPool.mockClear();
    nodeConnections.setDbPoolForTests(null, false);

    const res = await request(app).get('/dashboard/nodes');
    expect(res.status).toBe(200);
    expect(mysql.createPool).toHaveBeenCalled();

    nodeConnections.setDbPoolForTests(mockPool, false);
  });

  test('GET /dashboard/nodes with connections and escaping', async () => {
    nodeConnections.setDbPoolForTests(mockPool, false);
    mockPool.query
      .mockImplementationOnce(async () => [[{ nodeId: 'NODE1', functionalName: 'Node & <1>' }]])
      .mockImplementationOnce(async () => [[{ teamName: 'Red & Team', username: 'u<1>', lastSeen: new Date() }]])
      .mockImplementationOnce(async () => [[{ teamName: 'Red & Team', username: 'u<1>', lastSeen: new Date() }]]);

    const res = await request(app).get('/dashboard/nodes');
    expect(res.status).toBe(200);
    expect(res.text).toContain('Node &amp; &lt;1&gt;');
    expect(res.text).toContain('Red &amp; Team');
    expect(res.text).toContain('u&lt;1&gt;');
  });

  test('GET /dashboard/nodes with null connections list', async () => {
    nodeConnections.setDbPoolForTests(mockPool, false);
    mockPool.query
      .mockImplementationOnce(async () => [[{ nodeId: 'NODE2', functionalName: '' }]])
      .mockImplementationOnce(async () => [null])
      .mockImplementationOnce(async () => [null]);

    const res = await request(app).get('/dashboard/nodes');
    expect(res.status).toBe(200);
    expect(res.text).toContain('No team connections recorded');
    expect(res.text).toContain('NODE2');
  });

  test('GET /dashboard/nodes db unavailable', async () => {
    nodeConnections.setDbPoolForTests(null, true);
    const res = await request(app).get('/dashboard/nodes');
    expect(res.status).toBe(200);
    expect(res.text).toContain('Database not available');
    nodeConnections.setDbPoolForTests(mockPool, false);
  });

  test('GET /dashboard/nodes without connections', async () => {
    mockPool.query
      .mockImplementationOnce(async () => [[{ nodeId: '', functionalName: '' }]])
      .mockImplementationOnce(async () => [[]])
      .mockImplementationOnce(async () => [[]]);
    const res = await request(app).get('/dashboard/nodes');
    expect(res.status).toBe(200);
    expect(res.text).toContain('No team connections recorded');
  });

  test('GET /dashboard/nodes error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/dashboard/nodes');
    expect(res.status).toBe(500);
  });

  test('GET /api/nodes/dashboard', async () => {
    const res = await request(app).get('/api/nodes/dashboard');
    expect(res.status).toBe(200);
  });
});

describe('Error paths (db errors)', () => {
  test('GET /api/nodes error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/nodes');
    expect(res.status).toBe(500);
  });

  test('GET /api/users error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/users');
    expect(res.status).toBe(500);
  });

  test('GET /api/users/:userId error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/users/U1');
    expect(res.status).toBe(500);
  });

  test('POST /api/users error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/users').send({ username: 'sander', password: 'test', groupId: 1 });
    expect(res.status).toBe(500);
  });

  test('PUT /api/users/:userId error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).put('/api/users/U1').send({ username: 'x' });
    expect(res.status).toBe(500);
  });

  test('DELETE /api/users/:userId error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).delete('/api/users/U1');
    expect(res.status).toBe(500);
  });

  test('POST /api/auth/login error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/auth/login').send({ username: 'sander', password: 'test' });
    expect(res.status).toBe(500);
  });

  test('GET /api/sync/users error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/sync/users');
    expect(res.status).toBe(500);
  });

  test('GET /api/sync/pages error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).get('/api/sync/pages');
    expect(res.status).toBe(500);
  });

  test('POST /api/acks error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/acks').send({ msgId: 'M1', nodeId: 'NODE1' });
    expect(res.status).toBe(500);
  });

  test('POST /api/pings error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/pings').send({ nodeId: 'NODE1' });
    expect(res.status).toBe(500);
  });

  test('POST /api/messages error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/messages').send({ nodeId: 'NODE1' });
    expect(res.status).toBe(500);
  });

  test('POST /api/nodes/register error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/nodes/register').send({ nodeId: 'NODE1' });
    expect(res.status).toBe(500);
  });

  test('POST /api/nodes/:nodeId/update error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/nodes/NODE1/update').send({ functionalName: 'N1' });
    expect(res.status).toBe(500);
  });

  test('PUT /api/nodes/:nodeId error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).put('/api/nodes/NODE1').send({ functionalName: 'N1' });
    expect(res.status).toBe(500);
  });

  test('DELETE /api/nodes/:nodeId error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).delete('/api/nodes/NODE1');
    expect(res.status).toBe(500);
  });

  test('POST /api/pages error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/pages').send({ title: 'T', content: '<p>x</p>' });
    expect(res.status).toBe(400);
  });

  test('PUT /api/pages/:pageId error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).put('/api/pages/P1').send({ title: 'T2' });
    expect(res.status).toBe(500);
  });

  test('DELETE /api/pages/:pageId error', async () => {
    mockPool.query.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).delete('/api/pages/P1');
    expect(res.status).toBe(500);
  });

  test('POST /api/broadcast error', async () => {
    mockPool.getConnection.mockImplementationOnce(async () => { throw new Error('boom'); });
    const res = await request(app).post('/api/broadcast').send({ username: 'sander', content: 'hi' });
    expect(res.status).toBe(500);
  });
});

describe('Database unavailable paths', () => {
  beforeAll(() => {
    app.setDbPoolForTests(null);
  });

  afterAll(() => {
    app.setDbPoolForTests(mockPool);
  });

  test('GET /api/users returns []', async () => {
    const res = await request(app).get('/api/users');
    expect(res.status).toBe(200);
    expect(res.body).toEqual([]);
  });

  test('GET /api/nodes returns []', async () => {
    const res = await request(app).get('/api/nodes');
    expect(res.status).toBe(200);
    expect(res.body).toEqual([]);
  });

  test('GET /api/nodes/:nodeId returns 404', async () => {
    const res = await request(app).get('/api/nodes/NODE1');
    expect(res.status).toBe(404);
  });

  test('GET /api/users/:userId returns 404', async () => {
    const res = await request(app).get('/api/users/U1');
    expect(res.status).toBe(404);
  });

  test('POST /api/users returns 503', async () => {
    const res = await request(app).post('/api/users').send({ username: 'x', password: 'y', groupId: 1 });
    expect(res.status).toBe(503);
  });

  test('POST /api/nodes returns 503', async () => {
    const res = await request(app).post('/api/nodes').send({ functionalName: 'N1' });
    expect(res.status).toBe(503);
  });

  test('POST /api/nodes/register returns warning', async () => {
    const res = await request(app).post('/api/nodes/register').send({ nodeId: 'NODE1' });
    expect(res.status).toBe(200);
    expect(res.body.warning).toBeDefined();
  });

  test('POST /api/nodes/:nodeId/update returns warning', async () => {
    const res = await request(app).post('/api/nodes/NODE1/update').send({ functionalName: 'N1' });
    expect(res.status).toBe(200);
    expect(res.body.warning).toBeDefined();
  });

  test('PUT /api/nodes/:nodeId returns warning', async () => {
    const res = await request(app).put('/api/nodes/NODE1').send({ functionalName: 'N1' });
    expect(res.status).toBe(200);
    expect(res.body.warning).toBeDefined();
  });

  test('DELETE /api/nodes/:nodeId returns warning', async () => {
    const res = await request(app).delete('/api/nodes/NODE1');
    expect(res.status).toBe(200);
    expect(res.body.warning).toBeDefined();
  });

  test('POST /api/auth/login returns 503', async () => {
    const res = await request(app).post('/api/auth/login').send({ username: 'x', password: 'y' });
    expect(res.status).toBe(503);
  });

  test('GET /api/sync/users returns empty', async () => {
    const res = await request(app).get('/api/sync/users');
    expect(res.status).toBe(200);
    expect(res.body.user_count).toBe(0);
  });

  test('GET /api/sync/pages returns empty', async () => {
    const res = await request(app).get('/api/sync/pages');
    expect(res.status).toBe(200);
    expect(res.body.page_count).toBe(0);
  });

  test('ACK/Ping/Message return warning', async () => {
    const ack = await request(app).post('/api/acks').send({ msgId: 'M1', nodeId: 'NODE1' });
    expect(ack.status).toBe(200);
    expect(ack.body.warning).toBeDefined();
    const ping = await request(app).post('/api/pings').send({ nodeId: 'NODE1' });
    expect(ping.body.warning).toBeDefined();
    const msg = await request(app).post('/api/messages').send({ nodeId: 'NODE1' });
    expect(msg.body.warning).toBeDefined();
  });

  test('Groups/pages/topology return empty', async () => {
    const groups = await request(app).get('/api/groups');
    expect(groups.status).toBe(200);
    const pages = await request(app).get('/api/pages');
    expect(pages.status).toBe(200);
    const allPages = await request(app).get('/api/pages/all');
    expect(allPages.status).toBe(200);
    const nodePages = await request(app).get('/api/pages/NODE1');
    expect(nodePages.status).toBe(200);
    const topo = await request(app).get('/api/topology');
    expect(topo.status).toBe(200);
  });

  test('POST /api/broadcast returns 503', async () => {
    const res = await request(app).post('/api/broadcast').send({ username: 'x', content: 'y' });
    expect(res.status).toBe(503);
  });

  test('GET /api/broadcasts returns empty with error', async () => {
    const res = await request(app).get('/api/broadcasts');
    expect(res.status).toBe(200);
    expect(res.body.broadcasts).toEqual([]);
  });
});
