CREATE TABLE IF NOT EXISTS ip_address (
  ip         TEXT PRIMARY KEY,
  ip_type    TEXT NOT NULL CHECK (ip_type IN ('IPv4','IPv6')),
  status     TEXT NOT NULL CHECK (status IN ('FREE','RESERVED','ASSIGNED')),
  service_id TEXT NULL,
  expires_at TIMESTAMPTZ NULL
);