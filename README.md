# AudioChat v10

AudioChat e una mesh acustica sperimentale che usa microfono e altoparlante
per scambiare messaggi e file localmente, senza Internet, Wi-Fi, Bluetooth,
SIM o server centrale. Il modem e [ggwave](https://github.com/ggerganov/ggwave);
il protocollo di rete, arbitraggio e trasferimento e implementato da AudioChat.

## Versione corrente

Questo repository e allineato alla build **AudioChat v10 fast file transfer**
pubblicata su:

- https://audiochat-mesh.alegabe81.chatgpt.site/
- https://alessandrogabe.github.io/CHATGPT/

La pagina GitHub Pages usa gli stessi asset statici generati dalla sorgente v10.

## Caratteristiche

- discovery event-driven con elezione deterministica del coordinatore;
- JOIN/WELCOME/SYNC e roster multi-device con canale normalmente silenzioso;
- frame base da 16 byte e frame esteso da 32 byte;
- profili acustici Compatibile, Banda bassa, Veloce e Turbo con fallback;
- coda FIFO persistente in IndexedDB per i messaggi non ancora consegnati;
- rete e password trasformate tramite PBKDF2-HMAC-SHA-256, 150.000 iterazioni;
- file fino a 512 KiB protetti end-to-end con AES-256-GCM;
- compressione GZIP prima della cifratura quando riduce realmente i byte;
- trasferimento file `window-v1` con burst adattivi da 8 a 12 blocchi;
- ACK bitmap cumulativi e ritrasmissione dei soli blocchi mancanti;
- fallback al precedente ACK singolo quando la finestra non viene negoziata;
- prenotazione esclusiva del canale durante il file transfer;
- ggwave/WASM incluso negli asset statici, senza CDN a runtime.

I messaggi testuali v10 non sono cifrati; i file sono invece cifrati e
autenticati integralmente prima della trasmissione.

## Trasferimento file v10-fast

Il file viene prima compresso con GZIP solo se la compressione fa risparmiare
almeno l'overhead previsto, quindi cifrato con AES-GCM e suddiviso in blocchi
FILE_DATA. Il mittente invia una finestra iniziale di 8 blocchi senza attendere
un ACK tra un blocco e il successivo. Il ricevente risponde alla richiesta di
riscontro con un bitmap a 16 bit. Una finestra pulita cresce 8 -> 10 -> 12;
in caso di perdita torna a 8 e vengono ritrasmessi solo i blocchi mancanti.

Se il collegamento resta instabile, AudioChat rinegozia un profilo acustico piu
robusto e riprende dal primo blocco mancante. Il vecchio percorso stop-and-wait
resta disponibile come fallback di compatibilita.

## Protocollo

La specifica wire v10 completa e in [`PROTOCOL.md`](PROTOCOL.md). L'header di
riferimento per implementazioni native/ESP32 e in
[`protocol/audiochat_protocol.h`](protocol/audiochat_protocol.h).

## Stato sperimentale

AudioChat e un prototipo sperimentale e non ha subito un audit di sicurezza
indipendente ne una validazione estesa su hardware eterogeneo. Non va ancora
usato per comunicazioni safety-critical o ad alta confidenzialita.

## Licenza

L'uso personale e non commerciale e gratuito secondo [`LICENSE`](LICENSE).
Qualsiasi uso aziendale, professionale o commerciale richiede una licenza
commerciale separata a pagamento; prezzo, condizioni e supporto vengono
concordati caso per caso con il titolare del copyright.

Copyright (c) 2026 Alessandro Gabellotto.
