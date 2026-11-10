# RF RX/TX timing

This describes the COBC firmware schedule for the housekeeping beacon and the receive windows that follow it. The timing of one simple command, `TC_8_1_SYNCHRONIZE_TIME` at the earliest instant COSMOS allows, is shown in [rf-timing-simple-cmd.md](rf-timing-simple-cmd.md). The only ground-station timing stated in the firmware itself is the pause before switching from RX back to TX.

The schedule is implemented in `RfCommunicationThread`. The beacon period is set in `TelemetryThread`.

These timings exist because the link is half duplex. RX and TX use one frequency: the frequency-control registers in `DoInitialize` are a single setting (the comment there gives 437.395 MHz), and both `StartTx` and `StartRx` select channel 0. The modem is in transmit, in receive, or in standby, never both at once (`isInTxMode` in `Sts1CobcSw/Rf/Rf.cpp`). The cubesat and the ground station therefore have to agree on who is transmitting and who is listening. A transmission that overlaps the other side's transmission is lost, which is why the beacon, the receive windows, the 300 ms switch pause, and the CFDP retries are scheduled as they are.

## Summary

| What | Value in code | Name |
| --- | --- | --- |
| Beacon period | 30 s | `telemetryThreadInterval` |
| Delay before the first beacon | 1 min, after startup tests | `initialTelemetryDelay` |
| Receive window after a beacon | 5 s | `rxTimeoutAfterTelemetryRecord` |
| Receive window after a complete uplink frame | 3 s, and again while frames keep arriving | `rxTimeoutForAdditionalData` |
| Minimum time left before another receive attempt | 1 s | `minRxTimeout` |
| Pause after RX before the next TX | 300 ms, compile-time constant on the COBC | `rxToTxSwitchDuration` |
| Pause after TX before RX is usable | 100 ms | `postTxDelay` |
| Downlink frame on air | 520 bytes at the TX rate. 433.3 ms at the default 9600 bit/s | `fullyEncodedFrameLength`, `GetTxDataRate()` |
| Uplink frame the COBC waits for | 255 bytes at the RX rate. 212.5 ms at 9600 bit/s | `blockLength`, `GetRxDataRate()` |

