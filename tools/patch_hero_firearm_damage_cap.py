#!/usr/bin/env python3
"""Cap one firearm hit against the main character at 49% of maximum HP."""

from runtime_script_patch import FilePatch


PATH = "PROGRAM/Loc_ai/LAi_fightparams.c"

FILES = (
    FilePatch(
        PATH,
        "5eed196c397d59475474b0f2c5964402167ba024dd33621cc10c785cd007f553",
        "0e5c671e97b2575586ee87379db027c0393689f1e60b242b5c9d215a7cf37f78",
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
