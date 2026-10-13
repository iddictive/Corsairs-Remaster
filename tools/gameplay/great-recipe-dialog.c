void ProcessDialogEvent()
{
    ref NPChar;
    aref Link, NextDiag;
    makeref(NPChar, CharacterRef);
    DeleteAttribute(&Dialog, "Links");
    makearef(Link, Dialog.Links);
    makearef(NextDiag, NPChar.Dialog);
    NextDiag.TempNode = "First time";
    if (GreatRecipe_Phase() != "captive")
    {
        dialog.text = "Нам больше не о чем говорить.";
        link.l1 = "Прощай.";
        link.l1.go = "Exit";
        return;
    }
    if (Dialog.CurrentNode == "history" || Dialog.CurrentNode == "scar" || Dialog.CurrentNode == "mercy" || Dialog.CurrentNode == "crew" || Dialog.CurrentNode == "pearl_story")
    {
        string topic = Dialog.CurrentNode;
        if (topic == "pearl_story") topic = "pearl";
        dialog.text = GreatRecipe_Story(NPChar.id, topic);
        link.l1 = "Ещё один вопрос."; link.l1.go = "First time";
        link.l2 = "Мне надо подумать."; link.l2.go = "Exit";
        return;
    }
    if (Dialog.CurrentNode != "First time" && Dialog.CurrentNode != "Exit" &&
        Dialog.CurrentNode != "Duel" && Dialog.CurrentNode != "Fight" && Dialog.CurrentNode != "Pledge")
    {
        dialog.text = GreatRecipe_TaskAction(Dialog.CurrentNode);
        if (CheckAttribute(pchar, "GreatRecipe.Fight.Pending"))
        { link.l1 = "Тогда выйдем к орудиям. Клинки решат."; link.l1.go = "Fight"; }
        else { link.l1 = "Понял. Вернусь, когда сделаю дело."; link.l1.go = "Exit"; }
        return;
    }
    switch (Dialog.CurrentNode)
    {
        case "First time":
            string task = pchar.GreatRecipe.Task;
            if (NPChar.id == "GreatRecipeBosun")
            {
                dialog.text = "Капитан сохранил тебе жизнь. Хочешь ходить без цепи — заработай. Из кладовой исчезла бочка сухарей. Расспроси Тома в кубрике, Джека у пушек и Диего в трюме. Потом приходи с именем, а не с байкой.";
                if (task == "rations")
                {
                    if (CheckAttribute(pchar, "GreatRecipe.Clue.Cook") && CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner") && CheckAttribute(pchar, "GreatRecipe.Clue.Gunner") && CheckAttribute(pchar, "GreatRecipe.Clue.Alibi"))
                    { link.l1 = "Показания сходятся. Хочу обвинить помощника."; link.l1.go = "accuse"; }
                    if (CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner"))
                    { link.l2 = "Диего катил бочку. Виноват он."; link.l2.go = "betray"; }
                }
                if (task == "rigging") dialog.text = "Грот заклинило, и мы теряем ход. Джек у пушек знает снасти. Поговори с ним; нужное добро найдётся у Тома и Диего.";
                if (task == "medicine") dialog.text = "Диего едва стоит. Лекарь в кубрике, полотно в трюме, вода у Тома. Поможешь — может, услышишь то, чего он мне не сказал.";
                if (task == "training") dialog.text = "Джек требует два поединка у орудий. Не ради забавы: Ратли точит людей на драку, и кто-то должен удержать прислугу на палубе.";
                if (task == "powder") dialog.text = "Капитан обнаружил подмену в крюйт-камере. Поднимись к нему за накладной, возьми верные фитили у Диего и помоги Джеку собрать заряды.";
                if (task == "mutiny" || task == "mutiny_final")
                {
                    dialog.text = "Ратли поднимает мятеж. Кто-нибудь сегодня останется за бортом. Решай, с кем стоишь.";
                    link.l1 = "Сохраню корабль. Людей Ратли надо разоружить."; link.l1.go = "loyal";
                    link.l2 = "Борода держит нас в цепях. Я с мятежниками."; link.l2.go = "rebel";
                }
                if (task == "ready") dialog.text = "Своё ты сделал. Капитан ждёт в каюте; не заставляй его звать дважды.";
            }
            if (NPChar.id == "GreatRecipeCook")
            {
                dialog.text = "На моём камбузе чужими руками не шарить. Что тебе нужно?";
                link.l1 = "О сухарях, масле и воде. Боцман послал."; link.l1.go = "cook";
                link.l4 = "Как ты попал к Бороде?"; link.l4.go = "history";
            }
            if (NPChar.id == "GreatRecipePrisoner")
            {
                dialog.text = "Тише... На этом корабле и у бочек есть уши. Если пришёл бить — бей. Если слушать — закрой дверь.";
                link.l1 = "Расскажи, что видел. И что прячут в трюме."; link.l1.go = "prisoner";
                if (task == "medicine") { link.l2 = "Промою рану чистой водой и перевяжу полотном."; link.l2.go = "heal"; }
                link.l4 = "Что ты потерял на берегу?"; link.l4.go = "history";
            }
            if (NPChar.id == "GreatRecipeGunner")
            {
                dialog.text = "Отойди от дула. Эти ядра не любят дураков. Спрашивай, пока руки свободны.";
                link.l1 = "Что здесь произошло?"; link.l1.go = "gunner";
                if (task == "rigging")
                {
                    link.l2 = "Сначала снять натяжение. Затем заменить заклинившую ось."; link.l2.go = "repair";
                    link.l3 = "Перерубить шкот: блок сам освободится."; link.l3.go = "rigging_wrong";
                }
                if (task == "training") { link.l2 = "Готов. Выпускай первого, потом второго."; link.l2.go = "drill"; }
                if (task == "powder")
                {
                    link.l2 = "Собрать по накладной, с красным фитилём."; link.l2.go = "powder";
                    link.l3 = "Взять белый фитиль. Быстрее рванёт — больше толку."; link.l3.go = "wrong_fuse";
                }
                link.l4 = "Почему ты всё ещё служишь ему?"; link.l4.go = "history";
            }
            if (NPChar.id == "GreatRecipeMate")
            {
                dialog.text = "Пленникам полагается работать, а не вынюхивать. Держись подальше от моих людей.";
                if ((task == "rations" && CheckAttribute(pchar, "GreatRecipe.Clue.Cook") && CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner") && CheckAttribute(pchar, "GreatRecipe.Clue.Gunner") && CheckAttribute(pchar, "GreatRecipe.Clue.Alibi")) || (task == "rigging" && CheckAttribute(pchar, "GreatRecipe.Betrayed") && !CheckAttribute(pchar, "GreatRecipe.Reconciled")))
                { link.l1 = "Ключ от кладовой был у тебя. И синие сапоги тоже твои."; link.l1.go = "accuse"; }
                if (task == "mutiny") { link.l1 = "Ты обещал свободу. Я с тобой."; link.l1.go = "rebel"; }
                link.l4 = "Чего хотят твои люди?"; link.l4.go = "history";
            }
            if (NPChar.id == "GreatRecipeSurgeon")
            {
                dialog.text = "Руки вымой. На корабле хватает дыр без твоей помощи.";
                link.l1 = "Ты видел Диего? Где он был во время кражи?"; link.l1.go = "surgeon";
                if (task == "medicine") link.l1 = "Что нужно для раны Диего?";
                link.l4 = "Скольких людей на этом корабле ты спас?"; link.l4.go = "history";
            }
            if (NPChar.id == "GreatRecipeDreamBeard")
            {
                dialog.text = "Боцман от тебя устал. Слуга из тебя плохой — может, капитан выйдет лучше? Получишь такой же шлюп, как мой, с людьми и ядрами поровну. Возьмёшь мой корабль — расскажу, как делается жемчуг. А до той поры рецепт мой.";
                if (task == "ready")
                {
                    if (sti(pchar.GreatRecipe.Rebel)) dialog.text = "Помог моим врагам, а теперь стоишь передо мной? Смелости тебе хватает. Получишь такой же шлюп, как мой. Одолеешь меня — заберёшь рецепт; не одолеешь — я решу твою участь.";
                    link.l1 = "Согласен. Отдавай шлюп: выйдем один на один."; link.l1.go = "Duel";
                    if (GreatRecipe_AllianceEligible() && GreatRecipe_AllianceStage() == "")
                    { link.l8 = "Мы оба доказали, что держим клинок. Если сдашься в дуэли — я сохраню тебе жизнь. Но ты оставишь Бермуды в покое."; link.l8.go = "Pledge"; }
                }
                else if (task == "powder" || task == "mutiny_final")
                { dialog.text = "Подменённые заряды могли перебить мою команду. Хочешь заслужить доверие — сверяй накладную."; link.l1 = "Покажи накладную на жемчуг."; link.l1.go = "captain"; }
                else dialog.text = "За рецепт работают кровью, а не просьбами. Боцман найдёт тебе дело. Закончи его — тогда поговорим.";
                if (task == "mutiny_final") { dialog.text = "Твою драку я слышал отсюда. Ну, кого теперь назначишь хозяином корабля?"; link.l1 = "Поговорим о тех, кто ещё держит палубу."; }
                link.l2 = "Кто оставил тебе этот шрам и выбил глаз?"; link.l2.go = "scar";
                link.l3 = "Зачем ты мучаешь людей у берега?"; link.l3.go = "history";
                link.l4 = "Почему меня не убили вместе с кораблём?"; link.l4.go = "mercy";
                link.l5 = "Что твоя команда думает о тебе?"; link.l5.go = "crew";
                link.l6 = "Жемчуг — порох, скорлупа или сам разрыв?"; link.l6.go = "pearl_story";
                link.l7 = "Скрестим клинки на палубе. До сдачи."; link.l7.go = "captain_practice";
            }
            link.l9 = "Вернусь к работе."; link.l9.go = "Exit";
        break;
        case "Fight":
            DialogExit();
            AddDialogExitQuestFunction("GreatRecipe_StartFight");
        break;
        case "Duel":
            DialogExit();
            AddDialogExitQuestFunction("GreatRecipe_LaunchDuel");
        break;
        case "Pledge":
            if (GreatRecipe_AlliancePledge()) dialog.text = "Мой корабль ещё придётся взять, а клинок — выбить из руки. Сумеешь — приму твоё слово. Но мир с берегом за людей не подпишешь: с ними придётся говорить самому.";
            else dialog.text = "Сначала докажи делом, что стоишь своего слова. Дружба не покупается обещанием перед боем.";
            link.l1 = "Выйдем на равных. Дальше решит бой."; link.l1.go = "Duel";
            link.l2 = "Прежде закончу разговор с командой."; link.l2.go = "Exit";
        break;
        case "Exit":
            NextDiag.CurrentNode = "First time";
            DialogExit();
        break;
    }
}
