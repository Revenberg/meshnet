# MeshNet SSO (Single Sign-On) Integration

## Overview

SSO enables users to authenticate using their existing accounts from major providers:
- **Google** - Most common, supports Android/iOS
- **GitHub** - Developer-friendly
- **Azure AD** - Enterprise integration
- **Local** - Fallback for local-only deployments

## Architecture

```
┌─────────────────────────────────────┐
│   Client (Browser/Mobile)           │
└──────────────┬──────────────────────┘
               │
        ┌──────▼──────┐
        │  Redirects  │ (optional)
        │ to provider │
        │  login      │
        └──────┬──────┘
               │
    ┌──────────▼─────────────┐
    │ OAuth Provider         │
    │ (Google/GitHub/Azure)  │
    └──────────┬─────────────┘
               │
    ┌──────────▼──────────────┐
    │ Returns ID/Access Token │
    └──────────┬──────────────┘
               │
    ┌──────────▼────────────────────┐
    │ POST /auth/sso/callback/*      │
    │ (with token)                   │
    └──────────┬────────────────────┘
               │
    ┌──────────▼────────────────────┐
    │ MeshNet Backend                │
    │ 1. Verify token signature      │
    │ 2. Extract user info           │
    │ 3. Create/update user in DB    │
    │ 4. Generate JWT                │
    └──────────┬────────────────────┘
               │
    ┌──────────▼────────────────────┐
    │ Return JWT to Client           │
    │ Client stores in localStorage  │
    │ Sends with all future requests │
    └────────────────────────────────┘
```

## Setup Instructions

### 1. Database Setup

Run migration to create SSO tables:

```bash
# Via docker
docker exec meshnet-mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet < sso-schema.sql

# Or via local mysql
mysql -u meshnet -p meshnet < sso-schema.sql
```

Tables created:
- `sso_users` - SSO user accounts
- `user_device_associations` - Link users to devices
- `sso_sessions` - Active sessions (optional)
- `auth_audit_log` - Authentication audit trail

### 2. Backend Integration

Add SSO router to `server.js`:

```javascript
const ssoRouter = require('./modules/auth/router');

// Add to server initialization
app.use('/api/auth', ssoRouter);
```

Update `package.json` to include JWT support:

```json
{
  "dependencies": {
    "jsonwebtoken": "^9.0.0",
    "cors": "^2.8.5"
  }
}
```

### 3. Google OAuth Setup

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create new project: "MeshNet"
3. Enable APIs:
   - Google+ API
   - Google Identity API
4. Create OAuth 2.0 credentials:
   - Type: **Web application**
   - Authorized JavaScript origins:
     - `http://localhost:3000`
     - `http://meshnet.local`
     - `http://your-domain.com`
   - Authorized redirect URIs:
     - `http://localhost:3000/auth/google`
     - `http://meshnet.local/auth/google`
     - `http://your-domain.com/auth/google`
5. Save Client ID and Client Secret

Add to `.env`:
```
GOOGLE_CLIENT_ID=your-client-id.apps.googleusercontent.com
GOOGLE_CLIENT_SECRET=your-client-secret
```

### 4. GitHub OAuth Setup

1. Go to GitHub Settings → Developer settings → OAuth Apps
2. Click "New OAuth App"
3. Fill in:
   - **Application name**: MeshNet
   - **Homepage URL**: `http://meshnet.local`
   - **Authorization callback URL**: `http://meshnet.local/auth/github`
4. Save Client ID and Client Secret

Add to `.env`:
```
GITHUB_CLIENT_ID=your-client-id
GITHUB_CLIENT_SECRET=your-client-secret
```

### 5. Azure AD Setup

