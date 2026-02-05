from pathlib import Path

backend_path = Path('/home/copilot/meshnet/rpi/backend/src/server.js')
webserver_path = Path('/home/copilot/meshnet/rpi/webserver/server.js')
pages_view_path = Path('/home/copilot/meshnet/rpi/webserver/views/pages.ejs')

text = backend_path.read_text()
old = '''function extractVersionFromNodeId(nodeId) {
  if (!nodeId) return null;
  const match = nodeId.match(/_(\d+\.\d+\.\d+)$/);
  return match ? match[1] : null;
}
'''
new = old + '''
function stripVersionFromNodeId(nodeId) {
  if (!nodeId) return null;
  return nodeId.replace(/_(\d+\.\d+\.\d+)$/, '');
}
'''
if old not in text:
    raise SystemExit('Failed to find extractVersionFromNodeId block')
text = text.replace(old, new)

marker = '''}

// ============ API ROUTES ============
'''
insert = '''}

function buildDefaultPageContent(groupName, nodeIdBase) {
  const safeGroup = groupName || 'Team';
  const safeNode = nodeIdBase || 'Unknown Node';
  return `${safeGroup} page for ${safeNode}`;
}

async function ensureDefaultPagesForNode(nodeId) {
  if (!dbPool || !nodeId) return 0;

  const nodeIdBase = stripVersionFromNodeId(nodeId) || nodeId;
  const macAddress = extractMacFromNodeId(nodeId) || nodeId.substring(0, 17);

  await dbPool.query(
    'INSERT INTO nodes (nodeId, macAddress, functionalName, isActive, lastSeen) VALUES (?, ?, ?, true, NOW()) ON DUPLICATE KEY UPDATE lastSeen = NOW()',
    [nodeId, macAddress, nodeId]
  );

  const [groups] = await dbPool.query('SELECT id, name FROM `groups`');
  if (!groups.length) return 0;

  const [existingPages] = await dbPool.query(
    'SELECT groupId FROM pages WHERE nodeId = ? AND groupId IS NOT NULL',
    [nodeId]
  );
  const existingGroupIds = new Set(existingPages.map(row => row.groupId));

  let created = 0;
  for (const group of groups) {
    if (existingGroupIds.has(group.id)) continue;
    const pageId = uuidv4();
    const title = `${group.name} Page`;
    const content = buildDefaultPageContent(group.name, nodeIdBase);
    await dbPool.query(
      'INSERT INTO pages (pageId, nodeId, groupId, title, content, refreshInterval, isActive) VALUES (?, ?, ?, ?, ?, ?, true)',
      [pageId, nodeId, group.id, title, content, 30]
    );
    created += 1;
  }

  return created;
}

// ============ API ROUTES ============
'''
if marker not in text:
    raise SystemExit('Failed to find API ROUTES marker')
text = text.replace(marker, insert)

