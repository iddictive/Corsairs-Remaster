void ProcessDialogEvent()
{
    ref NPChar;
    aref Link, NextDiag;
    makeref(NPChar, CharacterRef);
    DeleteAttribute(&Dialog, "Links");
    makearef(Link, Dialog.Links);
    makearef(NextDiag, NPChar.Dialog);
    switch (Dialog.CurrentNode)
    {
        case "Freed":
            dialog.text = "Не думал, что снова увижу берег свободным. За рану ты уже ответил делом. За это — спасибо, капитан. До его корабля мне теперь нет дела.";
            link.l1 = "Иди своей дорогой, Диего."; link.l1.go = "FreedExit";
        break;
        case "FreedExit":
            NextDiag.CurrentNode = "Freed";
            DialogExit();
        break;
        case "Mercy":
            dialog.text = "Довольно. Шлюп твой, клинок мой у твоих ног. Ты победил честно. До выхода в море ты обещал принять мою сдачу — теперь я узнаю, чего стоит слово капитана.";
            link.l1 = "Живи. Отпущу тебя с командой; пленники выйдут свободными, а с Бермудами ты заключишь мир."; link.l1.go = "Spare";
            link.l2 = "За погибших ты ответишь клинком. Поднимай его."; link.l2.go = "Kill";
        break;
        case "Spare":
            DialogExit();
            GreatRecipe_AllianceMercy(NPChar, true);
        break;
        case "Kill":
            DialogExit();
            GreatRecipe_AllianceMercy(NPChar, false);
        break;
        case "First time":
            if (GreatRecipe_Allied())
            {
                dialog.text = "Капитан! Заходи. За корабли теперь отвечаю я, а Джекмен держит берег и договоры. Он ворчит, что я слишком громко сижу в его кресле. Друг пришёл — можно и пошуметь.";
                link.l1 = "Как держится наш мир?"; link.l1.go = "Peace";
                link.l2 = "Что с изготовлением жемчуга?"; link.l2.go = "Production";
            }
            else
            {
                dialog.text = "Ты взял мой корабль, одолел меня и отпустил. Клинок я тебе доверю. А на берегу меня ненавидят — и есть за что. Бермудцам нужен сильный флот, но они не станут ради него жить под моими пушками. Что ты готов предложить?";
                link.l1 = "Сначала освободи Диего и передай рецепт мастерам. Потом будем говорить с Лаки и Джекменом о мире."; link.l1.go = "Terms";
                if (CheckAttribute(pchar, "GreatRecipe.Alliance.Treaty"))
                { link.l2 = "Общий флот — под твоим началом, берег и договоры — у Джекмена. Я принимаю дружбу, ручаюсь за мир и не стану захватывать пиратские поселения."; link.l2.go = "Friendship"; }
                link.l8 = "Не стану ручаться за тебя. Пиратского союза не будет."; link.l8.go = "Decline";
            }
            link.l9 = "Увидимся, Борода."; link.l9.go = "Exit";
        break;
        case "Terms":
            dialog.text = GreatRecipe_AllianceAction(NPChar.id, "terms");
            link.l1 = "Увижу Диего на берегу. Жемчужину отнесу Лаки; затем поговорю с Джекменом."; link.l1.go = "Exit";
        break;
        case "Friendship":
            dialog.text = GreatRecipe_AllianceAction(NPChar.id, "friendship");
            link.l1 = "За этот мир отвечаем оба. До встречи, друг."; link.l1.go = "Exit";
        break;
        case "Decline":
            GreatRecipe_AllianceDecline();
            dialog.text = "За мою свободу я тебе обязан. Но за будущие дела ты не ручаешься. Понял. На этом разойдёмся.";
            if (CheckAttribute(pchar, "GreatRecipe.Alliance.PrisonerFreed")) dialog.text = dialog.text + " Диего свободен — и останется свободным.";
            if (CheckAttribute(pchar, "GreatRecipe.Alliance.RecipeShared")) dialog.text = dialog.text + " Рецепт уже передан, назад не заберу.";
            link.l1 = "Держи хотя бы это слово. Прощай."; link.l1.go = "Exit";
        break;
        case "Peace":
            dialog.text = "Пиратский флаг — ты свой, чужой — держим нейтралитет. Но дружба не отпишет чужие долги и войны. Я веду общие корабли; Джекмен ведёт поселение, договоры и прежние дела. Лаки брата не забыл. Если я снова ударю по этому берегу, одного кресла мне будет мало, чтобы спрятаться от них и от тебя.";
            link.l1 = "Мир держится на слове. Я своё держу."; link.l1.go = "Exit";
        break;
        case "Production":
            dialog.text = "Секрет не делает порох безопасным. Красную жемчужину отдашь Лаки — мастерам нужна неделя. Потом покупай у Паскаля: двадцать–тридцать бомб в день, не больше двух сотен в запасе. Корона не прибавляет им рук.";
            link.l1 = "Зайду к Лаки и Паскалю."; link.l1.go = "Exit";
        break;
        case "Exit":
            NextDiag.CurrentNode = "First time";
            DialogExit();
            if (CheckAttribute(pchar, "GreatRecipe.Alliance.SeatPending")) AddDialogExitQuestFunction("GreatRecipe_AllianceSeat");
        break;
    }
}
