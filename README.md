# AudioMesh

AudioMesh is an experimental open-source acoustic mesh network that communicates through microphones and loudspeakers without Internet, Wi-Fi, Bluetooth, SIM cards or a central server.

## Live demo

`https://alessandrogabe.github.io/CHATGPT/`

## AudioMesh 6.1

- encrypted and authenticated AES-GCM frames
- PBKDF2 master-key derivation and hourly HKDF session-key rotation
- uniform fixed-length 64-character acoustic frames
- network name and packet type never transmitted in clear text
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

The implementation currently uses ggwave as the modem layer. The mesh, medium-access, security and application layers are independent concepts and can be ported to ESP32, Raspberry Pi, native mobile applications or other acoustic modems.

## Documentation

See [`PROTOCOL.md`](PROTOCOL.md).

## License

MIT — see [`LICENSE`](LICENSE).

Copyright (c) 2026 Alessandro Gabellotto.
