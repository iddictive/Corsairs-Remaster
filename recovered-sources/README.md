# Recovered source variants

Historical engine, renderer, gameplay, build and diagnostic source recovered
from former local archives and local Git history. These are recovery inputs;
the active implementation remains on `main`.

`index.json` maps each original project-relative source path and blob to a
retained variant under `blobs/`, with its SHA-256 and file mode. Identical source
content uses Git's ordinary object storage. Already-public mainline source and
unchanged upstream history are referenced through their existing Git history.

Machine-specific repository, home and temporary paths are normalized using
the public export convention `/REQUIRED_EXTERNAL_INPUT`; affected entries list
their normalizations. Resolve those external inputs before reusing old helpers.
Existing source defects are preserved; this snapshot is not a build or gameplay
acceptance claim. Third-party attribution comments remain intact.

Private agent instructions, prompts, task diaries, raw recovery inventories,
player state, compiled outputs and private historical commit ancestry are
excluded. Only the public mainline is the parent of this source snapshot.
