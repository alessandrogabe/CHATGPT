# AudioMesh

AudioMesh is an experimental open-source acoustic mesh network that communicates through microphones and loudspeakers without Internet, Wi-Fi, Bluetooth, SIM cards or a central server.

## Live demo

`https://alessandrogabe.github.io/CHATGPT/`

## AudioMesh 6.1.1

- encrypted and authenticated AES-GCM frames
- PBKDF2 master-key derivation and hourly HKDF session-key rotation
- uniform fixed-length 64-character acoustic frames
- network name, packet type, identities and payload never transmitted in clear text
- replay protection
- up to 10 local node addresses
- coordinator election and shared time slots
- acoustic carrier sensing before every transmission
- random backoff after carrier release
- RESERVE/RELEASE leases for long transmissions
- presence confirmation
- per-fragment message acknowledgements
- persistent delayed-delivery queue
- real-time network and device dashboard
- integrated implementation guide
- corrected 16-bit message IDs, reservation deduplication and broadcast completion

The implementation currently uses ggwave as the modem layer. The mesh, medium-access, security and application layers are independent concepts and can be ported to ESP32, Raspberry Pi, native mobile applications or other acoustic modems.

## Experimental status

AudioMesh 6.1.1 is a functional research prototype. Its JavaScript syntax and encrypted frame construction were tested, but it has not undergone an independent security audit or broad hardware interoperability testing. Do not yet use it for safety-critical or highly confidential communication.

## Documentation

See [`PROTOCOL.md`](PROTOCOL.md).

## License

MIT — see [`LICENSE`](LICENSE).

Copyright (c) 2026 Alessandro Gabellotto.
