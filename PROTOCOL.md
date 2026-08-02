# AudioMesh Protocol 6.1

AudioMesh is an experimental open acoustic mesh protocol for local communication through microphones and loudspeakers.

## Physical and modem layer

- ggwave 0.4.0
- one fixed-length decoder only
- 64 printable Base64URL characters per acoustic frame
- audible robust/fast profiles
- carrier detection before complete frame decoding

## Uniform encrypted frame

Every transmission has the same visible length.

- 48 raw bytes
- 12-byte public nonce
- 36-byte AES-GCM ciphertext and authentication tag
- 20-byte authenticated plaintext

Plaintext layout:

| Field | Bytes |
|---|---:|
| Version | 1 |
| Packet type | 1 |
| Key epoch | 1 |
| Source/destination (4 bits each) | 1 |
| Sequence | 2 |
| Fragment index | 1 |
| Fragment count | 1 |
| Payload | 12 |

The network name, device name, packet type, source, destination and application payload are not transmitted in clear text.

## Cryptography

- PBKDF2-SHA-256 master-key derivation
- 150,000 iterations
- salt derived from the normalized network name
- HKDF-SHA-256 session-key derivation
- hourly key epoch rotation
- AES-GCM-256 encryption and authentication
- additional authenticated data bound to the network name
- nonce replay cache
- network fingerprint derived from the master key

## Network access

- maximum operational roster: 10 nodes
- deterministic local addresses 0-9
- broadcast address 15
- coordinator: lowest stable node ID
- hybrid TDMA and acoustic CSMA/CA
- every transmission waits for a free carrier
- guard time and random backoff after carrier release
- long transfers use RESERVE and RELEASE leases
- nodes skip their normal turns while another node owns a valid lease

## Control and delivery

- encrypted HELLO and name fragments
- encrypted PRESENCE
- PRESENCE_ACK for bidirectional confirmation
- coordinator SYNC for common slot timing
- encrypted ROSTER reconciliation
- per-fragment DATA_ACK
- persistent sender-side outbox through IndexedDB
- retry and delayed delivery when the recipient returns

## Security scope

An observer without the network key can detect acoustic digital activity but cannot authenticate or interpret a frame. Traffic timing and the existence of acoustic activity remain physically observable.

## License

MIT License. See `LICENSE`.
