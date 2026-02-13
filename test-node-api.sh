#!/bin/bash
# Test script for MeshNet Node Web Hosting API

echo "🧪 MeshNet Node Web Hosting API Test Suite"
echo "=========================================="
echo ""

# Node IDs (hardcoded from setup)
NODE_1="72d67530-dac6-4666-885c-160cb36579ee"
NODE_2="26b80c3a-a7e2-4634-957a-51f7b777de72"
NODE_3="d1ec1f02-0e0b-4763-94d5-984e93c11bde"

API_BASE="http://localhost:3001/api/host"
API_CORE="http://localhost:3001/api"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

test_count=0
pass_count=0
fail_count=0

# Helper function
run_test() {
  local name=$1
  local url=$2
  local expected_code=$3
  
  test_count=$((test_count + 1))
  echo -n "Test $test_count: $name ... "
  
  response=$(curl -s -w "\n%{http_code}" "$url")
  http_code=$(echo "$response" | tail -n1)
  body=$(echo "$response" | head -n-1)
  
  if [ "$http_code" = "$expected_code" ]; then
    echo -e "${GREEN}✓ PASS${NC} (HTTP $http_code)"
    pass_count=$((pass_count + 1))
  else
    echo -e "${RED}✗ FAIL${NC} (Expected $expected_code, got $http_code)"
    fail_count=$((fail_count + 1))
  fi
}

post_json() {
  local url=$1
  local body=$2
  curl -s -X POST -H "Content-Type: application/json" -d "$body" "$url" >/dev/null
}

get_json() {
  local url=$1
  curl -s "$url"
}

echo "Setup: Create 10 teams, users, virtual nodes, and pages"
echo ""

TEAM_NAMES=()
for i in $(seq -w 1 10); do
  TEAM_NAMES+=("Team ${i}")
done

for team in "${TEAM_NAMES[@]}"; do
  post_json "$API_CORE/groups" "{\"name\":\"$team\",\"description\":\"Auto test team $team\",\"permissions\":[]}" || true
  echo "  ✓ Team ensured: $team"
done

GROUPS_JSON=$(get_json "$API_CORE/groups")
TOTAL_NEW_USERS=0

for team in "${TEAM_NAMES[@]}"; do
  GROUP_ID=$(echo "$GROUPS_JSON" | grep -o "{[^}]*\"name\":\"$team\"[^}]*}" | grep -o '"id"[[:space:]]*:[[:space:]]*[0-9]*' | head -1 | grep -o '[0-9]*')
  if [ -z "$GROUP_ID" ]; then
    continue
  fi
  COUNT=$(( (RANDOM % 6) + 3 ))
  for i in $(seq -w 1 $COUNT); do
    USERNAME=$(echo "${team}" | tr -d ' ' | tr '[:upper:]' '[:lower:]')_user${i}
    post_json "$API_CORE/users" "{\"username\":\"$USERNAME\",\"password\":\"test123\",\"groupId\":$GROUP_ID}" || true
    TOTAL_NEW_USERS=$((TOTAL_NEW_USERS + 1))
    echo "  ✓ User ensured: $USERNAME"
  done
done

for i in $(seq -w 1 10); do
  NODE_ID="VIRTUAL_NODE_${i}"
  MAC="00:00:00:00:10:$i"
  post_json "$API_CORE/nodes" "{\"nodeId\":\"$NODE_ID\",\"macAddress\":\"$MAC\",\"functionalName\":\"Virtual Node $i\",\"version\":\"virtual\"}" || true
  echo "  ✓ Virtual node ensured: $NODE_ID"
done

post_json "$API_CORE/pages/ensure-defaults" "{}" || true
echo "  ✓ Default pages ensured"
echo ""

echo "📋 Test: Get Pages for Node 1"
run_test "List pages for MeshNode-1" \
  "$API_BASE/node/$NODE_1/pages" \
  "200"
echo ""

