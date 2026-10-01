"""Exact-hash correction of Spain's prisoner model literals for Metal."""
import hashlib

PATH = "PROGRAM/dialogs/russian/Governor/spa_Governor.c"
BASE = "5aec8a5f5c4722d042b2a1e92a0175fedf03be932442294fad50ea0be4589daa"
UPDATED = "fa900d90d028a243376fb451f951865f804f3702f38483b979aea7bb032b02e7"


def prepare(data):
    revision = hashlib.sha256(data).hexdigest()
    if revision == UPDATED:
        return data
    if revision != BASE:
        raise RuntimeError("unrecognized Spain governor dialogue revision")
    for model in (b"merch_8", b"trader_6", b"usurer_5"):
        anchor = b"sTemp = " + model + b";"
        if data.count(anchor) != 1:
            raise RuntimeError("Spain governor model anchor mismatch")
        data = data.replace(anchor, b'sTemp = "' + model + b'";')
    if hashlib.sha256(data).hexdigest() != UPDATED:
        raise RuntimeError("unreviewed Spain governor dialogue output")
    return data