Acceptance and completion reports are not on a timer of their own. The delay from the end of reception to each report is the sum of the pieces in [Verification reports](#verification-reports-acknack).

## Beacon

`TelemetryThread` publishes one telemetry record every 30 s. That record is the beacon: `RfCommunicationThread` sends it as a housekeeping parameter report as soon as the mailbox is full.

```45:48:Sts1CobcSw/Firmware/TelemetryThread.cpp
constexpr auto telemetryThreadInterval = 30 * s;
// The delay until the first telemetry record is published and thereby the first beacon is sent.
// This prevents immediate resets if we boot with too little power for the RF module.
constexpr auto initialTelemetryDelay = 1 * min;
```

```83:92:Sts1CobcSw/Firmware/TelemetryThread.cpp
        TIME_LOOP(0, value_of(telemetryThreadInterval))
        {
            auto firmwareIsIntact = CheckFirmwareIntegrities();
            persistentVariables.Store<"realTime">(CurrentRealTime());
            auto telemetryRecord = CollectTelemetryData();
            telemetryMemory.PushBack(telemetryRecord);
            DEBUG_PRINT("Publishing telemetry record\n");
            telemetryRecordMailbox.Overwrite(telemetryRecord);
            nextTelemetryRecordTimeMailbox.Overwrite(CurrentRodosTime() + telemetryThreadInterval);
            ResumeRfCommunicationThread();
```

Both threads wait for `totalStartupTestTimeout` (400 ms) before starting. The first beacon is therefore about one minute after that, not 30 s after boot. `TelemetryThread` has priority 910 and `RfCommunicationThread` has priority 900, so the telemetry thread writes `nextTelemetryRecordTimeMailbox` before the RF thread reads it.

The send itself is `SendAndWait` of `HousekeepingParameterReport` in `RfCommunicationThread::run`.

## Receive window after a beacon

Immediately after the beacon has been sent, the radio listens for 5 s.

```82:88:Sts1CobcSw/Firmware/RfCommunicationThread.cpp
constexpr auto rxTimeoutAfterTelemetryRecord = 5 * s;
constexpr auto rxTimeoutForAdditionalData = 3 * s;
constexpr auto minRxTimeout = 1 * s;
constexpr auto estimatedMaxDataProcessingDuration = 100 * ms;
// The ground station needs some time to switch from TX to RX so we need to wait for that when
// switching from RX to TX
constexpr auto rxToTxSwitchDuration = 300 * ms;
```

```187:196:Sts1CobcSw/Firmware/RfCommunicationThread.cpp
            if(telemetryRecordMailbox.IsFull())
            {
                DEBUG_PRINT("Sending housekeeping parameter report\n");
                SendAndWait(HousekeepingParameterReport(telemetryRecordMailbox.Get().value()));
                DEBUG_PRINT("Receiving for %" PRIi64 " s\n", rxTimeoutAfterTelemetryRecord / s);
                auto receiveResult = ReceiveAndHandleData(rxTimeoutAfterTelemetryRecord);
                DEBUG_PRINT("\n");
                moreDataShouldBeReceived = receiveResult.has_value();
                DEBUG_PRINT_STACK_USAGE();
                continue;
            }
```

`ReceiveAndHandleData` succeeds only when `rf::Receive` fills the whole telecommand buffer. A timeout, or a short reception, does not open another window.

## Further receptions

If that receive succeeds, the thread listens again. Each further attempt lasts at most 3 s. It is shortened so that it ends `EstimateDataHandlingDuration()` before the next beacon, and it is skipped entirely when less than 1 s would remain.

```204:218:Sts1CobcSw/Firmware/RfCommunicationThread.cpp
            if(moreDataShouldBeReceived)
            {
                auto remainingRxDuration = nextTelemetryRecordTimeMailbox.Peek().value()
                                         - CurrentRodosTime() - EstimateDataHandlingDuration();
                if(remainingRxDuration > minRxTimeout)
                {
                    auto rxTimeout = std::min(remainingRxDuration, rxTimeoutForAdditionalData);
                    DEBUG_PRINT("Receiving for %" PRIi64 " s\n", rxTimeout / s);
                    auto receiveResult = ReceiveAndHandleData(rxTimeout);
                    DEBUG_PRINT("\n");
                    moreDataShouldBeReceived = receiveResult.has_value();
                    continue;
                }
            }
```

A successful follow-up receive sets `moreDataShouldBeReceived` again, so the 3 s window repeats until a receive times out or the next beacon is too close. It is not a single extra slot after the first uplink frame.

Between beacons the thread may also send queued CFDP frames (`SendCfdpFrames`). That send window ends 1 s (`sendWindowMargin`) before the next beacon or before `fileTransferWindowEnd`, whichever is earlier. A new telemetry record interrupts that send. How many of those frames fit is computed from the same downlink airtime as below.

## Packet airtime

The 5 s and 3 s receive windows are fixed. The time a frame occupies the channel is not. It is the number of bits the modem sends or receives, divided by the current baud rate. RX and TX rates are independent. Both default to 9600 bit/s (`defaultDataRateConfig` in `Sts1CobcSw/Rf/Rf.cpp`) and can be changed by telecommand. The configured rates are 1200, 2400, 4800, 9600, 19200, 38400, 57600, 76800, and 115200 bit/s in `Sts1CobcSw/Rf/RfDataRateConfigs.hpp`.

### Downlink

A housekeeping report and every other TM or CFDP frame is one channel-access data unit: the 4-byte attached sync marker plus one Reed-Solomon block. The default build convolutionally encodes that unit with flush bits and no puncturing (`USE_PUNCTURING`, `DISABLE_CHANNEL_CODING`, and `DISABLE_CONVOLUTIONAL_CODING` are off). `fullyEncodedFrameLength` is that encoded size, 520 bytes:

```28:30:Sts1CobcSw/ChannelCoding/ChannelCoding.hpp
inline constexpr auto channelAccessDataUnitLength = attachedSynchMarkerLength + blockLength;
inline constexpr auto fullyEncodedFrameLength = static_cast<unsigned>(
    cc::ViterbiCodec::EncodedSize(channelAccessDataUnitLength, /*withFlushBits=*/true));
```

`DoSetTxDataLength` programs the Si4463 packet length to `EncodedSize` of the buffer it is about to send, so the modem transmits those 520 bytes at `GetTxDataRate()`. The firmware estimates one frame as

```239:243:Sts1CobcSw/Firmware/RfCommunicationThread.cpp
auto EstimateDataHandlingDuration() -> Duration
{
    // Worst case is that we receive a request and need to send two reports in response
    auto frameSendDuration = fullyEncodedFrameLength * CHAR_BIT * s / rf::GetTxDataRate();
    return 2 * frameSendDuration + estimatedMaxDataProcessingDuration;
```

`s` is one RODOS second in nanoseconds, and the division is integer. The same `frameSendDuration` decides whether another CFDP frame still fits before `sendWindowEnd`. `EstimateDataHandlingDuration()` reserves two such frames plus 100 ms, which is the time held back from the next receive window.

The modem also sends a preamble in front of that payload. `ComputePreambleLength` sizes it so the preamble covers the 500 µs TX-amplifier startup: `((500e-6 * baudRate) / 8)` rounded up. At 9600 bit/s that is 1 byte. `frameSendDuration` does not include it. The modem sync word is not transmitted; the comment in `DoInitialize` says the attached sync marker inside the encoded unit is what goes out.

| TX rate | One downlink frame (`520 * 8 / rate`) | Two frames plus 100 ms |
| --- | ---: | ---: |
| 1200 bit/s | 3.467 s | 7.033 s |
| 2400 bit/s | 1.733 s | 3.567 s |
| 4800 bit/s | 867 ms | 1.833 s |
| 9600 bit/s | 433 ms | 967 ms |
| 19200 bit/s | 217 ms | 533 ms |
| 38400 bit/s and above | 108 ms or less | 317 ms or less |

9600 bit/s is the standard rate. At that rate one downlink frame is 433 ms and the two-frame reserve is under 1 s, so it fits in the 5 s window. 1200 bit/s is a border case: the same reserve is 7.0 s, longer than the window, so a successful receive at that rate leaves no further receive attempt before the next beacon.

`FinalizeTransmission` uses a separate, baud-derived timeout for the TX FIFO rather than for a whole frame. The comment sizes it for a FIFO of at most 128 bytes at 1200 bit/s (0.853 s) and sets the timeout to 1 s.

### Uplink

Uplink frames are not convolutionally coded. The init comment in `Sts1CobcSw/Rf/Rf.cpp` says up- and downlink share one modem configuration except for that. `rf::Receive` reads `blockLength` bytes from the FIFO, which is the Reed-Solomon block (255 bytes: `maxUnencodedSize` in `ConvolutionalCoding.hpp` is `255 + 4`, and 520 encoded bytes is `EncodedSize(259, true)`). The modem detects the 4-byte sync word before those FIFO bytes, and a preamble before the sync word.

The COBC does not compute an uplink airtime. It waits until 255 bytes have arrived or the 5 s / 3 s timeout expires. The time those 255 bytes take is `blockLength * 8 / rxDataRate`:

| RX rate | 255-byte block |
| --- | ---: |
| 1200 bit/s | 1.700 s |
| 2400 bit/s | 850 ms |
| 4800 bit/s | 425 ms |
| 9600 bit/s | 212.5 ms |
| 19200 bit/s | 106.3 ms |

The sync word adds 4 bytes at the same rate (27 ms at 1200 bit/s, 3.3 ms at 9600 bit/s). A block that starts too late in the window still runs into the timeout.

### RF redirected over UCI

With `REDIRECT_RF_OVER_UCI`, `RfOverUci.cpp` replaces `Rf.cpp`. `SendAndWait` still convolutionally encodes the frame, then waits for a UART transfer at 10 bits per byte (`txDataRate / 10`, start and stop bit included). The timeout is twice that encoded-frame time plus 10 ms.

## Ground-station TX-to-RX switch time

That 300 ms is not configured on the ground station, and it is not a telecommand or a persistent variable. This repository has no COSMOS setting for it. The COBC waits that long itself, as a compile-time constant in `RfCommunicationThread.cpp`:

```86:88:Sts1CobcSw/Firmware/RfCommunicationThread.cpp
// The ground station needs some time to switch from TX to RX so we need to wait for that when
// switching from RX to TX
constexpr auto rxToTxSwitchDuration = 300 * ms;
```

`ReceiveAndHandleData` stores the time when `rf::Receive` returns, including when the receive times out:

```247:250:Sts1CobcSw/Firmware/RfCommunicationThread.cpp
auto ReceiveAndHandleData(Duration rxTimeout) -> Result<void>
{
    auto nReceivedBytes = rf::Receive(tcBuffer, rxTimeout);
    lastRxTime = CurrentRodosTime();
```

Every send goes through `SuspendUntilEarliestTxTime` before `rf::SendAndWait` or `rf::SendAndContinue`. That is the only use of the constant:

```1067:1073:Sts1CobcSw/Firmware/RfCommunicationThread.cpp
auto SuspendUntilEarliestTxTime() -> void
{
    auto earliestTxTime = lastRxTime + rxToTxSwitchDuration;
    if(earliestTxTime > CurrentRodosTime())
    {
        SuspendUntil(earliestTxTime);
    }
```

If handling the received frame already took 300 ms or more, the wait is zero. The constant has been 300 ms since it was introduced in commit `f1cdf6d` (2025-06-14). Nothing in the code derives it from the data rate.

## Switching the transceiver

`rf::Receive` calls `StartRx()` and, when it returns, `DoEnterStandbyMode()`. `rf::SendAndWait` and `rf::SendAndContinue` call `StartTx()` and leave TX mode when the packet has been sent. The modem commands are `StartRx` and `StartTx` in `Sts1CobcSw/Rf/Rf.cpp`.

Separate from the ground-station pause above, `DoSuspendUntilDataSent` waits `postTxDelay` (100 ms) after a transmission before RX is used again. The comment there says receiving does not work without this pause. That delay is in the RF driver, not in the communication thread:

```115:115:Sts1CobcSw/Rf/Rf.cpp
constexpr auto postTxDelay = 100 * ms;  // Time to wait after sending data to prevent RX issues
```

`txIsOn` can suppress transmission. `DoSendAndWait` and `DoSendAndContinue` return without sending when that persistent variable is false. The receive windows are unchanged. The rates used for the airtimes above are reported in the beacon.

## Verification reports (ACK/NACK)

The firmware sends two PUS verification stages, and only those two: acceptance, then completion of execution. There is no start-of-execution report. Both are built in `RfProtocols/Reports.cpp` and sent by `RfCommunicationThread` itself through `SendAndWait`. No other thread is asked to produce the report.

`lastRxTime` is stored when `rf::Receive` returns, before any of this parsing. The 300 ms ground-station pad is therefore measured from the end of reception, and the work below uses up part of it.

### Before the acceptance report

On a frame that decodes and parses:

1. Reed-Solomon decode of the received block (`tc::Decode`).
2. Persistent-variable updates in `HandleReceivedData` and `HandleRequestFrame`: `nResetsSinceRf`, the correctable-uplink-error counter, `nGoodTransferFrames`, `lastFrameSequenceNumber`, `lastMessageTypeIdWasInvalid`, and `lastMessageTypeId`. Each `Store` writes three FRAM copies. The SPI timeout on those transfers is 1 ms (`PersistentVariables::spiTimeout`); that is a cap on one transfer, not a sleep added on top.
3. Parsing of the transfer frame, the space packet, the request, and the application data (`VerifyAndHandle`).
4. `SuspendUntilEarliestTxTime`, which sleeps only the remainder of `lastRxTime + 300 ms`.
5. Reed-Solomon and convolutional encoding of one TM frame, then transmission of that frame at the TX rate (one downlink airtime from above).

A failed request parse or a failed application-data parse sends `FailedAcceptanceVerificationReport` at this same point and returns. No completion report follows. A frame that fails Reed-Solomon decode, or that parses as a transfer frame but not as a space packet, produces no verification report at all.

`estimatedMaxDataProcessingDuration` (100 ms) is not inserted here. It is only subtracted when deciding how long the next receive window may be.

### Between acceptance and completion

`SendAndWait` of the acceptance report includes `postTxDelay` (100 ms) after the packet-sent interrupt. The acceptance frame is already on the air before that pause. The pause, and then the command body, sit in front of the completion frame.

The completion report is a second `SendAndWait`. It does not wait another 300 ms: `lastRxTime` is still the receive time, and the acceptance transmission has already passed `lastRxTime + 300 ms`. It does add a second downlink airtime.

The command body runs on the RF thread, inline, before that second frame. What it does:

| Command | Work after acceptance, before completion |
| --- | --- |
| Load raw memory | `fram::WriteTo` of the payload. The 100 ms argument is the SPI timeout cap. |
| Delete a file | `fs::ForceRemove` (littlefs, on the external flash). |
| Copy a file | Path checks. A cubesat source file is opened and sized. A firmware destination erases one 128 KiB flash sector, busy-waiting on `FLASH_SR_BSY` in `DoErase`, with no timeout. Any other cubesat destination creates and resizes the file. The report means the transfer was initiated. Overwriting `fileTransferMetadataMailbox` wakes `FileTransferThread` if it is waiting, but that thread has lower priority, so it runs only once the RF thread suspends inside the completion transmission. |
| Update EDU queue | Store `eduProgramQueueIndex`, `programQueue.Clear()`, then `PushBack` of each entry. Each entry is one FRAM element write (`FramVector` SPI timeout is 1 ms, or 3 µs per byte for a large element). |
| Check firmware integrity | CRC-32 of the image in 128-byte reads, up to `partitionSize` (128 KiB), in `ComputeAndReadFirmwareChecksums`. |
| Stop antenna deployment, enable TX, enable file transfer, synchronize time, set active or backup firmware | A persistent-variable store, or `rf::EnableTx()`. |

`disableCubeSatTx` and `resetNow` send neither report. `disableCubeSatTx` turns TX off and returns. `resetNow` calls `RODOS::hwResetAndReboot()`.

These commands send the acceptance report and then data frames instead of a completion report. Each data frame is another downlink airtime. A completion report is sent only when the file or repository operation fails:

- Dump raw memory (`fram::ReadFrom`, 100 ms SPI timeout cap per area)
- Report or set parameter values (set also writes the new value first)
- Report file attributes (`fs::IsLocked`, `fs::FileSize`). Details are in [File and repository results](#file-and-repository-results).
- Repository content summary (a directory iteration). Details are in [File and repository results](#file-and-repository-results).
- Housekeeping history (`telemetryMemory.Get` in a loop)

`SetTxDataLength` before a multi-frame reply programs the modem packet length for the whole burst. `SendAndContinue` still calls `SuspendUntilEarliestTxTime` before every frame, which only sleeps if the previous frame finished inside the original 300 ms pad. `FinalizeTransmission` then waits up to 1 s for the TX FIFO to drain.

### Scheduling

Because the reports are sent on `RfCommunicationThread` (priority 900), only a higher-priority thread can push them later:

- `StartupAndSpiSupervisorThread` (priority 1000) wakes every 1 s (`supervisionPeriod`) and compares SPI transfer deadlines. That is a short preemption once a second.
- `TelemetryThread` (priority 910) wakes every 30 s and calls `CheckFirmwareIntegrities` before it publishes the beacon. That CRC covers the primary partition and the active secondary partition, the same 128 KiB scan as the telecommand above, twice. While it runs, the RF thread is preempted, including in the middle of parsing, FRAM access, or a `SuspendUntil`. A command that arrives as a beacon period expires waits out that scan before its acceptance or completion frame continues.

EDU threads (priorities 210–260), `FileTransferThread` (800), and the watchdog (100) are below the RF thread. They run when the RF thread suspends, which is the remainder of the 300 ms pad and the waits inside the radio driver. They do not run instead of the report.

`ResumeFileTransferThread` and `ResumeEduProgramQueueThread` are called after the completion report has been sent. A copy whose source is the cubesat then calls `SuspendUntilNewTelemetryRecordIsAvailable`, so the RF thread does not listen again until the next beacon. That wait is after the completion frame.

## File and repository results

A file-attribute report and a repository-content summary are sent after the acceptance report, from the same handlers. The acceptance frame, its 100 ms `postTxDelay`, and the filesystem lookups below all finish before the first result frame is transmitted. Neither report reads file contents. The on-air time of each frame is the downlink airtime above, because every TM frame is one Reed-Solomon block plus convolutional coding, independent of how full the packet is.

The command that asks for the result is one uplink frame, so it occupies the channel for `255 * 8 / rxDataRate` (212.5 ms at 9600 bit/s, 1.700 s at 1200 bit/s). The acceptance and every result frame each occupy it for `520 * 8 / txDataRate` (433 ms at 9600 bit/s, 3.467 s at 1200 bit/s). RX and TX rates are independent. A file-attribute reply is one result frame. A repository listing is `ceil(nObjects / 4)` result frames, sent back to back:

| TX rate | File attributes (1 frame) | 20 names (5 frames) | 40 names (10 frames) |
| --- | ---: | ---: | ---: |
| 1200 bit/s | 3.467 s | 17.33 s | 34.67 s |
| 9600 bit/s | 433 ms | 2.167 s | 4.333 s |
| 19200 bit/s | 217 ms | 1.083 s | 2.167 s |

At the standard 9600 bit/s a listing of a few dozen names fits easily before the next beacon. 1200 bit/s is a border case: the same listing of 40 names takes 34.7 s and is cut off by the beacon, as described below. This airtime is on top of the 300 ms switch pad, the 100 ms pause after the acceptance frame, and the directory reads.

### File attributes

`Handle(ReportTheAttributesOfAFileRequest)` calls `fs::IsLocked` and `fs::FileSize`, then `SendAndWait` of one `FileAttributeReport`.

`IsLocked` is `lfs_stat` of `<path>.lock`. `FileSize` is `lfs_stat` of the file, and the size is `lfs_info.size`. The file body is not read, so the file's length does not change how long this takes. Both calls return immediately with `ErrorCode::io` when `flashIsWorking` is false, and a failed lookup sends `FailedCompletionOfExecutionVerificationReport` instead of the attribute report.

The path stored in the report is padded to `maxPathLength` (35 bytes) in the `FileAttributeReport` constructor. A short path and a long path produce the same packet size and the same one-frame airtime.

What does change the time before that frame:

- Two littlefs lookups on the external flash. The flash SPI is initialized at 48 MHz in `Sts1CobcSw/Flash/Flash.cpp`. The number of SPI transactions is whatever littlefs needs to walk to that directory entry.
- The same preemption as for any other report: the telemetry thread's partition CRC if a beacon period expires during the lookups, and the one-second SPI supervisor.

### Repository content

`Handle(SummaryReportTheContentOfARepositoryRequest)` walks the directory twice. `std::distance` to the end iterator counts the entries, then `MakeIterator` opens the directory again and the loop reads each entry with `lfs_dir_read`. Only the name and the file-or-directory flag are kept. File sizes are not read.

The count is stored in a `std::uint8_t`, so a directory with more than 255 entries wraps that count. `SetTxDataLength` uses the wrapped count to program how many frames the modem should send.

Each report packet holds `RepositoryContentSummaryReport::maxNObjectsPerPacket` entries. With the RS(255,223) frame (223-byte TM frame, 6-byte primary header, 6-byte space-packet header, 11-byte PUS header), a 35-byte path, a 1-byte object count, and a 36-byte `FileSystemObject` (1 byte type + 35 byte name), that is

```text
(211 - 11 - 35 - 1) / 36 = 4
```

objects per frame. Names are padded to 35 bytes, so the name length does not change the packing. The number of result frames is `ceil(nObjects / 4)`. Each one is a full downlink frame, sent with `SendAndContinue` as the entries are read, not after the whole directory is buffered.

After the last frame, `FinalizeTransmission` waits up to 1 s for the TX FIFO to drain. The frames are already on the air when the packet-sent interrupt arrives; the timeout is only the cap.

A new beacon cuts the listing short. Between packets the loop checks `telemetryRecordMailbox.IsFull()` and breaks without a failure report, so entries not yet packed are not sent. The telemetry thread can also preempt the directory walk itself and run both firmware CRCs before the next `lfs_dir_read` returns.

A failure while opening the directory sends one `FailedCompletionOfExecutionVerificationReport` and no summary frames. An error on a later entry breaks the loop the same way a beacon does: the frames already sent stay sent, and no failure report is added.

## CFDP retry timings

The intervals are compile-time constants in `Sts1CobcSw/RfProtocols/Configuration.hpp`. The comments above them say the values still need to be chosen and verified. They are not telecommands.

```182:189:Sts1CobcSw/RfProtocols/Configuration.hpp
inline constexpr auto positiveAckTimerInterval = 35 * s;
inline constexpr auto postiveAckTimerExpirationLimit = 15;
inline constexpr auto nakTimerInterval = 35 * s;
inline constexpr auto nakLimit = 15;
inline constexpr auto transactionInactivityLimit = 35 * 10 * s;

inline constexpr auto maxNNaksPerSequence = 4;
inline constexpr auto nakSequenceTimeout = 5 * s;
```

`transactionInactivityLimit` is 350 s. `FileTransferThread` uses each constant as follows.

| Timer | Value | Limit | Where it runs |
| --- | --- | --- | --- |
| Positive ACK | 35 s | 15 transmissions | `SendAndWaitForAck`, for an EOF PDU and for a Finished PDU, including a cancel EOF |
| NAK wait | 35 s | 15 unanswered requests | `ReceiveMissingFileData`, after the cubesat has sent a NAK |
| Inactivity | 350 s | one expiry cancels the transfer | `ReceiveInitialFileData` while file data is still arriving, and `SendMissingDataUntilFinished` while the cubesat waits for NAKs or a Finished PDU |
| NAK sequence | 5 s | 4 NAKs in one sequence | `SendMissingDataUntilFinished`, between NAKs of one sequence |

Positive ACK. The cubesat sends the EOF or Finished PDU, then waits `positiveAckTimerInterval` for the matching ACK. Each expiry sends it again. After `postiveAckTimerExpirationLimit` transmissions with no matching ACK, the procedure returns `positiveAckLimitReached`.

NAK wait. Once the initial file data and the EOF have been received, missing ranges are requested with a NAK PDU. `ReceiveMissingFileData` then waits `nakTimerInterval`. If file data arrives, the wait ends and a further NAK is sent for whatever is still missing; that round is not counted. If the wait expires with no file data, `RequestAndReceiveMissingDataUntilFinished` counts one unanswered request. `nakLimit` such rounds return `nakLimitReached`.

Inactivity. `ReceiveInitialFileData` restarts the 350 s timer on every received PDU, including one that fails to parse. No PDU for 350 s returns `inactivityDetected`. On the sending side, `SendMissingDataUntilFinished` uses the same 350 s while no NAK sequence is open. Expiry with an empty sequence returns `inactivityDetected`. Sending the repaired ranges, or finishing a NAK sequence, starts a fresh 350 s wait.

NAK sequence. NAKs that belong together are collected before the missing bytes are sent. A NAK is the last in its sequence when its end of scope equals the file size, or when `maxNNaksPerSequence` (4) NAKs have arrived. The missing data is sent immediately then, and the 350 s inactivity timer starts again. A NAK that is not the last one starts `nakSequenceTimeout` (5 s). If that 5 s expires, the ranges collected so far are sent anyway.

All four timers are `FileTransferTimer`s and are built with `fileTransferWindowEnd`. While the expiry lies outside the open window, `ExpirationTime()` is `endOfTime`, so the wait does not fire. `UpdateFileTransferWindowEnd` keeps the remaining duration and adds it back when a new window is open, so time outside the window is not counted. The window length itself comes from the enable-file-transfer command (`durationInS`), not from these constants.
