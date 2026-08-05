# AudioChat Acoustic Mesh Protocol v10

AudioChat v10 usa **ggwave 0.4.0** come modem FSK/ECC e aggiunge un protocollo
binario half-duplex con arbitraggio deterministico. Il formato base è
implementabile su ESP32; browser, PC e Raspberry possono negoziare anche il
frame esteso. La v10 conserva la chiave di rete derivata da password e le
sessioni file esclusive della v9, ma rende la rete **event-driven**: senza
traffico tutti i trasmettitori restano muti e il prossimo JOIN ricostruisce la
stanza.

## Parametri ggwave obbligatori

| Parametro | Valore |
| --- | --- |
| `payloadLength` | `16` per il canale base; `32` opzionale per l'esteso |
| griglia delle frequenze | **46,875 Hz per bin** (`sampleRate / samplesPerFrame`) |
| Browser / PC | `sampleRate = 48000`, `samplesPerFrame = 1024` |
| ESP32 raccomandato | `sampleRate = 24000`, `samplesPerFrame = 512` |
| `sampleRateInp` / `sampleRateOut` | sample rate reale dell'I/O; ggwave ricampiona |
| `operatingMode` | RX + TX + DSS |
| Formato campioni browser | Float32 little-endian |

Non va impostato `sampleRate` al clock locale senza mantenere lo stesso rapporto
con `samplesPerFrame`. Browser a 44,1 e 48 kHz usano il clock operativo
48000/1024; ESP32 può usare la coppia equivalente 24000/512.

## Profili acustici

| Codice | ID ggwave | Uso |
| --- | --- | --- |
| `0` | `GGWAVE_PROTOCOL_AUDIBLE_NORMAL` | bootstrap robusto e fallback |
| `1` | `GGWAVE_PROTOCOL_AUDIBLE_FAST` | dati estesi su link verificato |
| `2` | `GGWAVE_PROTOCOL_AUDIBLE_FASTEST` | traffico rapido opzionale |
| `3` | `GGWAVE_PROTOCOL_DT_FASTEST` | banda base a frequenze più basse |

Un ESP32 può ricevere `DT_FASTEST` e trasmettere `MT_FASTEST` usando il codice
wire `3`. Ogni coppia richiesta/risposta usa lo stesso profilo. Il cambio banda
avviene soltanto in una finestra successiva, mai durante lo stesso scambio.

## Formato frame

Tutti i valori multibyte sono big-endian.

| Offset | Byte | Contenuto |
| --- | --- | --- |
| 0 | 1 | magic/versione `0xAA` |
| 1 | 1 | tipo pacchetto |
| 2 | 1 | ID acustico protetto, derivato da nome rete e password |
| 3 | 2 | ID mittente, esclusi `0x0000` e `0xFFFF` |
| 5 | 2 | ID destinatario; `0xFFFF` = broadcast |
| 7 | 1 | sequenza messaggio/join |
| 8 | 1 | parte nei 4 bit alti, totale nei 4 bit bassi |
| 9 | 1 | bit 7 esteso; bit 6–5 profilo; bit 4–0 lunghezza payload |
| 10 | 4 o 20 | payload, poi riempimento a zero |
| 14 o 30 | 2 | CRC16-CCITT dei byte precedenti |

Capacità utile: 4 byte nel frame base e 20 nell'esteso. Un testo può avere al
massimo 15 frammenti: 60 byte base o 300 byte estesi.

## Tipi

