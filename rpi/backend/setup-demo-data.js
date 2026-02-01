// Setup Demo Data for MeshNet
// Creates 3 nodes, admin + test group, and web pages for hosting

const mysql = require('mysql2/promise');
const { v4: uuidv4 } = require('uuid');
const bcryptjs = require('bcryptjs');

async function setupDemoData() {
  try {
    const pool = mysql.createPool({
      host: process.env.DB_HOST || 'mysql',
      port: process.env.DB_PORT || 3306,
      user: process.env.DB_USER || 'meshnet',
      password: process.env.DB_PASSWORD || 'meshnet_secure_pwd',
      database: process.env.DB_NAME || 'meshnet',
      waitForConnections: true,
      connectionLimit: 10
    });

    console.log('🔧 Setting up demo data...\n');

    // 1. Create Groups
    console.log('📋 Creating groups...');
    const adminGroupId = uuidv4();
    const testGroupId = uuidv4();

    await pool.query(
      'INSERT IGNORE INTO `groups` (groupId, name, description, permissions) VALUES (?, ?, ?, ?)',
      [adminGroupId, 'Admin', 'Administrator group with full permissions', JSON.stringify(["ALL"])]
    );

    await pool.query(
      'INSERT IGNORE INTO `groups` (groupId, name, description, permissions) VALUES (?, ?, ?, ?)',
      [testGroupId, 'Test', 'Test group for demo nodes', JSON.stringify(["READ", "WRITE"])]
    );

    console.log('✓ Admin group:', adminGroupId);
    console.log('✓ Test group:', testGroupId);

    // 2. Create Admin User
    console.log('\n👤 Creating admin user...');
    const adminUserId = uuidv4();
    const adminPasswordHash = await bcryptjs.hash('admin123', 10);

    await pool.query(
      'INSERT IGNORE INTO users (userId, username, email, passwordHash, groupId, isActive) VALUES (?, ?, ?, ?, ?, true)',
      [adminUserId, 'admin', 'admin@meshnet.local', adminPasswordHash, adminGroupId]
    );

    console.log('✓ Admin user:', adminUserId);
    console.log('  Username: admin');
    console.log('  Password: admin123');

    // 3. Create/Get 3 Nodes
    console.log('\n📡 Creating/Getting 3 nodes...');
    let nodes = [];

    // Check if nodes already exist
    const [existingNodes] = await pool.query(
      'SELECT nodeId, functionalName FROM nodes WHERE functionalName LIKE ? ORDER BY functionalName LIMIT 3',
      ['MeshNode-%']
    );

    if (existingNodes.length >= 3) {
      // Use existing nodes
      nodes = existingNodes.map((node, i) => ({
        id: node.nodeId,
        name: node.functionalName
      }));
      console.log('✓ Using existing nodes:');
      nodes.forEach(n => console.log(`  - ${n.name} (${n.id})`));
    } else {
      // Create new nodes
      for (let i = 1; i <= 3; i++) {
        const nodeId = uuidv4();
        const macAddress = `AA:BB:CC:DD:EE:0${i}`;
        const functionalName = `MeshNode-${i}`;

        await pool.query(
          'INSERT IGNORE INTO nodes (nodeId, macAddress, functionalName, version, isActive) VALUES (?, ?, ?, ?, true)',
          [nodeId, macAddress, functionalName, '1.0.0']
        );

        nodes.push({
          id: nodeId,
          name: functionalName
        });

        console.log(`✓ ${functionalName} (${macAddress})`);
        console.log(`  ID: ${nodeId}`);
      }
    }

    // 4. Create Web Pages
    console.log('\n📄 Creating web pages...');
    const pages = [];

    // Page 1: About (with image)
    const page1Id = uuidv4();
    const page1Content = `<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Over MeshNode-1</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; }
        .container { background: white; border-radius: 12px; box-shadow: 0 20px 60px rgba(0,0,0,0.3); overflow: hidden; max-width: 600px; }
        .header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 40px 20px; text-align: center; }
        .header h1 { font-size: 2em; margin-bottom: 10px; }
        .header p { font-size: 1.1em; opacity: 0.9; }
        .content { padding: 40px; }
        .content img { width: 100%; max-width: 100%; height: auto; border-radius: 8px; margin: 20px 0; box-shadow: 0 5px 15px rgba(0,0,0,0.1); }
        .info-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin: 30px 0; }
        .info-card { background: #f5f5f5; padding: 15px; border-radius: 8px; border-left: 4px solid #667eea; }
        .info-card h3 { color: #667eea; margin-bottom: 5px; }
        .info-card p { color: #666; font-size: 0.95em; }
        footer { background: #f5f5f5; padding: 20px; text-align: center; color: #666; font-size: 0.9em; border-top: 1px solid #ddd; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌐 MeshNode-1</h1>
            <p>LoRa Mesh Network Node</p>
        </div>
        <div class="content">
            <h2>Over Dit Node</h2>
            <p>MeshNode-1 is een geavanceerd IoT-apparaat dat deel uitmaakt van ons LoRa mesh network. Dit node fungeert als communicatiepunt in het gedistribueerde netwerk.</p>
            
            <img src="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 400 300'%3E%3Crect fill='%23667eea' width='400' height='300'/%3E%3Ctext x='200' y='150' font-size='48' fill='white' text-anchor='middle' dominant-baseline='middle'%3E📡 LoRa Node%3C/text%3E%3C/svg%3E" alt="Node Illustration">
            
            <div class="info-grid">
                <div class="info-card">
                    <h3>📊 Status</h3>
                    <p>Online en actief</p>
                </div>
                <div class="info-card">
                    <h3>🔋 Batterie</h3>
                    <p>95% geladen</p>
                </div>
                <div class="info-card">
                    <h3>📡 Signaal</h3>
                    <p>-95 dBm</p>
                </div>
                <div class="info-card">
                    <h3>⚙️ Uptime</h3>
                    <p>7 dagen 3 uur</p>
                </div>
            </div>
            
            <h3>Functies</h3>
            <ul style="margin-left: 20px; line-height: 2;">
                <li>LoRa communicatie (868 MHz)</li>
                <li>Real-time gegevensoverdracht</li>
                <li>Mesh relay mogelijkheden</li>
                <li>Laag energieverbruik</li>
                <li>Web interface hosting</li>
            </ul>
        </div>
        <footer>
            <p>&copy; 2026 MeshNet | Node-1 @ 192.168.1.101</p>
        </footer>
    </div>
</body>
</html>`;

    await pool.query(
      'INSERT INTO pages (pageId, nodeId, title, content, imageUrl, isActive) VALUES (?, ?, ?, ?, ?, true)',
      [page1Id, nodes[0].id, 'Over MeshNode-1', page1Content, null]
    ).catch(err => {
      if (err.code !== 'ER_DUP_ENTRY') throw err;
    });
    console.log('✓ Page 1: "Over MeshNode-1" (with embedded image)');

    // Page 2: Dashboard (with image)
    const page2Id = uuidv4();
    const page2Content = `<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dashboard - MeshNode-2</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); min-height: 100vh; padding: 20px; }
        .container { max-width: 900px; margin: 0 auto; }
        .header { background: white; padding: 30px; border-radius: 12px; margin-bottom: 20px; box-shadow: 0 5px 15px rgba(0,0,0,0.1); }
        .header h1 { color: #f5576c; margin-bottom: 10px; }
        .header p { color: #666; }
        .dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin: 20px 0; }
        .card { background: white; padding: 20px; border-radius: 12px; box-shadow: 0 5px 15px rgba(0,0,0,0.1); border-top: 4px solid #f5576c; }
        .card h3 { color: #f5576c; margin-bottom: 15px; font-size: 1.1em; }
        .metric { display: flex; justify-content: space-between; align-items: center; margin: 10px 0; padding: 10px; background: #f5f5f5; border-radius: 6px; }
        .metric-label { color: #666; }
        .metric-value { font-size: 1.3em; font-weight: bold; color: #f5576c; }
        .chart-container { background: #f5f5f5; height: 200px; border-radius: 8px; display: flex; align-items: center; justify-content: center; color: #999; margin: 20px 0; }
        .card img { width: 100%; border-radius: 8px; margin: 15px 0; }
        .status-online { color: #27ae60; font-weight: bold; }
        .status-indicator { display: inline-block; width: 12px; height: 12px; border-radius: 50%; background: #27ae60; margin-right: 8px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>📊 Dashboard MeshNode-2</h1>
            <p><span class="status-indicator"></span><span class="status-online">Online</span></p>
        </div>
        
        <div class="dashboard">
            <div class="card">
                <h3>🔋 Stroombeheer</h3>
                <div class="metric">
                    <span class="metric-label">Batterie</span>
                    <span class="metric-value">87%</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Spanning</span>
                    <span class="metric-value">4.2V</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Stroom</span>
                    <span class="metric-value">125mA</span>
                </div>
            </div>
            
            <div class="card">
                <h3>📡 Netwerk</h3>
                <div class="metric">
                    <span class="metric-label">RSSI</span>
                    <span class="metric-value">-92 dBm</span>
                </div>
                <div class="metric">
                    <span class="metric-label">SNR</span>
                    <span class="metric-value">7.5 dB</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Paketten</span>
                    <span class="metric-value">1247</span>
                </div>
            </div>
            
            <div class="card">
                <h3>⚙️ Systeem</h3>
                <div class="metric">
                    <span class="metric-label">Uptime</span>
                    <span class="metric-value">14d 5h</span>
                </div>
                <div class="metric">
                    <span class="metric-label">CPU Temp</span>
                    <span class="metric-value">45°C</span>
                </div>
                <div class="metric">
                    <span class="metric-label">RAM</span>
                    <span class="metric-value">62%</span>
                </div>
            </div>
        </div>
        
        <div class="card" style="margin-top: 20px;">
            <h3>📈 Activiteitsoverzicht</h3>
            <img src="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 600 300'%3E%3Crect fill='%23f5f5f5' width='600' height='300'/%3E%3Ctext x='300' y='150' font-size='36' fill='%23ccc' text-anchor='middle' dominant-baseline='middle'%3E📊 Activiteitsgrafiek%3C/text%3E%3C/svg%3E" alt="Activity Chart">
            <p style="color: #666; font-size: 0.9em; text-align: center; margin-top: 10px;">Toont de activiteit van de afgelopen 24 uur</p>
        </div>
    </div>
</body>
</html>`;

    await pool.query(
      'INSERT INTO pages (pageId, nodeId, title, content, imageUrl, isActive) VALUES (?, ?, ?, ?, ?, true)',
      [page2Id, nodes[1].id, 'Dashboard', page2Content, null]
    ).catch(err => {
      if (err.code !== 'ER_DUP_ENTRY') throw err;
    });
    console.log('✓ Page 2: "Dashboard" (with embedded image)');

    // Page 3: Status (no image)
    const page3Id = uuidv4();
    const page3Content = `<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Status - MeshNode-3</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%); min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; }
        .container { background: white; border-radius: 12px; box-shadow: 0 20px 60px rgba(0,0,0,0.3); overflow: hidden; max-width: 700px; }
        .header { background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%); color: white; padding: 40px 20px; text-align: center; }
        .header h1 { font-size: 2em; margin-bottom: 10px; }
        .content { padding: 40px; }
        .status-table { width: 100%; border-collapse: collapse; margin: 20px 0; }
        .status-table th { background: #f5f5f5; padding: 12px; text-align: left; color: #333; font-weight: 600; border-bottom: 2px solid #4facfe; }
        .status-table td { padding: 12px; border-bottom: 1px solid #ddd; }
        .status-table tr:hover { background: #f9f9f9; }
        .status-active { color: #27ae60; font-weight: bold; }
        .status-idle { color: #f39c12; font-weight: bold; }
        .status-offline { color: #e74c3c; font-weight: bold; }
        .event-log { background: #f5f5f5; padding: 20px; border-radius: 8px; margin: 20px 0; }
        .event-log h3 { color: #4facfe; margin-bottom: 15px; }
        .event { padding: 10px; margin: 10px 0; background: white; border-left: 3px solid #4facfe; border-radius: 4px; }
        .event-time { color: #999; font-size: 0.85em; }
        .event-msg { color: #333; margin-top: 5px; }
        footer { background: #f5f5f5; padding: 20px; text-align: center; color: #666; font-size: 0.9em; border-top: 1px solid #ddd; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔍 Status MeshNode-3</h1>
            <p>Real-time Status en Monitoring</p>
        </div>
        <div class="content">
            <h2>Huidigen Status</h2>
            <table class="status-table">
                <thead>
                    <tr>
                        <th>Component</th>
                        <th>Status</th>
                        <th>Details</th>
                    </tr>
                </thead>
                <tbody>
                    <tr>
                        <td>LoRa Radio</td>
                        <td><span class="status-active">● Actief</span></td>
                        <td>868 MHz, 14 dBm</td>
                    </tr>
                    <tr>
                        <td>WiFi Interface</td>
                        <td><span class="status-active">● Actief</span></td>
                        <td>192.168.1.103</td>
                    </tr>
                    <tr>
                        <td>Sensor Array</td>
                        <td><span class="status-active">● Actief</span></td>
                        <td>4 sensoren online</td>
                    </tr>
                    <tr>
                        <td>Webserver</td>
                        <td><span class="status-active">● Actief</span></td>
                        <td>HTTP/80</td>
                    </tr>
                    <tr>
                        <td>SD Card</td>
                        <td><span class="status-active">● OK</span></td>
                        <td>4.2 GB vrij</td>
                    </tr>
                </tbody>
            </table>

            <div class="event-log">
                <h3>📋 Recente Gebeurtenissen</h3>
                <div class="event">
                    <div class="event-time">28-01-2026 20:45:32</div>
                    <div class="event-msg">✓ Verbinding met mesh network hersteld</div>
                </div>
                <div class="event">
                    <div class="event-time">28-01-2026 20:40:15</div>
                    <div class="event-msg">⚠ Signaal zwak, retransmit ingeschakeld</div>
                </div>
                <div class="event">
                    <div class="event-time">28-01-2026 20:30:00</div>
                    <div class="event-msg">✓ 3 pakketten verzonden naar gateway</div>
                </div>
                <div class="event">
                    <div class="event-time">28-01-2026 20:15:22</div>
                    <div class="event-msg">✓ Sensorgegevens gesynchroniseerd</div>
                </div>
                <div class="event">
                    <div class="event-time">28-01-2026 20:00:00</div>
                    <div class="event-msg">✓ Dagelijkse backup voltooid</div>
                </div>
            </div>
        </div>
        <footer>
            <p>&copy; 2026 MeshNet | Status Dashboard v1.0</p>
        </footer>
    </div>
</body>
</html>`;

    await pool.query(
      'INSERT INTO pages (pageId, nodeId, title, content, imageUrl, isActive) VALUES (?, ?, ?, ?, ?, true)',
      [page3Id, nodes[2].id, 'Status', page3Content, null]
    ).catch(err => {
      if (err.code !== 'ER_DUP_ENTRY') throw err;
    });
    console.log('✓ Page 3: "Status"');

    // Summary
    console.log('\n✅ Demo data setup complete!\n');
    console.log('Summary:');
    console.log('========');
    console.log('✓ 2 Groups created (Admin, Test)');
    console.log('✓ 1 Admin user created');
    console.log('✓ 3 Nodes created and configured');
    console.log('✓ 3 Web pages created (2 with embedded images)\n');
    console.log('\nNodes configured:');
    nodes.forEach((node, i) => {
      console.log(`  ${i + 1}. ${node.name} (${node.id})`);
    });
    console.log('\nWeb pages:');
    console.log('  1. MeshNode-1 → "Over MeshNode-1" (with image)');
    console.log('  2. MeshNode-2 → "Dashboard" (with image)');
    console.log('  3. MeshNode-3 → "Status" (no image)');

    await pool.end();
    process.exit(0);

  } catch (error) {
    console.error('❌ Setup failed:', error.message);
    console.error(error);
    process.exit(1);
  }
}

setupDemoData();
