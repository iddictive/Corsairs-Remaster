void ProcessDialogEvent()
{
    ref NPChar;
    aref Link, NextDiag;
    makeref(NPChar, CharacterRef);
    makearef(Link, Dialog.Links);
    makearef(NextDiag, NPChar.Dialog);
    DeleteAttribute(&Dialog, "Links");
    NextDiag.TempNode = "First time";
    if (GreatRecipe_Phase() != "captive")
    {
        dialog.text = "Нам больше не о чем говорить.";
        link.l1 = "Прощай.";
        link.l1.go = "Exit";
        return;
    }
    switch (Dialog.CurrentNode)
    {
        case "First time":
            if (NPChar.id == "GreatRecipeBosun")
            {
                dialog.text = "Капитан велел оставить тебя в живых. Ведро у двери: палуба сама себя не выдраит. И не вздумай клянчить у Бороды пощаду.";
                if (CheckAttribute(pchar, "GreatRecipe.Last")) dialog.text = pchar.GreatRecipe.Last;
                link.l1 = "Драить палубу до вечера."; link.l1.go = "Scrub";
                if (!CheckAttribute(pchar, "GreatRecipe.Helped"))
                { link.l2 = "Подменить раненого пленника на тяжёлой работе."; link.l2.go = "Help"; }
                if (!CheckAttribute(pchar, "GreatRecipe.Defied"))
                { link.l3 = "Испортить крепление орудия и не выдать товарищей."; link.l3.go = "Defy"; }
                link.l4 = "Сейчас передохну."; link.l4.go = "Exit";
            }
            else
            {
                dialog.text = "Боцман от тебя устал. Слуга из тебя плохой — может, капитан выйдет лучше? Получишь такой же шлюп, как мой, с людьми и ядрами поровну. Возьмёшь мой корабль — расскажу, как делается жемчуг. А до той поры рецепт мой.";
                link.l1 = "Согласен. Отдавай шлюп: выйдем один на один."; link.l1.go = "Duel";
                link.l2 = "Сначала закончу дела в трюме."; link.l2.go = "Exit";
            }
        break;
        case "Scrub":
            GreatRecipe_Chore("scrub");
            dialog.text = pchar.GreatRecipe.Last;
            link.l1 = "Дальше."; link.l1.go = "First time";
        break;
        case "Help":
            GreatRecipe_Chore("help");
            dialog.text = pchar.GreatRecipe.Last;
            link.l1 = "Дальше."; link.l1.go = "First time";
        break;
        case "Defy":
            GreatRecipe_Chore("defy");
            dialog.text = pchar.GreatRecipe.Last;
            link.l1 = "Дальше."; link.l1.go = "First time";
        break;
        case "Duel":
            DialogExit();
            AddDialogExitQuestFunction("GreatRecipe_LaunchDuel");
        break;
        case "Exit":
            NextDiag.CurrentNode = "First time";
            DialogExit();
        break;
    }
}