| Valore | Nome | Regola |
| --- | --- | --- |
| 1 | `JOIN` | richiesta broadcast base; un solo profilo per round |
| 2 | `WELCOME` | risposta indirizzata sul profilo del JOIN |
| 3 | `BEACON` | presenza/recovery su evento; mai heartbeat periodico |
| 4 | `DATA` | frammento messaggio |
| 5 | `ACK` | conferma di una parte diretta |
| 6 | `PING` | test indirizzato |
| 7 | `PONG` | risposta sul profilo del PING |
| 8 | `LEAVE` | uscita volontaria best effort |
| 9 | `SYNC` | epoca, checksum roster, numero membri e generazione |
| 10 | `QUEUE_STATE` | coda pendente e ultima sequenza ricevuta |
| 11 | `ROSTER` | fino a due ID per frame base, multipart |
| 12 | `JOIN_ACK` | conferma WELCOME e annullamento dei fallback |
| 13 | `FILE_RESERVE` | prenotazione broadcast del canale e profilo proposto |
| 14 | `FILE_OFFER` | metadati multipart sul profilo da verificare |
| 15 | `FILE_ACCEPT` | profilo confermato e primo indice mancante |
| 16 | `FILE_DATA` | indice a 16 bit e fino a 18 byte cifrati |
| 17 | `FILE_ACK` | ACK singolo legacy oppure richiesta/risposta bitmap della finestra |
| 18 | `FILE_COMPLETE` | tutti i pacchetti inviati; richiesta verifica finale |
| 19 | `FILE_COMPLETE_ACK` | verifica AES-GCM riuscita, broadcast dal ricevente |
| 20 | `FILE_RELEASE` | il mittente ha ricevuto la conferma e riapre la rete |
| 21 | `FILE_ABORT` | annullamento broadcast e ritorno alla rete normale |

## Identità di rete protetta

Il nome rete viene normalizzato in NFKC, convertito in maiuscolo e usato per
derivare il salt SHA-256 `AudioChat9|NOME`. Il namespace KDF resta volutamente
quello della v9 per migrare le chiavi ricordate senza cambiare fingerprint o
stanza protetta. La password, lunga almeno otto
caratteri, entra in PBKDF2-HMAC-SHA-256 con **150.000 iterazioni** e produce una
chiave master da 256 bit. La password non attraversa mai il canale acustico.

Dalla chiave master vengono derivati:

- il byte `room` usato per filtrare fisicamente i frame;
- un fingerprint locale di 64 bit, mostrato nell'interfaccia per confrontare i
  dispositivi;
- tramite HKDF-SHA-256, la chiave AES-256-GCM usata per i file.

Il byte `room` serve a proteggere e separare l'ID di rete, ma da solo non è un
autenticatore crittografico completo. I testi della v10 mantengono il formato
compatto della chat e non sono cifrati. I file ricevono invece confidenzialità e
autenticazione end-to-end tramite AES-GCM.

## Bootstrap, elezione e ingresso

1. Il nodo apre RX e ascolta prima di trasmettere.
2. Il backoff del round è
   `900 ms + hash(deviceId, room, round) % 6 × 1900 ms`. Ogni mini-finestra
   contiene un frame robusto completo più guardia. Il clear-channel assessment
   resta comunque vincolante.
3. Ogni round invia un solo `JOIN`. Una ricerca esplicita usa al massimo una
   coppia di tentativi: prima profilo `0` (Compatibile), poi profilo `3`
   (Banda bassa); quindi il
   trasmettitore torna muto e resta soltanto RX.
4. Se esiste una **leader lease** attiva, risponde soltanto il coordinatore. I
   follower non preparano più WELCOME di fallback: restano completamente muti.
5. Senza lease, un nodo che era già passivamente in ascolto può candidarsi a
   rispondere. I responder distribuiscono i WELCOME su finestre derivate da ID,
   ID del nuovo nodo e sequenza. Il primo WELCOME valido chiude globalmente tutte
   le candidature, anche quelle nate da JOIN diversi.
6. Se due o più device stanno eseguendo JOIN contemporaneamente, il tie-break è
   deterministico: fra due candidati che si sentono ha priorità l'ID più basso.
   Il perdente non invia WELCOME concorrenti e conserva il proprio JOIN solo per
   rendersi udibile. Con `2C31` e `9047`, entrambi convergono quindi su `2C31`.
7. `WELCOME` usa la stessa banda del JOIN. Il nuovo nodo risponde subito con
   `JOIN_ACK` sulla stessa banda. Se la conferma non arriva, il coordinatore può
   provare l'altra banda soltanto nel superframe successivo.
8. Un nodo passivo che sente il WELCOME destinato a un altro nodo apprende chi ha
   vinto la contesa e si presenta dopo una guardia. In questo modo una stanza
   rimasta muta per molto tempo ricostruisce progressivamente il roster senza
   heartbeat.
