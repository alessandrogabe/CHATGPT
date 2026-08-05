#pragma once

#include <stddef.h>
#include <stdint.h>

namespace audiochat_v10 {

constexpr uint8_t kMagic = 0xAA;
constexpr size_t kBaseFrameSize = 16;
constexpr size_t kExtendedFrameSize = 32;
constexpr uint16_t kBroadcastId = 0xFFFF;
constexpr float kToneBinHz = 46.875f;
constexpr uint32_t kBrowserSampleRate = 48000;
constexpr uint16_t kBrowserSamplesPerFrame = 1024;
constexpr uint32_t kEsp32SampleRate = 24000;
constexpr uint16_t kEsp32SamplesPerFrame = 512;
constexpr uint16_t kDiscoveryInitialListenMs = 900;
constexpr uint8_t kContentionSlots = 6;
constexpr uint16_t kContentionSlotMs = 1900;
constexpr uint8_t kResponseSlots = 4;
constexpr uint16_t kResponseStartMs = 1900;
constexpr uint16_t kResponseSlotMs = 1900;
constexpr uint32_t kLeaderLeaseMs = 92000;
constexpr uint16_t kPhysicalDedupeMs = 900;
constexpr uint16_t kChannelGuardMs = 260;
constexpr uint16_t kNetworkSlotMs = 4200;
constexpr uint16_t kNetworkSlotGuardMs = 230;
constexpr uint8_t kMaximumNodes = 5;
constexpr uint8_t kQueueReconcileSuperframes = 2;
constexpr uint16_t kQueueReconcileGuardMs = 5000;
constexpr uint8_t kQueueFlagReplyRequest = 0x01;
constexpr uint8_t kQueueFlagHasLastReceived = 0x02;

enum PacketType : uint8_t {
  JOIN = 1,
  WELCOME = 2,
  BEACON = 3,
  DATA = 4,
  ACK = 5,
  PING = 6,
  PONG = 7,
  LEAVE = 8,
  SYNC = 9,
  QUEUE_STATE = 10,
  ROSTER = 11,
  JOIN_ACK = 12,
  FILE_RESERVE = 13,
  FILE_OFFER = 14,
  FILE_ACCEPT = 15,
  FILE_DATA = 16,
  FILE_ACK = 17,
  FILE_COMPLETE = 18,
  FILE_COMPLETE_ACK = 19,
  FILE_RELEASE = 20,
  FILE_ABORT = 21,
};

enum Profile : uint8_t {
  ROBUST = 0,
  FAST = 1,
  TURBO = 2,
  LOW_BAND = 3,
};

struct Packet {
  PacketType type;
  uint8_t room;
  uint16_t sender;
  uint16_t target;
  uint8_t sequence;
  uint8_t part;
  uint8_t total;
  Profile profile;
  bool extendedCapable;
  uint8_t payload[20];
  uint8_t payloadLength;
  uint8_t frameSize;
};

struct ScheduleState {
  uint8_t rosterChecksum;
  uint8_t memberCount;
  uint8_t generation;
};

struct QueueState {
  uint8_t pendingCount;
  uint8_t headSequence;
  uint8_t lastReceivedSequence;
  bool hasLastReceived;
  bool replyRequested;
};

inline uint16_t crc16Ccitt(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t index = 0; index < length; ++index) {
    crc ^= static_cast<uint16_t>(data[index]) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

inline uint8_t asciiUpper(uint8_t value) {
  return value >= 'a' && value <= 'z' ? static_cast<uint8_t>(value - 32) : value;
}

// Hash stanza legacy, utile soltanto per test e migrazione. In AudioChat v10 il
// campo `room` deve contenere il byte derivato da PBKDF2-SHA-256(nome, password)
// o deve essere precaricato durante il provisioning della periferica.
inline uint8_t roomHashAscii(const char *roomName) {
  if (!roomName) roomName = "LOCALE";
  size_t start = 0;
  while (roomName[start] == ' ' || roomName[start] == '\t') ++start;
  size_t end = start;
  while (roomName[end] != '\0') ++end;
  while (end > start && (roomName[end - 1] == ' ' || roomName[end - 1] == '\t')) --end;
  if (end == start) {
    roomName = "LOCALE";
    start = 0;
    end = 6;
  }
  uint16_t crc = 0xFFFF;
  for (size_t index = start; index < end; ++index) {
    const uint8_t value = asciiUpper(static_cast<uint8_t>(roomName[index]));
    crc ^= static_cast<uint16_t>(value) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
    }
  }
  return static_cast<uint8_t>((crc >> 8) ^ crc);
}

inline size_t bodyCapacity(size_t frameSize) {
  return frameSize == kBaseFrameSize || frameSize == kExtendedFrameSize
             ? frameSize - 12
             : 0;
}

inline uint8_t rosterChecksum(const uint16_t *ids, size_t count) {
  uint16_t crc = 0xFFFF;
  for (size_t index = 0; index < count; ++index) {
    const uint8_t bytes[2] = {
      static_cast<uint8_t>(ids[index] >> 8),
      static_cast<uint8_t>(ids[index])
    };
    for (uint8_t value : bytes) {
      crc ^= static_cast<uint16_t>(value) << 8;
      for (uint8_t bit = 0; bit < 8; ++bit) {
        crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                             : static_cast<uint16_t>(crc << 1);
      }
    }
  }
  return static_cast<uint8_t>((crc >> 8) ^ crc);
}

inline uint32_t mix32(uint32_t value) {
  value ^= value >> 16;
  value *= 0x7FEB352DU;
  value ^= value >> 15;
  value *= 0x846CA68BU;
  value ^= value >> 16;
  return value;
}

inline uint16_t joinContentionDelayMs(uint16_t deviceId, uint8_t room,
                                      uint16_t round) {
  const uint32_t seed = mix32((static_cast<uint32_t>(deviceId) << 8) ^ room ^
                              ((static_cast<uint32_t>(round) + 1U) * 0x9E3779B1U));
  return static_cast<uint16_t>(kDiscoveryInitialListenMs +
                               (seed % kContentionSlots) * kContentionSlotMs);
}

inline Profile joinProfileForRound(uint16_t round) {
  return (round & 1U) == 0 ? ROBUST : LOW_BAND;
}

// Tie-break di bootstrap usato quando piu nodi iniziano un JOIN insieme.
// Una lease gia attiva ha precedenza; questa funzione si usa solo senza lease.
inline uint16_t provisionalCoordinatorId(uint16_t selfId,
                                         const uint16_t *peerIds,
                                         size_t peerCount) {
  uint16_t winner = selfId;
  for (size_t index = 0; index < peerCount; ++index) {
    const uint16_t id = peerIds[index];
    if (id != 0 && id != kBroadcastId && id < winner) winner = id;
  }
  return winner;
}

inline uint16_t coordinatorResponseDelayMs(uint16_t responderId,
                                           uint16_t newcomerId,
                                           uint8_t sequence) {
  const uint32_t seed = mix32((static_cast<uint32_t>(responderId) << 16) ^
                              newcomerId ^
                              ((static_cast<uint32_t>(sequence) + 1U) * 0x045D9F3BU));
  return static_cast<uint16_t>(kResponseStartMs +
                               (seed % kResponseSlots) * kResponseSlotMs);
}

inline bool bootstrapWelcomeAllowed(uint16_t selfId, uint16_t joinSenderId,
                                    bool ownJoinActive) {
  return !ownJoinActive || selfId < joinSenderId;
}

inline int8_t nodeSlotIndex(uint16_t selfId, const uint16_t *roster, size_t count) {
  for (size_t index = 0; index < count; ++index) {
    if (roster[index] == selfId) return static_cast<int8_t>(index + 1);
  }
  return -1;
}

inline uint32_t superframeDurationMs(uint8_t memberCount) {
  const uint8_t safeCount = memberCount == 0 ? 1 : memberCount;
  return static_cast<uint32_t>(safeCount + 2) * kNetworkSlotMs;
}

inline uint32_t queueReconciliationWindowMs(uint8_t memberCount) {
  return superframeDurationMs(memberCount) * kQueueReconcileSuperframes +
         kQueueReconcileGuardMs;
}

inline void encodeScheduleState(const ScheduleState &state, uint8_t payload[4]) {
  payload[0] = state.rosterChecksum;
  payload[1] = state.memberCount;
  payload[2] = state.generation;
  payload[3] = static_cast<uint8_t>(kNetworkSlotMs / 100);
}

inline bool decodeScheduleState(const uint8_t payload[4], ScheduleState &state) {
  if (!payload || payload[1] == 0 || payload[1] > kMaximumNodes ||
      payload[3] != kNetworkSlotMs / 100) return false;
  state.rosterChecksum = payload[0];
  state.memberCount = payload[1];
  state.generation = payload[2];
  return true;
}

inline void encodeQueueState(const QueueState &state, uint8_t payload[4]) {
  payload[0] = state.pendingCount;
  payload[1] = state.headSequence;
  payload[2] = state.lastReceivedSequence;
  payload[3] = static_cast<uint8_t>(
    (state.replyRequested ? kQueueFlagReplyRequest : 0) |
    (state.hasLastReceived ? kQueueFlagHasLastReceived : 0)
  );
}

inline bool decodeQueueState(const uint8_t payload[4], QueueState &state) {
  if (!payload) return false;
  state.pendingCount = payload[0];
  state.headSequence = payload[1];
  state.lastReceivedSequence = payload[2];
  state.replyRequested = (payload[3] & kQueueFlagReplyRequest) != 0;
  state.hasLastReceived = (payload[3] & kQueueFlagHasLastReceived) != 0;
  return true;
}

// `knownPeerIds` deve contenere ID unici del roster locale. Se non sono noti
// altri responder, gli ID e la sequenza distribuiscono la risposta su 4 slot.
inline uint8_t welcomeSlotIndex(uint16_t selfId, uint16_t newcomerId,
                                uint8_t sequence,
                                const uint16_t *knownPeerIds,
                                size_t knownPeerCount) {
  size_t responderCount = 1;
  uint8_t rank = 0;
  for (size_t index = 0; index < knownPeerCount; ++index) {
    const uint16_t id = knownPeerIds[index];
    if (id == 0 || id == kBroadcastId || id == newcomerId || id == selfId) continue;
    ++responderCount;
    if (id < selfId) ++rank;
  }
  if (responderCount > 1) return rank;
  return static_cast<uint8_t>(((selfId ^ newcomerId ^ (sequence * 31U)) & 0xFFFFU) % 4U);
}

inline bool encode(const Packet &packet, uint8_t *output, size_t frameSize) {
  const size_t capacity = bodyCapacity(frameSize);
  if (!output || capacity == 0 || packet.payloadLength > capacity ||
      packet.sender == 0 || packet.sender == kBroadcastId ||
      packet.target == 0 || packet.type < JOIN || packet.type > FILE_ABORT ||
      packet.profile > LOW_BAND || packet.part > 14 || packet.total == 0 ||
      packet.total > 15 || packet.part >= packet.total) {
    return false;
  }
  for (size_t index = 0; index < frameSize; ++index) output[index] = 0;
  output[0] = kMagic;
  output[1] = static_cast<uint8_t>(packet.type);
  output[2] = packet.room;
  output[3] = static_cast<uint8_t>(packet.sender >> 8);
  output[4] = static_cast<uint8_t>(packet.sender);
  output[5] = static_cast<uint8_t>(packet.target >> 8);
  output[6] = static_cast<uint8_t>(packet.target);
  output[7] = packet.sequence;
  output[8] = static_cast<uint8_t>((packet.part << 4) | packet.total);
  output[9] = static_cast<uint8_t>((packet.extendedCapable ? 0x80 : 0) |
                                   ((packet.profile & 0x03) << 5) |
                                   packet.payloadLength);
  for (uint8_t index = 0; index < packet.payloadLength; ++index) {
    output[10 + index] = packet.payload[index];
  }
  const uint16_t crc = crc16Ccitt(output, frameSize - 2);
  output[frameSize - 2] = static_cast<uint8_t>(crc >> 8);
  output[frameSize - 1] = static_cast<uint8_t>(crc);
  return true;
}

inline bool decode(const uint8_t *frame, size_t frameSize, Packet &packet) {
  const size_t capacity = bodyCapacity(frameSize);
  if (!frame || capacity == 0 || frame[0] != kMagic) return false;
  const uint16_t expected = static_cast<uint16_t>(frame[frameSize - 2] << 8) |
                            frame[frameSize - 1];
  if (crc16Ccitt(frame, frameSize - 2) != expected) return false;
  const uint8_t payloadLength = frame[9] & 0x1F;
  const uint8_t type = frame[1];
  if (payloadLength > capacity || type < JOIN || type > FILE_ABORT) return false;

  packet.type = static_cast<PacketType>(type);
  packet.room = frame[2];
  packet.sender = static_cast<uint16_t>(frame[3] << 8) | frame[4];
  packet.target = static_cast<uint16_t>(frame[5] << 8) | frame[6];
  packet.sequence = frame[7];
  packet.part = frame[8] >> 4;
  packet.total = frame[8] & 0x0F;
  packet.profile = static_cast<Profile>((frame[9] >> 5) & 0x03);
  packet.extendedCapable = (frame[9] & 0x80) != 0;
  packet.payloadLength = payloadLength;
  packet.frameSize = static_cast<uint8_t>(frameSize);
  for (uint8_t index = 0; index < payloadLength; ++index) {
    packet.payload[index] = frame[10 + index];
  }
  return packet.sender != 0 && packet.sender != kBroadcastId && packet.target != 0 &&
         packet.total > 0 && packet.total <= 15 && packet.part < packet.total;
}

}  // namespace audiochat_v10