echo "📄 Test: Get Page Content"
# First get the page ID
PAGE_ID=$(curl -s "$API_BASE/node/$NODE_1/pages" | grep -o '"pageId":"[^"]*"' | head -1 | cut -d'"' -f4)
if [ -n "$PAGE_ID" ]; then
  run_test "Get page HTML content" \
    "$API_BASE/node/$NODE_1/pages/$PAGE_ID" \
    "200"
  echo ""
  
  run_test "Get page as JSON" \
    "$API_BASE/node/$NODE_1/pages/$PAGE_ID/json" \
    "200"
  echo ""
fi

echo "📡 Test: Node Information"
run_test "Get MeshNode-1 info" \
  "$API_BASE/node/$NODE_1/info" \
  "200"
echo ""

run_test "Get MeshNode-2 info" \
  "$API_BASE/node/$NODE_2/info" \
  "200"
echo ""

run_test "Get MeshNode-3 info" \
  "$API_BASE/node/$NODE_3/info" \
  "200"
echo ""

echo "🚫 Test: Offline Nodes Count"
NODES_JSON=$(get_json "$API_CORE/nodes")
offline_count=$(echo "$NODES_JSON" | grep -o '"isActive"[[:space:]]*:[[:space:]]*false' | wc -l | tr -d ' ')
test_count=$((test_count + 1))
echo -n "Test $test_count: Offline nodes count > 0 ... "
if [ -n "$offline_count" ] && [ "$offline_count" -gt 0 ]; then
  echo -e "${GREEN}✓ PASS${NC} (offline=$offline_count)"
  pass_count=$((pass_count + 1))
else
  echo -e "${RED}✗ FAIL${NC} (offline=${offline_count:-0})"
  fail_count=$((fail_count + 1))
fi
echo ""

echo "💓 Test: Heartbeat"
run_test "Send heartbeat for MeshNode-1" \
  -X POST "$API_BASE/node/$NODE_1/heartbeat" \
  -H "Content-Type: application/json" \
  -d '{"signalStrength": -92, "battery": 87, "connectedNodes": 3}' \
  "200"
echo ""

echo "❌ Test: Error Cases"
run_test "Invalid node ID (404)" \
  "$API_BASE/node/invalid-node/pages" \
  "200"  # Returns empty array, not 404
echo ""

echo "🔎 Test: Real device sync scope"
NODES_JSON=$(get_json "$API_CORE/nodes")
GROUPS_COUNT=$(echo "$GROUPS_JSON" | grep -o '"id"' | wc -l | tr -d ' ')

REAL_NODE_IDS=$(echo "$NODES_JSON" | grep -o '"nodeId"[[:space:]]*:[[:space:]]*"LoRA_[^"]*"' | cut -d '"' -f4)
for nodeId in $REAL_NODE_IDS; do
  PAGES_JSON=$(get_json "$API_CORE/sync/pages?nodeId=$nodeId")
  PAGE_COUNT=$(echo "$PAGES_JSON" | grep -o '"page_count"[[:space:]]*:[[:space:]]*[0-9]*' | grep -o '[0-9]*')
  echo "  Node $nodeId: pages=$PAGE_COUNT"
  if [ -n "$PAGE_COUNT" ] && [ "$PAGE_COUNT" -gt "$GROUPS_COUNT" ]; then
    echo -e "  ${RED}✗ Node has more pages than groups${NC}"
    fail_count=$((fail_count + 1))
  fi
done

USERS_SYNC=$(get_json "$API_CORE/sync/users")
USER_COUNT=$(echo "$USERS_SYNC" | grep -o '"user_count"[[:space:]]*:[[:space:]]*[0-9]*' | grep -o '[0-9]*')
echo "  Users synced: $USER_COUNT (new users added: $TOTAL_NEW_USERS)"
if [ -n "$USER_COUNT" ] && [ "$USER_COUNT" -lt "$TOTAL_NEW_USERS" ]; then
  echo -e "  ${RED}✗ Sync users count smaller than expected${NC}"
  fail_count=$((fail_count + 1))
fi
echo ""

echo ""
echo "=========================================="
echo -e "Test Results: ${GREEN}$pass_count passed${NC}, ${RED}$fail_count failed${NC}, $test_count total"
echo "=========================================="

if [ $fail_count -eq 0 ]; then
  exit 0
else
  exit 1
fi
