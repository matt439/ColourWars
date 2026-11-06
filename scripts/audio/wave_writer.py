"""The authoring projects' PCM16 WAV writer (NumPy is an authoring dependency)."""

from pathlib import Path
import struct

import numpy as np


def write_wave(path: Path, samples: np.ndarray, loop: bool, channels: int = 1,
               *, rate: int = 44100, loop_before_data: bool = False) -> np.ndarray:
    """Round then saturate PCM16; duplicate mono only when stereo is requested.

    Existing music masters put smpl before data, while foley/interface put it
    after data. Keep that container order so accepted renders retain their bytes.
    """
    samples = np.asarray(samples)
    if channels not in (1, 2) or samples.ndim not in (1, 2) or not len(samples):
        raise ValueError("expected nonempty mono or stereo samples")
    if not np.all(np.isfinite(samples)):
        raise ValueError("WAV samples must be finite")
    if samples.ndim == 2 and samples.shape[1] != channels:
        raise ValueError("WAV sample channels differ from the declared channels")
    pcm = np.clip(np.rint(samples * 32767), -32768, 32767).astype("<i2")
    if channels == 2 and pcm.ndim == 1:
        pcm = np.column_stack((pcm, pcm))
    fmt = struct.pack("<HHIIHH", 1, channels, rate, rate * 2 * channels, 2 * channels, 16)
    chunks = [(b"fmt ", fmt), (b"data", pcm.tobytes())]
    if loop:
        # RIFF smpl uses an inclusive end; infinite forward, full-file loop.
        smpl = struct.pack("<15I", 0, 0, round(1e9 / rate), 60, 0, 0, 0, 1, 0,
                           0, 0, 0, len(pcm) - 1, 0, 0)
        chunks.insert(1 if loop_before_data else len(chunks), (b"smpl", smpl))
    payload = b"WAVE" + b"".join(tag + struct.pack("<I", len(data)) + data +
                                  b"\0" * (len(data) & 1) for tag, data in chunks)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(b"RIFF" + struct.pack("<I", len(payload)) + payload)
    return pcm