old_sync = '''// Sync pages for nodes
app.get('/api/sync/pages', async (req, res) => {
  try {
    if (!dbPool) return res.json({ status: 'success', page_count: 0, pages: [] });
    const [rows] = await dbPool.query(
      'SELECT g.name AS team, p.content AS html '
      + 'FROM pages p '
      + 'JOIN `groups` g ON p.groupId = g.id '
      + 'WHERE p.isActive = true '
      + 'ORDER BY g.name'
    );
    const pages = rows.map(r => ({ team: r.team, html: r.html || '' }));
    res.json({ status: 'success', page_count: pages.length, pages });
  } catch (error) {
    res.status(500).json({ status: 'error', message: error.message });
  }
});
'''
new_sync = '''// Sync pages for nodes
app.get('/api/sync/pages', async (req, res) => {
  try {
    if (!dbPool) return res.json({ status: 'success', page_count: 0, pages: [] });
    const nodeId = req.query.nodeId || '';
    if (nodeId) {
      await ensureDefaultPagesForNode(nodeId);
    }

    const nodeIdBase = nodeId ? (stripVersionFromNodeId(nodeId) || nodeId) : '';
    const likePattern = nodeIdBase ? nodeIdBase.replace(/[!%_]/g, '!$&') + '%' : '';

    const [rows] = nodeId
      ? await dbPool.query(
          'SELECT g.name AS team, p.content AS html '
          + 'FROM pages p '
          + 'JOIN `groups` g ON p.groupId = g.id '
          + 'WHERE p.isActive = true AND (p.nodeId = ? OR p.nodeId = ? OR p.nodeId LIKE ? ESCAPE "!") '
          + 'ORDER BY g.name',
          [nodeId, nodeIdBase, likePattern]
        )
      : await dbPool.query(
          'SELECT g.name AS team, p.content AS html '
          + 'FROM pages p '
          + 'JOIN `groups` g ON p.groupId = g.id '
          + 'WHERE p.isActive = true '
          + 'ORDER BY g.name'
        );
    const pages = rows.map(r => ({ team: r.team, html: r.html || '' }));
    res.json({ status: 'success', page_count: pages.length, pages });
  } catch (error) {
    res.status(500).json({ status: 'error', message: error.message });
  }
});

// Ensure default pages exist for nodes/groups
app.post('/api/pages/ensure-defaults', async (req, res) => {
  try {
    if (!dbPool) return res.status(503).json({ error: 'Database not available' });
    const { nodeId } = req.body || {};

    let created = 0;
    if (nodeId) {
      created += await ensureDefaultPagesForNode(nodeId);
    } else {
      const [nodes] = await dbPool.query('SELECT nodeId FROM nodes');
      for (const node of nodes) {
        created += await ensureDefaultPagesForNode(node.nodeId);
      }
    }

    res.json({ status: 'ok', created });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});
'''
if old_sync not in text:
    raise SystemExit('Failed to find sync pages block')
text = text.replace(old_sync, new_sync)
backend_path.write_text(text)

web_text = webserver_path.read_text()
old_proxy = '''// Delete page
app.delete('/api/pages/:pageId', async (req, res) => {
  try {
    const response = await axios.delete(`${API_URL}/api/pages/${req.params.pageId}`);
    res.json(response.data || { success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});
'''
new_proxy = old_proxy + '''
// Ensure default pages exist
app.post('/api/pages/ensure-defaults', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/pages/ensure-defaults`, req.body || {});
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});
'''
if old_proxy not in web_text:
    raise SystemExit('Failed to find delete page proxy block')
web_text = web_text.replace(old_proxy, new_proxy)
webserver_path.write_text(web_text)

view_text = pages_view_path.read_text()
old_header = '''        <div class="page-header">
            <h2>Page Management</h2>
            <button class="btn btn-primary" onclick="openAddPageModal()">+ Add Page</button>
        </div>
'''
new_header = '''        <div class="page-header">
            <h2>Page Management</h2>
            <div class="d-flex gap-2">
                <button class="btn btn-outline-secondary" onclick="ensureDefaultPages()">🔄 Add Missing Pages</button>
                <button class="btn btn-primary" onclick="openAddPageModal()">+ Add Page</button>
            </div>
        </div>
'''
if old_header not in view_text:
    raise SystemExit('Failed to find page header block')
view_text = view_text.replace(old_header, new_header)

insert_after = '''        function openAddPageModal() {
            currentPageId = null;
            document.getElementById('pageForm').reset();
            document.querySelector('#pageModal .modal-title').textContent = 'Add Page';
            document.querySelector('#pageModal .btn-primary').textContent = 'Create Page';
            pageModal.show();
        }
'''
if insert_after not in view_text:
    raise SystemExit('Failed to find openAddPageModal block')
addon = '''
        async function ensureDefaultPages() {
            try {
                const response = await axios.post('/api/pages/ensure-defaults');
                const created = response?.data?.created ?? 0;
                alert(`Missing pages created: ${created}`);
                window.location.reload();
            } catch (error) {
                console.error('Failed to ensure default pages:', error);
                alert('Failed to add missing pages.');
            }
        }
'''
view_text = view_text.replace(insert_after, insert_after + addon)
pages_view_path.write_text(view_text)

print('patched backend, webserver, and pages view')
