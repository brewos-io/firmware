import { WebSocket } from 'ws';
import { v4 as uuidv4 } from 'uuid';
import { OAuth2Client } from 'google-auth-library';
// Google OAuth client for WebSocket auth
const GOOGLE_CLIENT_ID = process.env.GOOGLE_CLIENT_ID || '';
const googleClient = new OAuth2Client(GOOGLE_CLIENT_ID);
/**
 * Client Proxy
 *
 * Handles WebSocket connections from client apps (web, mobile).
 * Routes messages between clients and their associated devices.
 */
export class ClientProxy {
    clients = new Map();
    deviceClients = new Map(); // deviceId -> sessionIds
    deviceRelay;
    constructor(wss, deviceRelay) {
        this.deviceRelay = deviceRelay;
        wss.on('connection', (ws, req) => this.handleConnection(ws, req));
        // Subscribe to device messages
        deviceRelay.onDeviceMessage((deviceId, message) => {
            this.broadcastToDeviceClients(deviceId, message);
        });
    }
    async handleConnection(ws, req) {
        const url = new URL(req.url || '', `http://${req.headers.host}`);
        const token = url.searchParams.get('token');
        const deviceId = url.searchParams.get('device');
        if (!token || !deviceId) {
            ws.close(4001, 'Missing token or device ID');
            return;
        }
        // Verify Google ID token
        let userId;
        try {
            const ticket = await googleClient.verifyIdToken({
                idToken: token,
                audience: GOOGLE_CLIENT_ID,
            });
            const payload = ticket.getPayload();
            if (!payload?.sub) {
                ws.close(4002, 'Invalid token');
                return;
            }
            userId = payload.sub;
        }
        catch (error) {
            console.error('[Client] Token verification failed:', error);
            ws.close(4002, 'Invalid or expired token');
            return;
        }
        // TODO: Verify user has access to this device
        const sessionId = uuidv4();
        const connection = {
            ws,
            sessionId,
            userId,
            deviceId,
            connectedAt: new Date(),
        };
        this.registerClient(connection);
        console.log(`[Client] Connected: ${sessionId} -> device ${deviceId}`);
        // Handle messages from client
        ws.on('message', (data) => {
            try {
                const message = JSON.parse(data.toString());
                this.handleClientMessage(connection, message);
            }
            catch (err) {
                console.error(`[Client] Invalid message from ${sessionId}:`, err);
            }
        });
        // Handle disconnect
        ws.on('close', () => {
            this.unregisterClient(connection);
            console.log(`[Client] Disconnected: ${sessionId}`);
        });
        ws.on('error', (err) => {
            console.error(`[Client] Error from ${sessionId}:`, err);
            this.unregisterClient(connection);
        });
        // Send connection status
        const deviceOnline = this.deviceRelay.isDeviceConnected(deviceId);
        this.sendToClient(connection, {
            type: 'connected',
            sessionId,
            deviceId,
            deviceOnline,
            timestamp: Date.now(),
        });
    }
    registerClient(client) {
        this.clients.set(client.sessionId, client);
        // Track clients per device
        if (!this.deviceClients.has(client.deviceId)) {
            this.deviceClients.set(client.deviceId, new Set());
        }
        this.deviceClients.get(client.deviceId).add(client.sessionId);
    }
    unregisterClient(client) {
        this.clients.delete(client.sessionId);
        const deviceSessions = this.deviceClients.get(client.deviceId);
        if (deviceSessions) {
            deviceSessions.delete(client.sessionId);
            if (deviceSessions.size === 0) {
                this.deviceClients.delete(client.deviceId);
            }
        }
    }
    handleClientMessage(client, message) {
        // Forward message to device
        message.timestamp = Date.now();
        const sent = this.deviceRelay.sendToDevice(client.deviceId, message);
        if (!sent) {
            // Device is offline, notify client
            this.sendToClient(client, {
                type: 'error',
                error: 'device_offline',
                message: 'Device is not connected',
            });
        }
    }
    sendToClient(client, message) {
        if (client.ws.readyState === WebSocket.OPEN) {
            client.ws.send(JSON.stringify(message));
        }
    }
    broadcastToDeviceClients(deviceId, message) {
        const sessions = this.deviceClients.get(deviceId);
        if (!sessions)
            return;
        const payload = JSON.stringify(message);
        for (const sessionId of sessions) {
            const client = this.clients.get(sessionId);
            if (client?.ws.readyState === WebSocket.OPEN) {
                client.ws.send(payload);
            }
        }
    }
    /**
     * Get connected client count
     */
    getConnectedClientCount() {
        return this.clients.size;
    }
}
//# sourceMappingURL=client-proxy.js.map