9. Per reti con più di due nodi il coordinatore invia `ROSTER` base: ogni parte
   contiene due ID. Mittente e destinatario sono già nell'header. Anche un nodo
   base ESP32 può quindi ricostruire il roster completo.
10. Il roster scoperto è *pending*. Il roster e la durata del superframe attivi
   non cambiano finché il coordinatore non trasmette il successivo `SYNC`.

Il coordinatore è quindi un ruolo temporaneo della conversazione, non un device
permanente. Se la stanza era inattiva, il primo scambio utile ricrea una lease;
se una sessione era ancora attiva, il coordinatore esistente conserva la
precedenza fino alla scadenza.

## Superframe TDMA

Il superframe contiene:

1. slot `0`: `SYNC` del coordinatore;
2. slot `1..N`: un turno per nodo, ordinato per ID nel roster attivo;
3. slot `N+1`: accesso/recovery per JOIN, WELCOME e riallineamento.

Ogni slot dura **4200 ms**, con 230 ms di guardia. La durata è
`(N + 2) × 4200 ms`: 16,8 s con due nodi, 21 s con tre, fino a 29,4 s con cinque.
Il frame più lento ammesso nello slot è il 32 byte/ROBUST da circa 2880 ms.

Quando entra un nodo, tutti continuano a calcolare le finestre con il roster
attivo precedente. Il leader trasmette il nuovo SYNC nella vecchia finestra
slot 0; soltanto allora l'ancora e il nuovo numero di membri entrano in vigore.
Chi ha checksum diverso sospende DATA e usa esclusivamente recovery/ROSTER.

Il ricevitore ricostruisce l'ancora sottraendo la durata nota del SYNC robusto
dal momento di decodifica. In v10 il SYNC non è un heartbeat: viene emesso per
aprire/riallineare una sessione o quando cambia il roster, non a ogni
superframe. Alla scadenza della lease non parte alcun recupero automatico: il
TDMA viene disattivato e tutti i trasmettitori restano muti finché un nuovo JOIN,
un messaggio broadcast o un'altra operazione esplicita riapre il traffico.

## Serializzazione e collisioni

- Una sola coda radio governa ogni frame, inclusi JOIN, WELCOME, ROSTER, SYNC,
  ACK, contatori, test e presenza.
- La coda prova tutti i frame maturi: un SYNC in attesa del proprio slot non può
  bloccare un WELCOME già nella finestra recovery.
- Ogni controllo ha scadenza. JOIN o WELCOME rinviati troppo a lungo vengono
  eliminati invece di partire insieme a un round nuovo.
- Carrier sense richiede almeno 330 ms di silenzio e applica backoff derivato
  da ID, sequenza e numero di rinvii.
- Le callback multiple prodotte dallo stesso segnale ggwave vengono deduplicate
  per 900 ms tramite tipo, mittente, destinatario, sequenza, parte, profilo e
  dimensione frame.
- Ogni avvio incrementa una generazione di sessione. Timer, watchdog e frame
  appartenenti a una generazione precedente non possono agire sulla nuova.

## Dati, test e coda persistente

DATA/ACK e PING/PONG usano lo stesso profilo nello stesso tentativo. La chat
diretta è stop-and-wait per frammento; il timeout copre il superframe massimo.
Il broadcast non ha ACK per evitare molte risposte contemporanee.

Ogni messaggio in uscita viene scritto in IndexedDB prima della trasmissione e
resta per sette giorni. La coda è FIFO e ammette un solo messaggio attivo per
destinatario. Dopo standby o riavvio, il mittente scambia `QUEUE_STATE` e attende
due superframe prima di ritrasmettere.

`QUEUE_STATE` usa quattro byte:

| Byte | Contenuto |
| --- | --- |
| 0 | numero pendenti |
| 1 | sequenza in testa |
| 2 | ultima sequenza completata dal destinatario |
| 3 | bit 0 richiesta risposta; bit 1 byte 2 valido |

La chiave anti-duplicato del messaggio completo è `(sender, sequence)`. Un
duplicato diretto riceve nuovamente ACK senza essere mostrato due volte.

