#!/usr/bin/env python3
"""Cap one firearm hit against the main character at 49% of maximum HP."""

from runtime_script_patch import FilePatch


PATH = "PROGRAM/Loc_ai/LAi_fightparams.c"

FILES = (
    FilePatch(
        PATH,
        "ec132e2de67b54fdc50480fa01fd12b2ab8eddca7612ee06a6691d024e079539",
        "1d96b2fec71cc6666457b95221ddea2da963551f89d1365922142678378442ef",
        (
            (
                '''\treturn dmg;
}

//Расчитать полученный опыт при попадании из пистолета''',
                '''\treturn dmg;
}

// Один выстрел не может снять главному герою больше 49% максимального здоровья.
float LAi_LimitMainCharacterFirearmDamage(aref enemy, float damage)
{
\tif(sti(enemy.index) != GetMainCharacterIndex()) return damage;
\tfloat maxDamage = MakeFloat(MakeInt(LAi_GetCharacterMaxHP(enemy) * 0.49));
\tif(damage > maxDamage) return maxDamage;
\treturn damage;
}

//Расчитать полученный опыт при попадании из пистолета''',
            ),
            (
                '''\t//Начисляем повреждение
\tfloat damage = LAi_GunCalcDamage(attack, enemy);

\t//Аттака своей группы''',
                '''\t//Начисляем повреждение
\tfloat damage = LAi_GunCalcDamage(attack, enemy);
\tdamage = LAi_LimitMainCharacterFirearmDamage(enemy, damage);

\t//Аттака своей группы''',
            ),
            (
                '''\t//AddCharacterExp(attack, 100*kDmg);
\t//Наносим повреждение
\tLAi_ApplyCharacterDamage(enemy, MakeInt((5 + rand(5))*kDmg));''',
                '''\t//AddCharacterExp(attack, 100*kDmg);
\t//Наносим повреждение
\tfloat damage = MakeFloat((5 + rand(5))*kDmg);
\tdamage = LAi_LimitMainCharacterFirearmDamage(enemy, damage);
\tLAi_ApplyCharacterDamage(enemy, MakeInt(damage + 0.5));''',
            ),
        ),
    ),
)
