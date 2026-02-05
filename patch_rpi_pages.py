from pathlib import Path

backend_path = Path('/home/copilot/meshnet/rpi/backend/src/server.js')
webserver_path = Path('/home/copilot/meshnet/rpi/webserver/server.js')
pages_view_path = Path('/home/copilot/meshnet/rpi/webserver/views/pages.ejs')

# ---- Patch backend server.js ----
text = backend_path.read_text()
old = "function extractVersionFromNodeId(nodeId) {\n  if (!nodeId) return null;\n  const match = nodeId.match(/_(\\d+\\.\\d+\\.\\d+)$/);\n  return match ? match[1] : null;\n}\n"
new = old + "\nfunction stripVersionFromNodeId(nodeId) {\n  if (!nodeId) return null;\n  return nodeId.replace(/_(\\d+\\.\\d+\\.\\d+)$/, '');\n}\n"
if old not in text:
    raise SystemExit('Failed to find extractVersionFromNodeId block')
text = text.replace(old, new)

marker = "}\n\n// ============ API ROUTES ============\n"
insert = "}\n\nfunction buildDefaultPageContent(groupName, nodeIdBase) {\n  const safeGroup = groupName || 'Team';\n  const safeNode = nodeIdBase || 'Unknown Node';\n  return `${safeGroup} page for ${safeNode}`;\n}\n\nasync function ensureDefaultPagesForNode(nodeId) {\n  if (!dbPool || !nodeId) return 0;\n\n  const nodeIdBase = stripVersionFromNodeId(nodeId) || nodeId;\n  const macAddress = extractMacFromNodeId(nodeId) || nodeId.substring(0, 17);\n\n  await dbPool.query(\n    'INSERT INTO nodes (nodeId, macAddress, functionalName, isActive, lastSeen) VALUES (?, ?, ?, true, NOW()) ON DUPLICATE KEY UPDATE lastSeen = NOW()',\n    [nodeId, macAddress, nodeId]\n  );\n\n  const [groups] = await dbPool.query('SELECT id, name FROM `groups`');\n  if (!groups.length) return 0;\n\n  const [existingPages] = await dbPool.query(\n    'SELECT groupId FROM pages WHERE nodeId = ? AND groupId IS NOT NULL',\n    [nodeId]\n  );\n  const existingGroupIds = new Set(existingPages.map(row => row.groupId));\n\n  let created = 0;\n  for (const group of groups) {\n    if (existingGroupIds.has(group.id)) continue;\n    const pageId = uuidv4();\n    const title = `${group.name} Page`;\n    const content = buildDefaultPageContent(group.name, nodeIdBase);\n    await dbPool.query(\n      'INSERT INTO pages (pageId, nodeId, groupId, title, content, refreshInterval, isActive) VALUES (?, ?, ?, ?, ?, ?, true)',\n      [pageId, nodeId, group.id, title, content, 30]\n    );\n    created += 1;\n  }\n\n  return created;\n}\n\n// ============ API ROUTES ============\n"
if marker not in text:
    raise SystemExit('Failed to find API ROUTES marker')
text = text.replace(marker, insert)

