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
    if (NPChar.id == "GreatRecipeDreamBeard" && GreatRecipe_IntroPending() &&
        (!CheckAttribute(pchar, "GreatRecipe.Arrival.Stage") || pchar.GreatRecipe.Arrival.Stage == "captain") && Dialog.CurrentNode != "Exit")
    {
        if (Dialog.CurrentNode == "ArrivalBeardLeave")
        {
            pchar.GreatRecipe.Arrival.Stage = "leaving";
            DeleteAttribute(pchar, "GreatRecipe.Arrival.Resume");
            DialogExit();
            AddDialogExitQuestFunction("GreatRecipe_CaptainLeaves");
            return;
        }
        pchar.GreatRecipe.Arrival.Stage = "captain";
        pchar.GreatRecipe.Arrival.Resume = Dialog.CurrentNode;
        NextDiag.TempNode = Dialog.CurrentNode;
        dialog.text = "Проснулся? Мои люди взяли тебя живым. Не потому, что не умеют убивать. Я приказал. " + GreatRecipe_ArrivalTaunt();
        if (CheckAttribute(pchar, "GreatRecipe.Arrival.Cause") && pchar.GreatRecipe.Arrival.Cause == "sinking") dialog.text = "Моя шлюпка выловила тебя после того, как твой флагман ушёл под воду. Не принимай это за милосердие. " + GreatRecipe_ArrivalTaunt();
        if (CheckAttribute(pchar, "GreatRecipe.Arrival.Cause") && pchar.GreatRecipe.Arrival.Cause == "boarding") dialog.text = "На палубе тебя разоружили. Добить было проще, но я велел спустить тебя сюда. " + GreatRecipe_ArrivalTaunt();
        if (Dialog.CurrentNode == "ArrivalBeardWhy") dialog.text = "Убить успею. Сперва хочу посмотреть, способен ли ты хоть на что-нибудь без корабля за спиной. Морган отвечает за тебя. За борт шагнёшь — пристрелят; в мою каюту придёшь, когда позову.";
        link.l1 = "Зачем оставил меня в живых?"; link.l1.go = "ArrivalBeardWhy";
        if (Dialog.CurrentNode == "ArrivalBeardWhy") { link.l1 = "Посмотрим, кто ещё пожалеет об этой встрече."; link.l1.go = "ArrivalBeardLeave"; }
        link.l2 = "Скажи Моргану: я его выслушаю."; link.l2.go = "ArrivalBeardLeave";
        link.l9 = "Мне нужно перевести дух."; link.l9.go = "Exit";
        return;
    }
    if (NPChar.id == "GreatRecipeBosun" && GreatRecipe_IntroPending() &&
        (!CheckAttribute(pchar, "GreatRecipe.Arrival.Stage") || pchar.GreatRecipe.Arrival.Stage != "bosun") && Dialog.CurrentNode != "Exit")
    {
        dialog.text = "Капитан ещё не закончил. Сначала выслушай его.";
        link.l1 = "Ладно."; link.l1.go = "Exit";
        return;
    }
    if (NPChar.id == "GreatRecipeBosun" && GreatRecipe_IntroPending() &&
        (Dialog.CurrentNode == "First time" || Dialog.CurrentNode == "Arrival" || Dialog.CurrentNode == "ArrivalFleet" || Dialog.CurrentNode == "ArrivalMotive" || Dialog.CurrentNode == "ArrivalTask"))
    {
        pchar.GreatRecipe.Arrival.Resume = Dialog.CurrentNode;
        NextDiag.TempNode = Dialog.CurrentNode;
        dialog.text = "Ты в трюме Красной Бороды. Капитан велел взять тебя живым; я отвечаю, чтоб приказ исполнили. Сиди ровно — после такой драки кости не сразу слушаются.";
        if (CheckAttribute(pchar, "GreatRecipe.Arrival.Cause") && pchar.GreatRecipe.Arrival.Cause == "boarding") dialog.text = "На палубе тебя разоружили. Борода велел не добивать — матросы стащили тебя в трюм. Сейчас ты на его корабле, и за твою голову отвечаю я.";
        if (CheckAttribute(pchar, "GreatRecipe.Arrival.Cause") && pchar.GreatRecipe.Arrival.Cause == "sinking") dialog.text = "Твой флагман ушёл под воду. Наша шлюпка выловила тебя, а Борода велел оставить в живых. Сейчас ты в его трюме. Не вскакивай: наглотался воды.";
        dialog.text = dialog.text + " Он хочет увидеть, чего стоит капитан без своих пушек. По кораблю пока ходишь под моим словом; в каюту без приглашения не пустят.";
        if (Dialog.CurrentNode == "ArrivalFleet") dialog.text = "За остальные корабли не скажу. В воронке я различал огни, потом и те пропали. Борода смотрел с верхней палубы — у него и спрашивай, когда примет.";
        if (Dialog.CurrentNode == "ArrivalMotive") dialog.text = "Ты привёл сюда эскадру, а не лодку с контрабандой. Борода хочет посмотреть, что стоит капитан без своих пушек. Мёртвый ему на это не ответит. Пока он решает, по кораблю ходишь под моим словом; за борт и в его каюту — без приглашения не суйся.";
        if (Dialog.CurrentNode == "ArrivalTask")
        {
            dialog.text = "Из кладовой пропала бочка сухарей. Ратли сваливает кражу на Диего, но Том и Джек рассказывают разное. Разберись, пока команда не устроила самосуд. Принесёшь правду вместо удобной лжи — я добьюсь, чтобы капитан тебя выслушал.";
            link.l1 = "Поговорю с ними. Самосудом правду не найдёшь."; link.l1.go = "ArrivalComplete";
        }
        else
        {
            link.l1 = "Что стало с моей эскадрой?"; link.l1.go = "ArrivalFleet";
            link.l2 = "Зачем он оставил меня в живых?"; link.l2.go = "ArrivalMotive";
            link.l3 = "Хочу говорить с Бородой."; link.l3.go = "ArrivalTask";
        }
        link.l9 = "Мне нужно прийти в себя."; link.l9.go = "Exit";
        return;
    }
    if (NPChar.id == "GreatRecipeDreamBeard" && !GreatRecipe_AudienceAllowed() && Dialog.CurrentNode != "Exit")
    {
        dialog.text = "Морган ещё не поручился за тебя. Вернись к нему на палубу; приглашение в мою каюту он не обещал за красивые слова.";
        link.l1 = "Поговорю с ним."; link.l1.go = "Exit";
        return;
    }
    if (Dialog.CurrentNode == "ArrivalComplete")
    {
        pchar.GreatRecipe.Arrival.Complete = true;
        pchar.GreatRecipe.Arrival.Stage = "done";
        DeleteAttribute(pchar, "GreatRecipe.Arrival.Resume");
        dialog.text = "Том на камбузе в кубрике, Джек у орудий, Диего здесь, в трюме. Я буду на палубе. И не верь Ратли только потому, что он говорит громче всех.";
        link.l1 = "Найду, кто видел кражу."; link.l1.go = "Exit";
        GreatRecipe_Journal("Я в плену у Красной Бороды. Судьба остальных кораблей неизвестна. Боцман обещал добиться разговора с капитаном, если я разберусь с обвинением против Диего.", false);
        LAi_SetStayType(NPChar);
        ChangeCharacterAddressGroup(NPChar, "GreatRecipeDeck", "rld", "loc0");
        return;
    }
    if (CheckAttribute(pchar, "GreatRecipe.Fight.Active"))
    {
        dialog.text = "На палубе ещё обнажены клинки. Договорим, когда разнимут дерущихся.";
        link.l1 = "До встречи."; link.l1.go = "Exit";
        if (Dialog.CurrentNode != "Exit") return;
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
        Dialog.CurrentNode != "Duel" && Dialog.CurrentNode != "Fight" && Dialog.CurrentNode != "Pledge" &&
        Dialog.CurrentNode != "ShoreSolo" && Dialog.CurrentNode != "ShoreHelp" && Dialog.CurrentNode != "ShoreHandOver")
    {
        dialog.text = GreatRecipe_TaskAction(Dialog.CurrentNode);
        if (CheckAttribute(pchar, "GreatRecipe.Fight.Pending"))
        { link.l1 = "Тогда выйдем к орудиям. Клинки решат."; link.l1.go = "Fight"; }
        else { link.l1 = "Понял."; link.l1.go = "Exit"; }
        return;
    }
    switch (Dialog.CurrentNode)
    {
        case "First time":
            string task = pchar.GreatRecipe.Task;
            if (NPChar.id == "GreatRecipeBosun")
            {
                dialog.text = "На палубе мне нужны живые люди, а не ещё одна драка. Что узнал?";
                if (task == "rations")
                {
                    dialog.text = "Ратли требует запереть Диего за кражу сухарей. Пока не услышал свидетелей, к нему не полезу. Том отвечает за кладовую, Джек стоял у люка; Диего тоже должен сказать своё.";
                    if (CheckAttribute(pchar, "GreatRecipe.Clue.Cook") && CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner") && CheckAttribute(pchar, "GreatRecipe.Clue.Gunner") && !CheckAttribute(pchar, "GreatRecipe.Clue.Alibi"))
                    {
                        dialog.text = "Ключ был у Ратли, Джек видел его ночью с бочкой. Но Диего тоже её катал. Нужен человек, который подтвердит время: доктор Вейн спускался к нему после ужина. Найди врача в кубрике и принеси его показания.";
                        link.l1 = "Проверю время кражи у врача."; link.l1.go = "Exit";
                    }
                    if (CheckAttribute(pchar, "GreatRecipe.Clue.Cook") && CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner") && CheckAttribute(pchar, "GreatRecipe.Clue.Gunner") && CheckAttribute(pchar, "GreatRecipe.Clue.Alibi"))
                    { link.l1 = "Показания сходятся. Хочу обвинить помощника."; link.l1.go = "accuse"; }
                    if (CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner"))
                    { link.l2 = "Диего катил бочку. Обвини его — с остальным разберётесь сами."; link.l2.go = "betray"; }
                }
                if (task == "rigging")
                {
                    dialog.text = "Грот заклинило. Джек подозревает, что это не износ: слишком вовремя мы потеряли ход. Послушай его, прежде чем лезть к натянутой снасти.";
                    if (GreatRecipe_EarnAudience()) dialog.text = "С Ратли ты разобрался. Я передал капитану твоё слово — в каюту пустят. А Джек нашёл новую пакость в снастях: грот заклинило.";
                    if (CheckAttribute(pchar, "GreatRecipe.Betrayed") && !CheckAttribute(pchar, "GreatRecipe.Reconciled")) dialog.text = "Диего после твоих слов заперт и никому не поможет. Хочешь, чтоб я отвёл тебя к капитану, — сначала заставь Ратли ответить за свою ложь.";
                }
                if (task == "medicine") dialog.text = "Диего едва стоит. Лекарь в кубрике, полотно в трюме, вода у Тома. Поможешь — может, услышишь то, чего он мне не сказал.";
                if (task == "training") dialog.text = "Джек требует два поединка у орудий. Не ради забавы: Ратли точит людей на драку, и кто-то должен удержать прислугу на палубе.";
                if (task == "powder") dialog.text = "Капитан обнаружил подмену в крюйт-камере. Поднимись к нему за накладной. Диего скажет, где в кубрике спрятаны сухие фитили; затем найди орудийный пост у Джека.";
                if (task == "shore") dialog.text = "Капитан хочет послать тебя за судовой книгой. Своим он её не доверил — после истории с Ратли я его понимаю. Условия высадки спроси у него в каюте.";
                if (task == "mutiny" || task == "mutiny_final")
                {
                    dialog.text = "Ратли поднимает мятеж. Кто-нибудь сегодня останется за бортом. Решай, с кем стоишь.";
                    link.l1 = "Сохраню корабль. Людей Ратли надо разоружить."; link.l1.go = "loyal";
                    link.l2 = "Борода держит нас в цепях. Я с мятежниками."; link.l2.go = "rebel";
                    if (CheckAttribute(pchar, "GreatRecipe.MutinySide") || task == "mutiny_final")
                    {
                        if (sti(pchar.GreatRecipe.Rebel))
                        { DeleteAttribute(Link, "l1"); dialog.text = "Ты пошёл с Ратли. Его люди ждут: оставшаяся вахта Бороды ещё держит палубу."; }
                        else
                        { DeleteAttribute(Link, "l2"); dialog.text = "Ты встал против Ратли. Его люди ещё держат палубу; доведи начатое до конца."; }
                    }
                    if (task == "mutiny_final" && !CheckAttribute(pchar, "GreatRecipe.Clue.CaptainOrder"))
                    { DeleteAttribute(Link, "l1"); DeleteAttribute(Link, "l2"); dialog.text = "Первых разоружили, но нос ещё держат. Борода требует объяснений в каюте. Потом решим, как пройти к оставшимся."; }
                }
                if (task == "ready") dialog.text = "Своё ты сделал. Капитан ждёт в каюте; не заставляй его звать дважды.";
            }
            if (NPChar.id == "GreatRecipeCook")
            {
                dialog.text = "Я кормлю этот корабль. Пока люди сыты, они хотя бы слушают друг друга. Что тебе нужно?";
                if (task == "rations") { dialog.text = "Сухари пропали, а Ратли уже нашёл виноватого. Диего мою кладовую после ужина не открывал — это я могу сказать точно."; link.l1 = "Что пропало и у кого был ключ?"; link.l1.go = "cook"; }
                if (task == "rigging" && CheckAttribute(pchar, "GreatRecipe.Clue.Block")) { link.l1 = "Джеку нужно масло для блока. Где взять?"; link.l1.go = "cook"; }
                if (task == "medicine" && CheckAttribute(pchar, "GreatRecipe.Clue.Medicine")) { link.l1 = "Лекарь просит кипячёную воду для Диего."; link.l1.go = "cook"; }
                link.l4 = "Как ты попал к Бороде?"; link.l4.go = "history";
            }
            if (NPChar.id == "GreatRecipePrisoner")
            {
                dialog.text = "Тише... На этом корабле и у бочек есть уши. Если пришёл бить — бей. Если слушать — закрой дверь.";
                if (task == "rations") { link.l1 = "Тебя обвиняют в краже сухарей. Где ты был после ужина?"; link.l1.go = "prisoner"; }
                if (task == "rigging" && CheckAttribute(pchar, "GreatRecipe.Clue.Block")) { link.l1 = "Ты работал со снастями. Есть дубовый нагель?"; link.l1.go = "prisoner"; }
                if (task == "medicine")
                {
                    dialog.text = "Повязка прилипла к ране. Вейн сказал, что надо промыть... Только не ромом. Мне и так огня хватает.";
                    if (CheckAttribute(pchar, "GreatRecipe.Clue.Medicine")) { link.l1 = "Лекарь велел найти чистое полотно. У тебя осталось?"; link.l1.go = "prisoner"; }
                    link.l2 = "Попробую промыть и перевязать рану."; link.l2.go = "heal";
                }
                if (task == "powder")
                {
                    dialog.text = "Рука ещё ноет, но жар отпустил. Про Ратли я не выдумывал: он возился у пороха и заменил фитили.";
                    if (CheckAttribute(pchar, "GreatRecipe.Clue.Manifest")) { link.l1 = "Капитан подтвердил красную нить. Где сухие фитили?"; link.l1.go = "prisoner"; }
                }
                link.l4 = "Что ты потерял на берегу?"; link.l4.go = "history";
            }
            if (NPChar.id == "GreatRecipeGunner")
            {
                dialog.text = "Стой сбоку от дула. Моим людям хватает опасности и без чужих капитанов на проходе.";
                if (task == "rations") { dialog.text = "Я стоял у люка, когда исчезли сухари. Том запер кладовую после ужина — потом кто-то снова её открыл."; link.l1 = "Кого ты видел у люка после ужина?"; link.l1.go = "gunner"; }
                if (task == "rigging")
                {
                    dialog.text = "Грот не идёт за рулём. Прежде чем трогать шкот, выясни, что его держит. Под ним работают двое моих.";
                    link.l1 = "Что заклинило в снасти?"; link.l1.go = "gunner";
                    if (CheckAttribute(pchar, "GreatRecipe.Clue.Block"))
                    {
                        dialog.text = "Блок заклинен железом. Натяжение надо снять, ось смазать и заменить деревянным нагелем. Как собираешься подойти к работе?";
                        link.l2 = "Сниму натяжение и поставлю нагель с маслом."; link.l2.go = "repair";
                        if (!CheckAttribute(pchar, "GreatRecipe.Lesson.RiggingDone")) { link.l3 = "Перерублю шкот. Некогда возиться с блоком."; link.l3.go = "rigging_wrong"; }
                        else dialog.text = "Мы уже скрестили клинки из-за этого шкота. Я своё сказал: людям под ним нужна целая снасть. Блок всё ещё ждёт нагель и масло.";
                    }
                }
                if (task == "shore") dialog.text = "Пост готов, фитили сухие. Борода хочет говорить с тобой о береговой высадке. Моим людям теперь есть чем встретить чужой борт.";
                if (task == "mutiny" || task == "mutiny_final") dialog.text = "Заряды мы уберегли, теперь Ратли взялся за людей. У люка уже тянут клинки. Кого решил прикрыть?";
                if (task == "ready") dialog.text = "Палубу удержали. Два тренировочных поединка я ещё предложу, если ты их не закончил. Но капитан уже ждёт; задерживать тебя приказа нет.";
                if (GreatRecipe_DrillAllowed())
                {
                    if (sti(pchar.GreatRecipe.TrainingWins) == 0) { dialog.text = "Снасть ходит. Если хочешь прикрыть прислугу у пушек, попробуй себя против моего быстрого фехтовальщика. До сдачи; это не приказ."; link.l6 = "Хочу попробовать. Зови первого."; }
                    else { dialog.text = "Первого ты остановил. Второй рубит тяжёлым клинком и не отступает после выпада. Попробуешь удержать и его?"; link.l6 = "Готов ко второму поединку."; }
                    link.l6.go = "drill";
                }
                if (task == "powder")
                {
                    dialog.text = "Диего говорит о подмене фитилей. Я проверю пост, но сперва сверю капитанскую накладную — чужими пальцами ошибку не оплатишь.";
                    if (CheckAttribute(pchar, "GreatRecipe.Clue.Manifest"))
                    {
                        dialog.text = "В накладной сухой фитиль с красной нитью: заряд должен лопнуть у врага, а не у нашего борта. Что поставишь на пост?";
                        link.l2 = "Сухой красный фитиль, как в накладной."; link.l2.go = "powder";
                        if (!CheckAttribute(pchar, "GreatRecipe.Lesson.PowderDone")) { link.l3 = "Белый: разрыв будет быстрее."; link.l3.go = "wrong_fuse"; }
                        else dialog.text = "Из-за белого фитиля мы уже схватились. Он остался у меня; сухой красный поставь на орудийный пост по накладной.";
                    }
                    if (GreatRecipe_DrillAllowed())
                    {
                        if (sti(pchar.GreatRecipe.TrainingWins) == 0) dialog.text = dialog.text + " Если хочешь потренироваться отдельно, мой быстрый фехтовальщик свободен. До сдачи; это не приказ.";
                        else dialog.text = dialog.text + " Первого ты остановил. Второй рубит тяжёлым клинком и не отступает после выпада. Попробуешь удержать и его?";
                    }
                }
                link.l4 = "Почему ты всё ещё служишь ему?"; link.l4.go = "history";
            }
            if (NPChar.id == "GreatRecipeMate")
            {
                dialog.text = "Пленникам полагается работать, а не вынюхивать. Держись подальше от моих людей.";
                if ((task == "rations" && CheckAttribute(pchar, "GreatRecipe.Clue.Cook") && CheckAttribute(pchar, "GreatRecipe.Clue.Prisoner") && CheckAttribute(pchar, "GreatRecipe.Clue.Gunner") && CheckAttribute(pchar, "GreatRecipe.Clue.Alibi")) || (task == "rigging" && CheckAttribute(pchar, "GreatRecipe.Betrayed") && !CheckAttribute(pchar, "GreatRecipe.Reconciled")))
                { link.l1 = "Ключ от кладовой был у тебя. И синие сапоги тоже твои."; link.l1.go = "accuse"; }
                if (task == "mutiny" && (!CheckAttribute(pchar, "GreatRecipe.MutinySide") || sti(pchar.GreatRecipe.Rebel)))
                { link.l1 = "Ты обещал свободу. Я с тобой."; link.l1.go = "rebel"; }
                link.l4 = "Чего хотят твои люди?"; link.l4.go = "history";
            }
            if (NPChar.id == "GreatRecipeSurgeon")
            {
                dialog.text = "Руки вымой. На корабле хватает дыр без твоей помощи.";
                if (task == "rations") { dialog.text = "Если пришёл искать вора, сперва уточни время. Рану Диего я видел своими глазами, как и цепь у его койки."; link.l1 = "Где был Диего, когда исчезли сухари?"; link.l1.go = "surgeon"; }
                if (task == "medicine") { dialog.text = "Диего скрывает жар, чтоб его не бросили за борт. Рану надо промыть и закрыть чистым полотном. Возьмёшься?"; link.l1 = "Скажи, чем промыть и перевязать."; link.l1.go = "surgeon"; }
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
                else if (task == "shore")
                {
                    dialog.text = "До Ратли у меня были другие люди. Их имена и доли остались в судовой книге, которую я спрятал на берегу. В эту луну проход открыт. Ключ у восточных скал; книгу принеси мне. За работу рассчитаемся, когда мастера доведут рецепт до первой партии.";
                    link.l1 = "Возьму шлюпку и найду твою книгу."; link.l1.go = "ShoreSolo";
                    if (GreatRecipe_ShoreHelpAllowed()) { link.l8 = "Идём вместе. Свои долги лучше увидеть самому."; link.l8.go = "ShoreHelp"; }
                    if (GetCharacterItem(pchar, "GreatRecipeCacheLedger") > 0) { link.l1 = "Вот судовая книга из тайника."; link.l1.go = "ShoreHandOver"; }
                }
                else dialog.text = "Морган поручился за тебя. Своих людей здесь нет, а чужих заставил тебя слушать — это мне интересно. О твоей эскадре поговорим после корабельных дел. Пока спрашивай; второго приглашения я не обещал.";
                if (task == "mutiny_final") { dialog.text = "Твою драку я слышал отсюда. Ну, кого теперь назначишь хозяином корабля?"; link.l1 = "Поговорим о тех, кто ещё держит палубу."; }
                link.l2 = "Кто оставил тебе этот шрам и выбил глаз?"; link.l2.go = "scar";
                link.l3 = "Зачем ты мучаешь людей у берега?"; link.l3.go = "history";
                link.l4 = "Почему меня не убили вместе с кораблём?"; link.l4.go = "mercy";
                link.l5 = "Что твоя команда думает о тебе?"; link.l5.go = "crew";
                link.l6 = "Жемчуг — порох, скорлупа или сам разрыв?"; link.l6.go = "pearl_story";
                link.l7 = "Скрестим клинки на палубе. До сдачи."; link.l7.go = "captain_practice";
            }
            link.l9 = "До встречи."; link.l9.go = "Exit";
        break;
        case "Fight":
            DialogExit();
            AddDialogExitQuestFunction("GreatRecipe_StartFight");
        break;
        case "Duel":
            DialogExit();
            AddDialogExitQuestFunction("GreatRecipe_LaunchDuel");
        break;
        case "ShoreSolo":
            DialogExit();
            GreatRecipe_ShoreStart(false);
        break;
        case "ShoreHelp":
            DialogExit();
            GreatRecipe_ShoreStart(true);
        break;
        case "ShoreHandOver":
            if (GreatRecipe_ShoreHandOver()) dialog.text = "Почерк тот же... Вот где кончились доли моей старой команды. Книга останется у меня. А ты к боцману: Ратли снова собирает людей у пушек.";
            else dialog.text = "Мне нужна сама книга, а не рассказ о ней. Вернись к тайнику, если оставил её там.";
            link.l1 = "Пойду на палубу."; link.l1.go = "Exit";
        break;
        case "Pledge":
            if (GreatRecipe_AlliancePledge()) dialog.text = "Мой корабль ещё придётся взять, а клинок — выбить из руки. Сумеешь — приму твоё слово. Но мир с берегом за людей не подпишешь: с ними придётся говорить самому.";
            else dialog.text = "Сначала докажи делом, что стоишь своего слова. Дружба не покупается обещанием перед боем.";
            link.l1 = "Выйдем на равных. Дальше решит бой."; link.l1.go = "Duel";
            link.l2 = "Прежде закончу разговор с командой."; link.l2.go = "Exit";
        break;
        case "Exit":
            NextDiag.CurrentNode = "First time";
            if (GreatRecipe_IntroPending() && CheckAttribute(pchar, "GreatRecipe.Arrival.Resume")) NextDiag.CurrentNode = pchar.GreatRecipe.Arrival.Resume;
            DialogExit();
        break;
    }
}
