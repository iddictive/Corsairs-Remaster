void ProcessDialogEvent()
{
    ref NPChar;
    aref Link, NextDiag;
    makeref(NPChar, CharacterRef);
    DeleteAttribute(&Dialog, "Links");
    makearef(Link, Dialog.Links);
    makearef(NextDiag, NPChar.Dialog);
    NextDiag.TempNode = "First time";
    switch (Dialog.CurrentNode)
    {
        case "First time":
            dialog.text = "Шлюпка на месте. Склеп — в скалах справа от воды. Я останусь здесь: назад пойдём, как только скажешь.";
            if (NPChar.id == "GreatRecipeShoreBeard")
                dialog.text = "Когда-то эти двое стояли со мной у пушек. Теперь сторожат мои долги. Если они снова поднимутся — прикрою. Книгу заберёшь ты.";
            if (NPChar.id == "GreatRecipeShoreBeard" && CheckAttribute(NPChar, "GreatRecipeShoreWithdrawn"))
                dialog.text = "До шлюпки доберусь. Рана дрянная — дальше сам. Не нравится берег, возвращаемся вместе: книгу ещё успеем забрать.";
            if (GetCharacterItem(pchar, "GreatRecipeCacheLedger") > 0)
                dialog.text = "Книга у тебя? Тогда довольно этой бухты. В шлюпку; капитан ждёт в каюте.";
            link.l1 = "Вернёмся на корабль."; link.l1.go = "Return";
            link.l2 = "Я ещё поищу на берегу."; link.l2.go = "Exit";
        break;

        case "Return":
            NextDiag.CurrentNode = NextDiag.TempNode;
            DialogExit();
            GreatRecipe_ShoreRetreat(false);
        break;

        case "Exit":
            NextDiag.CurrentNode = NextDiag.TempNode;
            DialogExit();
        break;
    }
}
