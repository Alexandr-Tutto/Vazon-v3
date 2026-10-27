# Vazon V3 Experiments

## EXP-2026-10-27-01: Long-term SHT31 relative-humidity comparison

- Status: **неоднозначно**
- Date: 2026-10-27
- Firmware/commit: installed firmware version not captured; repository worktree
  contains the `0.1.15` candidate, while `HEAD` is `48a20ae`
- Hardware: Vazon V3 device 02; both cabinet SHT31 sensors at addresses `0x44`
  and `0x45`; colocated household humidity indicators used as references
- Hypothesis: both installed SHT31 channels have a persistent negative
  relative-humidity bias under cabinet conditions
- Conditions and method: approximately three months of colocated observation
  in the cabinet over displayed humidity readings from about 55 to 85 %RH;
  comparison instruments are household devices of different types and are not
  traceably calibrated
- Observed result: both SHT31 readings were reported as consistently lower than
  the household references, by approximately 10% according to the field
  observation; temperatures were previously reported as closely aligned
- Existing implementation fact: firmware supports independent additive
  percentage-point offsets for both SHT31 channels; repository defaults and
  schema-v1 migration values are currently `+5.0` percentage points
- Unresolved measurement detail: it is not yet known whether the reported
  approximately 10% difference was observed while the retained `+5.0` offsets
  were active, nor whether “10%” means 10 percentage points or a relative 10%
  error
- Conclusion: the persistent common-direction difference justifies checking
  the retained offsets and performing a controlled offset adjustment, but does
  not yet establish a new exact calibration constant
- Follow-up: a total additive offset of `+12.0` percentage points for each
  SHT31 channel was selected for the next field-trial firmware. This is a
  provisional operational correction to be checked against subsequent field
  observations, not a traceable sensor calibration.
- Applicability limits: device 02, its present sensor placement, the observed
  55–85 %RH range, and the particular household reference instruments

## EXP-2026-10-27-02: First observation after installing firmware 0.1.15

- Status: **ambiguous**
- Date: 2026-10-27
- Firmware/commit: firmware version 0.1.15 installed on the device; repository
  worktree based on `HEAD` `48a20ae` with uncommitted field-offset changes
- Hardware: Vazon V3 device 02; upper SHT31 is represented as address `0x44`
  in the PWA and lower SHT31 as address `0x45`
- Hypothesis: the new total `+12.0` percentage-point correction is applied to
  both SHT31 channels and brings both displayed readings closer to their
  colocated references
- Conditions and method: immediate qualitative observation after OTA install;
  exact displayed readings, reference readings, raw readings, and published
  runtime setting values were not captured
- Observed result: the lower reading moved somewhat closer to the expected
  value. The upper reading appeared not to receive the correction and was now
  lower than the lower reading, although cabinet observations normally show
  higher humidity at the upper position.
- Implementation fact: firmware 0.1.15 adds the independently persisted
  offsets to their corresponding channels, and both migration/default values
  are `+12.0`. Equal additive offsets cannot by themselves reverse the ordering
  of two unclamped sensor readings.
- Conclusion: the observation does not yet identify a calibration value or a
  software fault. The active runtime offsets and simultaneous channel/reference
  readings must be captured before changing either correction.
- Follow-up: record `rh_offset_0x44_pct`, `rh_offset_0x45_pct`, upper/lower
  displayed RH, and both colocated reference RH values at the same time. If
  possible, also capture the UART `SENSOR_OK` raw readings.
- Applicability limits: first qualitative observation after installing 0.1.15;
  no controlled stabilization interval or numerical readings were recorded

### Follow-up

- The existing PWA climate calibration fields were found and operated as
  intended; no firmware or UI change was required.
- The two channel corrections were adjusted through the PWA. The final numeric
  offset values and simultaneous reference readings were not reported, so the
  calibration outcome remains unverified quantitatively.
