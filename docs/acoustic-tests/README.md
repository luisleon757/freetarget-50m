# Acoustic tests

The 50 m project must be developed against real recordings, not only synthetic tests.

Minimum dataset classes:

- valid projectile event on our target
- neighboring shot from left
- neighboring shot from right
- simultaneous/near-simultaneous neighboring shots
- target-frame mechanical noise
- face/frame strike
- handling/vibration events
- quiet background

For each capture record:

- four raw waveforms
- four comparator timestamps
- sensor geometry
- ammunition and firearm when known
- lane position / neighboring-lane relation
- temperature
- accepted/rejected ground truth
- notes about physical shielding or microphone mounting

Do not tune rejection thresholds solely on one firing point or one session.
