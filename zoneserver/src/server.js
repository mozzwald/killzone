/**
 * KillZone Server
 * 
 * Central authority for world state, entity management, and game logic.
 * REST API server managing shared game state across all connected clients.
 */

const express = require('express');
const cors = require('cors');
const World = require('./world');
const Mob = require('./mob');
const createApiRoutes = require('./routes/api');
const TcpServer = require('./tcp_server');

const PORT = parseInt(process.env.PORT || '3000', 10);
const TCP_PORT = parseInt(process.env.TCP_PORT || '6809', 10);

// Initialize world
const world = new World(40, 20);

// Spawn initial mobs for testing multi-player rendering
function spawnMobs() {
  world.respawnMobs(3);
  console.log(`🎮 Spawned initial mobs`);
}

// Create Express app
const app = express();

// Middleware
app.use(cors());
app.use(express.json());

// Enhanced request logging
app.use((req, res, next) => {
  const timestamp = new Date().toISOString();
  const method = req.method.padEnd(6);
  const path = req.path;

  // Get client IP (handle proxies and direct connections)
  const clientIp = req.headers['x-forwarded-for'] || req.connection.remoteAddress || 'unknown';
  const ipOnly = clientIp.split(',')[0].trim();  // Get first IP if multiple

  // Condensed logging for /state requests (they're frequent)
  const isStateRequest = path === '/world/state' || path.includes('/world/state');

  // Log request details
  let logMsg = `[${timestamp}] ${method} ${path}`;

  // Add body info for POST/PUT requests
  if ((req.method === 'POST' || req.method === 'PUT') && req.body && Object.keys(req.body).length > 0) {
    logMsg += ` | Body: ${JSON.stringify(req.body)}`;
  }

  // For state requests, use condensed format with client IP
  if (isStateRequest) {
    logMsg += ` [state] from ${ipOnly}`;
  }

  console.log(logMsg);

  // Capture response status
  const originalJson = res.json;
  res.json = function (data) {
    const statusCode = res.statusCode;
    const statusColor = statusCode >= 400 ? '❌' : '✅';

    // Condensed response logging for state requests
    if (isStateRequest) {
      console.log(`  ${statusColor} [${statusCode}]`);
    } else {
      console.log(`  ${statusColor} Response [${statusCode}]: ${JSON.stringify(data).substring(0, 100)}${JSON.stringify(data).length > 100 ? '...' : ''}`);
    }
    return originalJson.call(this, data);
  };

  next();
});

// API routes
app.use('/api', createApiRoutes(world));

// Error handling middleware
app.use((err, req, res, next) => {
  console.error('Error:', err);
  res.status(500).json({
    success: false,
    error: 'Internal server error'
  });
});

// 404 handler
app.use((req, res) => {
  res.status(404).json({
    success: false,
    error: 'Endpoint not found'
  });
});

// Start server only if not in test environment
let server;
if (process.env.NODE_ENV !== 'test') {
  // Start TCP Server
  const tcpServer = new TcpServer(world, TCP_PORT);
  tcpServer.start();

  server = app.listen(PORT, () => {
    console.log(`KillZone Server running on http://localhost:${PORT}`);
    console.log(`World dimensions: 40x20`);
    console.log(`API health check: GET http://localhost:${PORT}/api/health`);

    // Spawn mobs for testing
    spawnMobs();

    // Server maintenance loop
    setInterval(() => {
      // Every 100 ticks: respawn mobs if needed
      if (world.ticks % 100 === 0) {
        const spawnedMobs = world.respawnMobs(3);
        if (spawnedMobs.length > 0) {
          const hunterInfo = spawnedMobs.some(m => m.isHunter) ? ' (including Hunter)' : '';
          console.log(`  🎮 Respawned ${spawnedMobs.length} mobs${hunterInfo}`);
        }
      }

      // Every 30 seconds: clean up inactive players (2 min timeout)
      if (world.ticks % 600 === 0) {
        const inactivePlayers = world.cleanupInactivePlayers(120000);
        if (inactivePlayers.length > 0) {
          console.log(`  🧹 Cleaned up ${inactivePlayers.length} inactive player(s): ${inactivePlayers.map(p => p.name).join(', ')}`);
        }
      }
    }, 100);  // Check every 100ms
  });

  // Graceful shutdown
  process.on('SIGTERM', () => {
    console.log('SIGTERM received, shutting down gracefully...');
    server.close(() => {
      console.log('Server closed');
      process.exit(0);
    });
  });
}

module.exports = { app, world };
