"""Compose lamp model activation from the existing weather Lights contract."""

PATH = "PROGRAM/locations/locations_loader.c"


def prepare(data: bytes) -> bytes:
    newline = b"\r\n" if b"\r\n" in data else b"\n"
    text = data.replace(b"\r\n", b"\n")
    old = b'    res = SendMessage(loc, "lssll", MSG_LOCATION_ADD_MODEL, loc.(sat) + addition, tech, level, dynamicLightsOn);'
    new = b'''    // Lamp variants follow the source activation flag, not the later Night flag.
    bool lampModel = sat == "models.day.fonar" || sat == "models.night.fonar";
    string modelName = loc.(sat);
    if (lampModel)
    {
        if (Whr_IsLight() && CheckAttribute(loc, "models.night.fonar")) modelName = loc.models.night.fonar;
        if (!Whr_IsLight() && CheckAttribute(loc, "models.day.fonar")) modelName = loc.models.day.fonar;
    }
    res = SendMessage(loc, "lssll", MSG_LOCATION_ADD_MODEL, modelName + addition, tech, level, dynamicLightsOn);'''
    lamp_old = b'attr = sat + ".lamps";\n\tif(CheckAttribute(loc, attr) != 0)'
    lamp_new = b'attr = sat + ".lamps";\n\tif (lampModel) SendMessage(loc, "l", MSG_LOCATION_MODEL_LAMPS);\n\telse if(CheckAttribute(loc, attr) != 0)'
    if new in text and lamp_new in text:
        return data
    if text.count(old) != 1 or text.count(lamp_old) != 1:
        raise RuntimeError("Location lamp loader changed; no files written")
    return text.replace(old, new).replace(lamp_old, lamp_new).replace(b"\n", newline)