Safari/iOS può sospendere microfono, AudioContext e timer in background. Coda,
chat e log vengono salvati; al ritorno la pipeline viene ricreata se necessario,
poi JOIN, SYNC e contatori vengono riallineati.

## Sessione file esclusiva

Il trasferimento è sempre uno-a-uno. Il mittente invia `FILE_RESERVE` in banda
robusta e in broadcast, indicando ricevente, profilo candidato e lease. Ogni
nodo che lo decodifica conserva i normali frame in coda e sospende JOIN, BEACON,
SYNC, messaggi e test. Solo mittente e ricevente possono emettere frame con
accesso `exclusive`.

La negoziazione procede così:

1. il mittente propone `TURBO`, poi `FAST`, `ROBUST` e `LOW_BAND`;
2. invia i metadati `FILE_OFFER` direttamente sul profilo candidato;
3. se il ricevente li decodifica, risponde `FILE_ACCEPT` sullo stesso profilo e
   indica il primo pacchetto mancante;
4. senza conferma il mittente ripete `FILE_RESERVE` e prova il profilo seguente;
5. dopo una perdita ripetuta durante i dati viene eseguito un nuovo handshake e
   la trasmissione riprende dall'indice comunicato dal ricevente.

Prima dell'offerta il mittente prova la compressione GZIP. La usa soltanto per
file di almeno 256 byte e solo quando il risultato, incluso un margine di 64
byte, resta piu piccolo dell'originale; altrimenti usa `identity`. Il contenuto
risultante viene cifrato con AES-256-GCM usando IV casuale da 96 bit. I metadati
canonici (ID trasferimento, mittente, ricevente, nome, dimensione, MIME e, se
presente, encoding) sono AAD. Il flusso `IV || ciphertext || tag` viene diviso
in blocchi da 18 byte e `FILE_DATA` antepone l'indice big-endian a 16 bit.

La v10-fast negozia `transport = window-v1` nei metadati. Se il ricevente lo
accetta, aggiunge un quarto byte a `FILE_ACCEPT` e il mittente passa dal vecchio
stop-and-wait a finestre adattive. La finestra parte da 8 blocchi, cresce a 10 e
poi 12 dopo finestre pulite e torna a 8 quando manca qualcosa. Terminato il
burst, il mittente invia un `FILE_ACK` a 4 byte con `baseIndex` e bitmap zero
come richiesta. Il ricevente risponde con lo stesso `baseIndex` e un bitmap a
16 bit che indica i blocchi gia presenti. Vengono ritrasmessi soltanto i blocchi
mancanti. Dopo perdite ripetute il mittente rinegozia un profilo piu robusto e
riprende dall'indice conservato dal ricevente. Un peer che non negozia
`window-v1` continua a usare il precedente `FILE_ACK` a 2 byte per ogni blocco.

Dopo `FILE_COMPLETE`, il ricevente ricompone il flusso, verifica il tag GCM,
decomprime GZIP quando indicato e verifica la dimensione in chiaro. Soltanto
dopo una verifica riuscita emette
`FILE_COMPLETE_ACK` in banda robusta e broadcast, conservando lo stato per poter
rispondere di nuovo se la conferma viene persa. Quando il mittente riceve questa
conferma trasmette `FILE_RELEASE` in banda robusta: soltanto allora tutti i nodi
riaprono la rete normale. `FILE_ABORT` e la scadenza della lease garantiscono il
ritorno allo stato normale anche in caso di interruzione. La web app limita un
file a 512 KiB e richiede che entrambi i partecipanti supportino il modem esteso
da 32 byte.

## Implementazioni

- **Browser / PC / Raspberry:** applicazione web con ggwave/WASM incluso.
- **Raspberry nativo:** stesso codec con ggwave C++ e backend audio locale.
- **ESP32:** frame base 16 byte, 24000 Hz/512 campioni, RX `DT_FASTEST`, TX
  `MT_FASTEST`, codec in `audiochat_protocol.h`, speaker e microfono I2S. Per i
  file deve inizializzare anche il modem esteso da 32 byte.

Il magic `0xAA` identifica il wire protocol v10. Versioni incompatibili devono
cambiare magic e non reinterpretare i campi esistenti.
