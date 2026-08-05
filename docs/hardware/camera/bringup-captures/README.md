# Camera bring-up captures

This directory retains the small, representative image set needed to compare
the Rev C OV5640 investigations after disposable ESP-IDF build trees are
removed. The filenames identify the sensor module and configuration under
test. These are engineering evidence from a backlit room, not production image
quality or a calibrated corridor-lighting qualification.

The most useful pair for the exposure correction is:

- `qxga-alternate-auto-2x-before-aec-fix.jpg`: stale anti-flicker timing,
  885-line exposure at the raw `0x0020` automatic-gain ceiling.
- `qxga-alternate-auto-2x-aec60-fixed.jpg`: timing-derived 60 Hz anti-flicker
  configuration, 1,898-line exposure at the same gain ceiling.

The remaining files preserve the clean binned-mode and internal-colour-bar
controls, the original module's severe QXGA banding, and the alternate-module
manual/automatic comparisons.
