import { Request, Response, NextFunction } from 'express';
import jwt from 'jsonwebtoken';
import { ensureProfile } from '../services/device.js';

// Supabase config
const SUPABASE_URL = process.env.SUPABASE_URL || process.env.VITE_SUPABASE_URL || '';
const SUPABASE_JWT_SECRET = process.env.SUPABASE_JWT_SECRET || '';

// Cache for JWKS
let jwksCache: { keys: JsonWebKey[]; fetchedAt: number } | null = null;
const JWKS_CACHE_TTL = 3600000; // 1 hour

// Extend Express Request type
declare global {
  namespace Express {
    interface Request {
      user?: {
        id: string;
        email: string;
      };
    }
  }
}

interface SupabaseJWTPayload {
  sub: string;           // User ID
  email?: string;
  user_metadata?: {
    full_name?: string;
    name?: string;
    avatar_url?: string;
  };
  iat: number;
  exp: number;
}

interface JsonWebKey {
  kty: string;
  kid: string;
  n?: string;
  e?: string;
  x?: string;
  y?: string;
  crv?: string;
}

/**
 * Fetch JWKS from Supabase
 */
async function getJwks(): Promise<JsonWebKey[]> {
  if (jwksCache && Date.now() - jwksCache.fetchedAt < JWKS_CACHE_TTL) {
    return jwksCache.keys;
  }

  const response = await fetch(`${SUPABASE_URL}/.well-known/jwks.json`);
  if (!response.ok) {
    throw new Error('Failed to fetch JWKS');
  }

  const data = await response.json() as { keys: JsonWebKey[] };
  jwksCache = { keys: data.keys, fetchedAt: Date.now() };
  return data.keys;
}

/**
 * Convert JWK to PEM format for RS256
 */
function jwkToPem(jwk: JsonWebKey): string {
  // For RS256 keys
  if (jwk.kty === 'RSA' && jwk.n && jwk.e) {
    const n = Buffer.from(jwk.n, 'base64url');
    const e = Buffer.from(jwk.e, 'base64url');
    
    // Build DER encoding
    const modulus = Buffer.concat([Buffer.from([0x02]), encodeLength(n.length + 1), Buffer.from([0x00]), n]);
    const exponent = Buffer.concat([Buffer.from([0x02]), encodeLength(e.length), e]);
    const sequence = Buffer.concat([Buffer.from([0x30]), encodeLength(modulus.length + exponent.length), modulus, exponent]);
    const bitString = Buffer.concat([Buffer.from([0x03]), encodeLength(sequence.length + 1), Buffer.from([0x00]), sequence]);
    const algorithmId = Buffer.from('300d06092a864886f70d0101010500', 'hex');
    const publicKey = Buffer.concat([Buffer.from([0x30]), encodeLength(algorithmId.length + bitString.length), algorithmId, bitString]);
    
    const base64 = publicKey.toString('base64');
    const lines = base64.match(/.{1,64}/g) || [];
    return `-----BEGIN PUBLIC KEY-----\n${lines.join('\n')}\n-----END PUBLIC KEY-----`;
  }
  
  throw new Error(`Unsupported key type: ${jwk.kty}`);
}

function encodeLength(len: number): Buffer {
  if (len < 128) return Buffer.from([len]);
  if (len < 256) return Buffer.from([0x81, len]);
  return Buffer.from([0x82, (len >> 8) & 0xff, len & 0xff]);
}

/**
 * Verify JWT using JWKS or legacy secret
 */
async function verifyToken(token: string): Promise<SupabaseJWTPayload> {
  // Try legacy secret first (simpler)
  if (SUPABASE_JWT_SECRET) {
    try {
      return jwt.verify(token, SUPABASE_JWT_SECRET) as SupabaseJWTPayload;
    } catch {
      // Fall through to JWKS
    }
  }

  // Use JWKS
  if (!SUPABASE_URL) {
    throw new Error('Neither SUPABASE_JWT_SECRET nor SUPABASE_URL configured');
  }

  const decoded = jwt.decode(token, { complete: true });
  if (!decoded || !decoded.header.kid) {
    throw new Error('Invalid token format');
  }

  const keys = await getJwks();
  const key = keys.find(k => k.kid === decoded.header.kid);
  if (!key) {
    throw new Error('Key not found in JWKS');
  }

  const pem = jwkToPem(key);
  return jwt.verify(token, pem, { algorithms: ['RS256'] }) as SupabaseJWTPayload;
}

/**
 * Middleware to verify Supabase JWT token
 */
export function supabaseAuthMiddleware(
  req: Request,
  res: Response,
  next: NextFunction
): void {
  const authHeader = req.headers.authorization;

  if (!authHeader?.startsWith('Bearer ')) {
    res.status(401).json({ error: 'Missing authorization header' });
    return;
  }

  const token = authHeader.slice(7);

  if (!SUPABASE_JWT_SECRET && !SUPABASE_URL) {
    console.error('[Auth] Neither SUPABASE_JWT_SECRET nor SUPABASE_URL configured');
    res.status(500).json({ error: 'Auth not configured' });
    return;
  }

  verifyToken(token)
    .then(payload => {
      req.user = {
        id: payload.sub,
        email: payload.email || '',
      };

      ensureProfile(
        payload.sub,
        payload.email,
        payload.user_metadata?.full_name || payload.user_metadata?.name,
        payload.user_metadata?.avatar_url
      );

      next();
    })
    .catch(error => {
      console.error('[Auth] Token verification failed:', error);
      res.status(401).json({ error: 'Invalid or expired token' });
    });
}

/**
 * Optional auth - doesn't require auth but attaches user if present
 */
export function optionalAuthMiddleware(
  req: Request,
  _res: Response,
  next: NextFunction
): void {
  const authHeader = req.headers.authorization;

  if (authHeader?.startsWith('Bearer ')) {
    const token = authHeader.slice(7);

    verifyToken(token)
      .then(payload => {
        req.user = {
          id: payload.sub,
          email: payload.email || '',
        };
      })
      .catch(() => {
        // Ignore errors - just continue without user
      })
      .finally(() => next());
    return;
  }

  next();
}