1. Go to [Azure Portal](https://portal.azure.com/)
2. Azure Active Directory → App registrations → New registration
3. Configure:
   - **Name**: MeshNet
   - **Redirect URI**: `http://meshnet.local/auth/azure`
4. Certificates & secrets → Create client secret
5. API permissions → Add Microsoft Graph API permissions:
   - User.Read
   - User.Read.All

Add to `.env`:
```
AZURE_TENANT_ID=your-tenant-id
AZURE_CLIENT_ID=your-client-id
AZURE_CLIENT_SECRET=your-client-secret
```

### 6. Frontend Integration (Vue.js Example)

```javascript
// auth.js - Composable for SSO
import { ref } from 'vue'

export const useAuth = () => {
  const user = ref(null)
  const token = ref(localStorage.getItem('meshnet_token'))

  const loginWithGoogle = async (idToken) => {
    const response = await fetch('/api/auth/sso/callback/google', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ idToken })
    })
    const data = await response.json()
    token.value = data.token
    user.value = data.user
    localStorage.setItem('meshnet_token', data.token)
  }

  const loginWithGitHub = async (accessToken) => {
    const response = await fetch('/api/auth/sso/callback/github', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ accessToken })
    })
    const data = await response.json()
    token.value = data.token
    user.value = data.user
    localStorage.setItem('meshnet_token', data.token)
  }

  const logout = () => {
    token.value = null
    user.value = null
    localStorage.removeItem('meshnet_token')
  }

  const getAuthHeader = () => ({
    'Authorization': `Bearer ${token.value}`
  })

  return {
    user,
    token,
    loginWithGoogle,
    loginWithGitHub,
    logout,
    getAuthHeader
  }
}
```

Usage in Vue component:
```vue
<template>
  <div v-if="user">
    <p>Welcome, {{ user.username }}!</p>
    <img :src="user.avatarUrl" :alt="user.username">
    <button @click="logout">Logout</button>
  </div>
  <div v-else>
    <GoogleLoginButton @success="handleGoogleLogin" />
    <GitHubLoginButton @success="handleGitHubLogin" />
  </div>
</template>

<script setup>
import { useAuth } from '@/composables/auth'

const { user, loginWithGoogle, loginWithGitHub, logout } = useAuth()

const handleGoogleLogin = async (credentialResponse) => {
  await loginWithGoogle(credentialResponse.credential)
}

const handleGitHubLogin = async (accessToken) => {
  await loginWithGitHub(accessToken)
}
</script>
```

## API Endpoints

### Authentication

| Method | Endpoint | Body | Returns |
|--------|----------|------|---------|
| POST | `/api/auth/sso/callback/google` | `{ idToken }` | `{ token, user }` |
| POST | `/api/auth/sso/callback/github` | `{ accessToken }` | `{ token, user }` |
| POST | `/api/auth/sso/callback/azure` | `{ idToken }` | `{ token, user }` |
| POST | `/api/auth/logout` | - | `{ status }` |

### Token Management

| Method | Endpoint | Header | Returns |
|--------|----------|--------|---------|
| GET | `/api/auth/verify` | `Authorization: Bearer <token>` | `{ status, user }` |
| POST | `/api/auth/refresh` | - | `{ token }` |

### User Profile

| Method | Endpoint | Header | Returns |
|--------|----------|--------|---------|
| GET | `/api/auth/profile` | `Authorization: Bearer <token>` | `{ user }` |

## Protected Endpoints Example

Use middleware to protect API routes:

```javascript
const { verifyToken, verifyRole } = require('./modules/auth/middleware')

// Require authentication
app.get('/api/nodes', verifyToken, async (req, res) => {
  // req.user contains { userId, username, role }
  console.log(`User ${req.user.username} accessing nodes`)
  // ...
})

// Require admin role
app.post('/api/admin/users', verifyRole('admin'), async (req, res) => {
  // Only admins can access
  // ...
})

// Optional authentication
app.get('/api/public', optionalAuth, async (req, res) => {
  if (req.user) {
    // User is authenticated
  } else {
    // Public access
  }
})
```

## Environment Variables

```bash
# JWT Configuration
JWT_SECRET=your-secret-key-change-in-production

# Google OAuth
GOOGLE_CLIENT_ID=xxx.apps.googleusercontent.com
GOOGLE_CLIENT_SECRET=xxx

# GitHub OAuth
GITHUB_CLIENT_ID=xxx
GITHUB_CLIENT_SECRET=xxx

# Azure AD
AZURE_TENANT_ID=xxx
AZURE_CLIENT_ID=xxx
AZURE_CLIENT_SECRET=xxx

# Database
DB_HOST=mysql
DB_PORT=3306
DB_USER=meshnet
DB_PASSWORD=meshnet_secure_pwd
DB_NAME=meshnet
```

## Troubleshooting

### "Token Signature Invalid"

Ensure JWT_SECRET is same on all instances:
```bash
export JWT_SECRET="your-consistent-secret"
```

### "User Not Found After Login"

1. Check sso_users table: `SELECT * FROM sso_users;`
2. Verify user provisioning ran successfully
3. Check auth logs for errors

### "CORS Errors"

Add to `server.js`:
```javascript
app.use(cors({
  origin: [
    'http://localhost:3000',
    'http://meshnet.local',
    'https://meshnet.local'
  ],
  credentials: true
}))
```

### Provider Token Validation Failed

In production, properly validate tokens:

```javascript
// For Google (use google-auth-library)
const { OAuth2Client } = require('google-auth-library')
const client = new OAuth2Client(process.env.GOOGLE_CLIENT_ID)
const ticket = await client.verifyIdToken({ idToken })

// For GitHub (already validated via access token)
const response = await fetch('https://api.github.com/user', {
  headers: { Authorization: `Bearer ${token}` }
})

// For Azure (use microsoft-identity-client)
const msal = require('@azure/msal-node')
// ...
```

## Security Considerations

1. **JWT Secret**: Use strong, random secret in production
2. **HTTPS Only**: Always use HTTPS for OAuth in production
3. **Token Expiry**: Default 7 days - adjust as needed
4. **Refresh Tokens**: Implement refresh token rotation
5. **Token Blacklist**: Maintain blacklist for revoked tokens
6. **Audit Logging**: Enable auth_audit_log for security tracking
7. **Rate Limiting**: Implement rate limits on auth endpoints

## Advanced: Custom Provider

Add custom provider (e.g., Keycloak):

```javascript
router.post('/sso/callback/keycloak', async (req, res) => {
  const { idToken } = req.body
  const decoded = jwt.decode(idToken)
  
  const user = await provisionUser('keycloak', {
    id: decoded.sub,
    email: decoded.email,
    name: decoded.name,
  })
  
  const token = generateToken(user.userId, user.username, user.role)
  res.json({ token, user })
})
```

## Related Documentation

- [JWT Best Practices](https://tools.ietf.org/html/rfc8949)
- [OAuth 2.0 Security](https://datatracker.ietf.org/doc/html/rfc6749)
- [OpenID Connect](https://openid.net/connect/)
- [OWASP Authentication](https://cheatsheetseries.owasp.org/cheatsheets/Authentication_Cheat_Sheet.html)

---

**Status**: ✅ SSO Integration Ready  
**Version**: V0.8.1  
**Date**: 2026-01-29  
**Supported Providers**: Google, GitHub, Azure AD  
**Token Type**: JWT (7-day expiry)