old_sync = "// Sync pages for nodes\napp.get('/api/sync/pages', async (req, res) => {\n  try {\n    if (!dbPool) return res.json({ status: 'success', page_count: 0, pages: [] });\n    const [rows] = await dbPool.query(\n      'SELECT g.name AS team, p.content AS html '\n      + 'FROM pages p '\n      + 'JOIN `groups` g ON p.groupId = g.id '\n      + 'WHERE p.isActive = true '\n      + 'ORDER BY g.name'\n    );\n    const pages = rows.map(r => ({ team: r.team, html: r.html || '' }));\n    res.json({ status: 'success', page_count: pages.length, pages });\n  } catch (error) {\n    res.status(500).json({ status: 'error', message: error.message });\n  }\n});\n"
new_sync = "// Sync pages for nodes\napp.get('/api/sync/pages', async (req, res) => {\n  try {\n    if (!dbPool) return res.json({ status: 'success', page_count: 0, pages: [] });\n    const nodeId = req.query.nodeId || '';\n    if (nodeId) {\n      await ensureDefaultPagesForNode(nodeId);\n    }\n\n    const nodeIdBase = nodeId ? (stripVersionFromNodeId(nodeId) || nodeId) : '';\n    const likePattern = nodeIdBase ? nodeIdBase.replace(/[!%_]/g, '!$&') + '%' : '';\n\n    const [rows] = nodeId\n      ? await dbPool.query(\n          'SELECT g.name AS team, p.content AS html '\n          + 'FROM pages p '\n          + 'JOIN `groups` g ON p.groupId = g.id '\n          + 'WHERE p.isActive = true AND (p.nodeId = ? OR p.nodeId = ? OR p.nodeId LIKE ? ESCAPE "!") '\n          + 'ORDER BY g.name',\n          [nodeId, nodeIdBase, likePattern]\n        )\n      : await dbPool.query(\n          'SELECT g.name AS team, p.content AS html '\n          + 'FROM pages p '\n          + 'JOIN `groups` g ON p.groupId = g.id '\n          + 'WHERE p.isActive = true '\n          + 'ORDER BY g.name'\n        );\n    const pages = rows.map(r => ({ team: r.team, html: r.html || '' }));\n    res.json({ status: 'success', page_count: pages.length, pages });\n  } catch (error) {\n    res.status(500).json({ status: 'error', message: error.message });\n  }\n});\n\n// Ensure default pages exist for nodes/groups\napp.post('/api/pages/ensure-defaults', async (req, res) => {\n  try {\n    if (!dbPool) return res.status(503).json({ error: 'Database not available' });\n    const { nodeId } = req.body || {};\n\n    let created = 0;\n    if (nodeId) {\n      created += await ensureDefaultPagesForNode(nodeId);\n    } else {\n      const [nodes] = await dbPool.query('SELECT nodeId FROM nodes');\n      for (const node of nodes) {\n        created += await ensureDefaultPagesForNode(node.nodeId);\n      }\n    }\n\n    res.json({ status: 'ok', created });\n  } catch (error) {\n    res.status(500).json({ error: error.message });\n  }\n});\n"
if old_sync not in text:
    raise SystemExit('Failed to find sync pages block')
text = text.replace(old_sync, new_sync)
backend_path.write_text(text)

# ---- Patch webserver server.js ----
web_text = webserver_path.read_text()
old_proxy = "// Delete page\napp.delete('/api/pages/:pageId', async (req, res) => {\n  try {\n    const response = await axios.delete(`${API_URL}/api/pages/${req.params.pageId}`);\n    res.json(response.data || { success: true });\n  } catch (error) {\n    res.status(500).json({ error: error.message });\n  }\n});\n"
new_proxy = old_proxy + "\n// Ensure default pages exist\napp.post('/api/pages/ensure-defaults', async (req, res) => {\n  try {\n    const response = await axios.post(`${API_URL}/api/pages/ensure-defaults`, req.body || {});\n    res.json(response.data);\n  } catch (error) {\n    res.status(500).json({ error: error.message });\n  }\n});\n"
if old_proxy not in web_text:
    raise SystemExit('Failed to find delete page proxy block')
web_text = web_text.replace(old_proxy, new_proxy)
webserver_path.write_text(web_text)

# ---- Patch pages.ejs ----
view_text = pages_view_path.read_text()
old_header = "        <div class=\"page-header\">\n            <h2>Page Management</h2>\n            <button class=\"btn btn-primary\" onclick=\"openAddPageModal()\">+ Add Page</button>\n        </div>\n"
new_header = "        <div class=\"page-header\">\n            <h2>Page Management</h2>\n            <div class=\"d-flex gap-2\">\n                <button class=\"btn btn-outline-secondary\" onclick=\"ensureDefaultPages()\">🔄 Add Missing Pages</button>\n                <button class=\"btn btn-primary\" onclick=\"openAddPageModal()\">+ Add Page</button>\n            </div>\n        </div>\n"
if old_header not in view_text:
    raise SystemExit('Failed to find page header block')
view_text = view_text.replace(old_header, new_header)

insert_after = "        function openAddPageModal() {\n            currentPageId = null;\n            document.getElementById('pageForm').reset();\n            document.querySelector('#pageModal .modal-title').textContent = 'Add Page';\n            document.querySelector('#pageModal .btn-primary').textContent = 'Create Page';\n            pageModal.show();\n        }\n"
if insert_after not in view_text:
    raise SystemExit('Failed to find openAddPageModal block')
addon = "\n        async function ensureDefaultPages() {\n            try {\n                const response = await axios.post('/api/pages/ensure-defaults');\n                const created = response?.data?.created ?? 0;\n                alert(`Missing pages created: ${created}`);\n                window.location.reload();\n            } catch (error) {\n                console.error('Failed to ensure default pages:', error);\n                alert('Failed to add missing pages.');\n            }\n        }\n"
view_text = view_text.replace(insert_after, insert_after + addon)
pages_view_path.write_text(view_text)

print('patched backend, webserver, and pages view')
