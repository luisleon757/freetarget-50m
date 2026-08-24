# Acoustic datasets

Store curated, labelled datasets here. Large unfiltered raw captures should remain outside Git unless intentionally selected.

Recommended naming:

```text
YYYYMMDD_sessionNN_eventNNN_<class>/
  metadata.json
  north.csv
  east.csv
  south.csv
  west.csv
```

Suggested classes: `valid_hit`, `neighbor_left`, `neighbor_right`, `frame_noise`, `face_strike`, `unknown`.
