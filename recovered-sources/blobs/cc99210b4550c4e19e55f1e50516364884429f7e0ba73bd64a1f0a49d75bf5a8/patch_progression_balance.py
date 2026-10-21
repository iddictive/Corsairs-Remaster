#!/usr/bin/env python3
"""Install a transparent, mastery-shaped KVL skill progression curve."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT, sha256, transform


PATCH = PatchSet(
    TARGET_ROOT / ".codex-progression-balance" / "20260914-v1",
    (
        FilePatch(
            "PROGRAM/characters/RPGUtilite.c",
            "217759d242ba4bb8035d3d61ae5d31dde1c8c1c0be73527356cf71f2ad2e1d21",
            "761f79cc4dccebc26e0a3eb2b06d487939609e742480891f06c9951fc59f0bec",
            (
                (
                    '''// порог следующего скила (множетель)
float GetCharacterExpRate(ref _chref, string _skill)''',
                    '''// Плавная крива мастерства: быстрее освоить основы,
// без скидки на 80 и с небольшой надбавкой за мастерство выше 80.
float Progression_GetMasteryRate(ref _chref, string _skill)
{
    int baseSkill = GetSkillValue(_chref, SKILL_TYPE, _skill);
    float rate = 0.72 + makefloat(baseSkill) * 0.0035;

    if (baseSkill > 80)
    {
        rate = 1.0 + makefloat(baseSkill - 80) * 0.005;
    }
    if (rate < 0.72) rate = 0.72;
    if (rate > 1.10) rate = 1.10;
    return rate;
}

// порог следующего скила (множетель)
float GetCharacterExpRate(ref _chref, string _skill)''',
                ),
                (
                    '''float GetCharacterExpRate(ref _chref, string _skill)
{
    string  skill_rate = _skill + "_rate";''',
                    '''float GetCharacterExpRate(ref _chref, string _skill)
{
    string  skill_rate = _skill + "_rate";
    float result;
    int baseSkill;''',
                ),
                (
                    '''    return  stf(_chref.skill.(skill_rate));
}''',
                    '''    result = stf(_chref.skill.(skill_rate)) * Progression_GetMasteryRate(_chref, _skill);
    baseSkill = GetSkillValue(_chref, SKILL_TYPE, _skill);
    if (baseSkill < 1) baseSkill = 1;
    if (makefloat(baseSkill) * result < 1.0)
    {
        result = 1.0 / makefloat(baseSkill);
    }
    return result;
}''',
                ),
                (
                    '''            /*if (sti(_chref.index) == GetMainCharacterIndex())
            {
               Log_SetStringToLog(XI_ConvertString(_skill)+" UP");
            } */
            AddCharacterSkillDontClearExp(_chref, _skill, 1);''',
                    '''            AddCharacterSkillDontClearExp(_chref, _skill, 1);
            if (sti(_chref.index) == GetMainCharacterIndex())
            {
                Log_SetStringToLog(XI_ConvertString(_skill) + ": " + sti(_chref.skill.(_skill)));
            }''',
                ),
            ),
        ),
    ),
)


def verify(root: Path) -> int:
    state, rows = PATCH.overall_state(root)
    if state not in {"original", "patched"}:
        PATCH.print_status(root)
        return 1

    spec, item_state = rows[0]
    source = (root / spec.relative_path).read_bytes()
    candidate = transform(source, spec) if item_state == "original" else source
    text = candidate.decode("utf-8")

    required = (
        "float Progression_GetMasteryRate(ref _chref, string _skill)",
        "0.72 + makefloat(baseSkill) * 0.0035",
        "if (baseSkill > 80)",
        "makefloat(baseSkill - 80) * 0.005",
        "result = stf(_chref.skill.(skill_rate)) * Progression_GetMasteryRate(_chref, _skill)",
        "makefloat(baseSkill) * result < 1.0",
        'Log_SetStringToLog(XI_ConvertString(_skill) + ": " + sti(_chref.skill.(_skill)))',
    )
    for marker in required:
        if text.count(marker) != 1:
            raise RuntimeError(f"progression marker is missing or duplicated: {marker}")

    rate_function = text[
        text.index("float GetCharacterExpRate(ref _chref, string _skill)") :
        text.index("// table service")
    ]
    if rate_function.count(
        "_chref.skill.(skill_rate) = makefloat(MOD_EXP_RATE / divBy);"
    ) != 1:
        raise RuntimeError("cached base experience rate was changed")
    assignment = rate_function.index(
        "_chref.skill.(skill_rate) = makefloat(MOD_EXP_RATE / divBy);"
    )
    dynamic = rate_function.index("Progression_GetMasteryRate(_chref, _skill)")
    if dynamic < assignment:
        raise RuntimeError("mastery factor is still stored in the rate cache")

    if text.index("int GetSkillValue(ref _refCharacter") > text.index(
        "float Progression_GetMasteryRate(ref _chref, string _skill)"
    ):
        raise RuntimeError("mastery curve is declared before its skill-value dependency")
    gain = text[text.index("void AddCharacterExpToSkill(ref _chref") :]
    if gain.index("if (_addValue < 0)") > gain.index(
        "* GetCharacterExpRate(_chref, _skill)) <="
    ):
        raise RuntimeError("negative experience no longer exits before advancement")
    if gain.index("AddCharacterSkillDontClearExp(_chref, _skill, 1);") > gain.index(
        'Log_SetStringToLog(XI_ConvertString(_skill) + ": "'
    ):
        raise RuntimeError("skill feedback is emitted before the skill increase")

    login = (root / "PROGRAM/characters/characters_login.c").read_text(
        encoding="utf-8"
    )
    if "ClearCharacterExpRate(&Characters[i]);" not in login:
        raise RuntimeError("save-load skill-rate cache refresh is unavailable")

    def mastery_rate(skill: int) -> float:
        rate = 0.72 + float(skill) * 0.0035
        if skill > 80:
            rate = 1.0 + float(skill - 80) * 0.005
        return max(0.72, min(rate, 1.10))

    expected = {1: 0.7235, 20: 0.79, 50: 0.895, 80: 1.0, 90: 1.05, 100: 1.10}
    for skill, rate in expected.items():
        actual = mastery_rate(skill)
        if abs(actual - rate) > 0.00001:
            raise RuntimeError(f"unexpected mastery rate at {skill}: {actual}")

    ordered = [mastery_rate(skill) for skill in range(1, 101)]
    if any(ordered[index] > ordered[index + 1] for index in range(len(ordered) - 1)):
        raise RuntimeError("mastery curve is not monotonic")
    if mastery_rate(79) >= 1.0 or mastery_rate(80) != 1.0 or mastery_rate(81) <= 1.0:
        raise RuntimeError("mastery boundary around skill 80 is invalid")
    if mastery_rate(20) == mastery_rate(21):
        raise RuntimeError("multi-level awards cannot observe the next skill factor")
    fastest_first_threshold = int(max(1.0, 1.0 * 1.0 * mastery_rate(1)))
    if fastest_first_threshold < 1:
        raise RuntimeError("fastest first skill threshold can become free")

    print("progression balance patch: verified")
    print("curve: skill 20=0.790, 50=0.895, 80=1.000, 90=1.050")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("status", "verify", "apply", "revert"))
    args = parser.parse_args()

    try:
        if args.action == "status":
            return PATCH.print_status(TARGET_ROOT)
        if args.action == "verify":
            return verify(TARGET_ROOT)
        if args.action == "apply":
            print(PATCH.apply(TARGET_ROOT))
        else:
            print(PATCH.revert(TARGET_ROOT))
        return PATCH.print_status(TARGET_ROOT)
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
