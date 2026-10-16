// диалог по городам
void ProcessCommonDialogEvent(ref NPChar, aref Link, aref NextDiag)
{
    switch (Dialog.CurrentNode)
	{
		case "quests":
			dialog.text = NPCStringReactionRepeat(RandPhraseSimple("Какие вопросы?", "Что вам угодно, " + GetAddress_Form(NPChar) + "?"), "Совсем недавно вы пытались задать мне вопрос, " + GetAddress_Form(NPChar) + "...", "В течение этого дня вы уже третий раз говорите о каком-то вопросе...",
                          "Опять вопросы будем задавать?", "block", 1, npchar, Dialog.CurrentNode);
			link.l1 = HeroStringReactionRepeat(RandPhraseSimple("Я "+ GetSexPhrase("передумал","передумала") +"...", "Сейчас мне не о чем говорить."), "Хм, что-то с памятью моей стало...",
                      "Да уж, действительно в третий раз...", "Да нет, какие вопросы...", npchar, Dialog.CurrentNode);
			link.l1.go = "exit";
			// ==> Проверяем поле состояния квестов.
            link.l9 = "О жемчуге черепахи и Великом рецепте."; link.l9.go = "GreatRecipe_Offer";
            if (pchar.questTemp.State == "SeekBible_toBermudes") 
			{				
                link.l1 = HeroStringReactionRepeat("Послушай, я ищу пирата, который продавал здесь Библию какую-то странную. Не слышал об этом деле?",
                "Я про Библию "+ GetSexPhrase("хотел","хотела") +" спросить... уже выяснили, извини.", "Да уж, верно...", "А-а-а, разобрались уже, извини...", npchar, Dialog.CurrentNode);
                link.l1.go = DialogGoNodeRepeat("Step_H8_1", "", "", "", npchar, Dialog.CurrentNode);
            }
            if (pchar.RomanticQuest == "BrotherIsDead") 
			{				
                link.l1 = HeroStringReactionRepeat("Слушай, ты не в курсе, где Атилла сейчас обретается?",
                "Да, да, про Атиллу говорили...", "Да уж, верно...", "А-а-а, разобрались уже, извини...", npchar, Dialog.CurrentNode);
                link.l1.go = DialogGoNodeRepeat("Step_Romantic_1", "", "", "", npchar, Dialog.CurrentNode);
            }
			if (pchar.questTemp.BlueBird == "toBermudes")
			{
				link.l1 = "Послушай, приятель, ничего не слышно в таверне интересного про шебеку 'Синяя Птица'?";
				link.l1.go = "BlueBird_1";
			}
			if (pchar.questTemp.piratesLine == "KillLoy_toBermudes")
			{
				link.l1 = "Здесь, случайно, не появлялся Эдвард Лоу?";
				link.l1.go = "PL_Q3_1";
			}
			if (pchar.questTemp.piratesLine == "KillLoy_toTavernAgain" || pchar.questTemp.piratesLine == "KillLoy_2toTavernAgain")
			{
				link.l1 = "Я ищу Эдварда Лоу.";
				link.l1.go = "PL_Q3_2";
			}
			if (pchar.questTemp.piratesLine == "PL5Hunter_toBermudes")
			{
				link.l1 = "К тебе в таверну, Джон Эйвори не забегал?";
				link.l1.go = "StepPL5Berm_1";
			}
            // <== Проверяем поле состояния квестов.
		break;

        case "GreatRecipe_Offer":
            dialog.text = "Слыхал о Кровавой Бороде? Не о том, что за долей гонится. Этот жжёт суда ради смеха. Недавно рыбаки подняли шлюпку: в ней ни одного целого человека, а в борту железная скорлупа, будто жемчуг. Кто-то клянётся, что видел его корабль у Бермуд при большой луне. Кто-то — остров, которого вчера ещё не было. Пьяные байки? Может быть. Только мой брат с того рейса не вернулся.";
            link.l1 = "Что за Борода?"; link.l1.go = "GreatRecipe_Story";
            link.l2 = "Кто видел остров? Где искать?"; link.l2.go = "GreatRecipe_Rumor";
            link.l3 = "Почему ты назвал железо жемчугом?"; link.l3.go = "GreatRecipe_Ammo";
            link.l4 = "Тебе нужен брат или рецепт?"; link.l4.go = "GreatRecipe_Price";
            if (GreatRecipe_Phase() == "" && CheckAttribute(pchar, "Ship.Type") && sti(pchar.Ship.Type) != SHIP_NOTUSED)
            { link.l5 = "Проверю слухи. Если найду что-нибудь — принесу."; link.l5.go = "GreatRecipe_Accept"; }
            if (GreatRecipe_Phase() == "search") dialog.text = "Том с рыбацкой шхуны говорит — ясная луна, открытая вода у Бермуд. А его матрос божится, что это был корабль, не остров. Оба трезвые. За точность их слов я головой не ручаюсь.";
            if (GreatRecipe_Phase() == "pearl_search")
            {
                dialog.text = "Ты вернулся! Скажи, было там что-нибудь, кроме воды?";
                if (GetCharacterItem(pchar, "GreatRecipeRedPearl") > 0)
                { link.l5 = "Вот красная жемчужина с его клеймом."; link.l5.go = "GreatRecipe_TurnIn"; }
            }
            if (GreatRecipe_Phase() == "production") dialog.text = "Паскаль уже договорился с мастерами. Дай им неделю: это не пули отлить. Потом в магазине будет по небольшой партии каждый день. Не заставляй людей работать с порохом впопыхах.";
            if (GreatRecipe_Phase() == "won") dialog.text = "Мастера разобрались с жемчугом. Паскаль держит партии в магазине. Надеюсь, с твоего борта эта дрянь полетит в тех, кто её заслужил.";
            if (GreatRecipe_Phase() == "lost") dialog.text = "Мне тоже снилось море после брата. Только один портовый сторож видел красные паруса в грозу у Бермуд. Ночь была лунная, пока не затянуло тучами... Сам уж решай, верить ли ему.";
            if (GreatRecipe_AllianceStage() == "parley")
            { link.l8 = "Борода обещал освободить пленников и оставить берег в покое. Обсудим возмещение семьям?"; link.l8.go = "GreatRecipe_Restitution"; }
            link.l9 = "Не хочу связываться с этой историей."; link.l9.go = "exit";
        break;
        case "GreatRecipe_Story":
            dialog.text = "Перехватывает мирных, сдирает с судов паруса, а потом кружит, пока люди просят воды. Не торгуется и не требует выкупа. Слышал, держит людей в трюме, заставляет служить — и смотрит, кто сломается первым. Мне это рассказал калека без кисти. Остальным рассказчикам я бы не поверил.";
            link.l1 = "Капитан, которому убийство в удовольствие..."; link.l1.go = "GreatRecipe_Offer";
        break;
        case "GreatRecipe_Rumor":
            dialog.text = "Не рисуй крест на карте с моих слов. Они шли мимо Бермуд, далеко от берега; море светилось, было тихо. Впереди показалась темнота, как остров. Потом вода пошла кругом. Один уверяет — видел мачты внутри, другой ничего не помнит. После такой ночи я бы тоже напился.";
            link.l1 = "Поищу в открытой воде, при ясной луне."; link.l1.go = "GreatRecipe_Offer";
        break;
        case "GreatRecipe_Ammo":
            dialog.text = "Рыбаки так прозвали скорлупу: круглая, гладкая, внутри железные зубы. Говорят, рвётся уже за бортом — и людей срезает, и корпус распарывает. Ни наш кузнец, ни Паскаль такого не делали. Будь у нас этот секрет, берег бы не дрожал от каждого красного паруса.";
            link.l1 = "Сначала надо понять, что внутри."; link.l1.go = "GreatRecipe_Offer";
        break;
        case "GreatRecipe_Price":
            dialog.text = "Брата рецепт не вернёт. Но я устал выкупать чужих детей из рук людей, которые служили этому ублюдку. Хочу, чтоб однажды у берега оказался капитан с такими же ядрами — и повернул их против него. А если найдёшь след брата... просто скажи правду.";
            link.l1 = "Пустых обещаний не дам."; link.l1.go = "GreatRecipe_Offer";
        break;
        case "GreatRecipe_TurnIn":
            if (GreatRecipe_TurnInPearl()) dialog.text = "Клеймо настоящее. Смотри — шов, а под ним порох и железная рубка... Паскаль найдёт мастеров. Через неделю зайди в магазин; больше двух сотен в запасе держать не станем. Я не хочу, чтобы пол-Бермуд взлетело в воздух.";
            else dialog.text = "Покажи саму жемчужину. По рассказу мастер её не соберёт.";
            link.l1 = "Береги людей, Лаки."; link.l1.go = "exit";
        break;
        case "GreatRecipe_Accept":
            if (GreatRecipe_Accept()) dialog.text = "Хорошего тебе ветра. Узнаешь правду — вернись и расскажи. Я давно не жду добрых вестей, но ждать не перестал.";
            else dialog.text = "Сейчас за это дело не взяться. Сначала нужен собственный корабль и свобода.";
            link.l1 = "До встречи."; link.l1.go = "exit";
        break;
        case "GreatRecipe_Restitution":
            dialog.text = GreatRecipe_AllianceAction(NPChar.id, "restitution");
            link.l1 = "Поговорю с Джекменом."; link.l1.go = "exit";
        break;

		//шебека Синяя Птица
		case "BlueBird_1":
			dialog.text = NPCStringReactionRepeat("Слышно-то много всякого. В основном говорят, что корабль очень быстрый. Поймать пытались, да куда там...", 
				"Мы уже говорили об этом.", 
				"Опять? Уже все выяснили.",
                "Простите, если вам больше ничего не надо, то, пожалуй, займусь другими клиентами.", "block", 1, npchar, Dialog.CurrentNode);
			link.l1 = HeroStringReactionRepeat("А кто пытался поймать?", 
				"Да, точно...",
                "Хм, верно.", 
				"Хорошо...", npchar, Dialog.CurrentNode);
			link.l1.go = DialogGoNodeRepeat("BlueBird_2", "exit", "exit", "exit", npchar, Dialog.CurrentNode);
		break;

		case "BlueBird_2":
			dialog.text = "Многие пытались, да обломались, ха-ха!!";
			link.l1 = "А что-нибудь путное тебе известно? Где её видят часто, к примеру.";
			link.l1.go = "BlueBird_3";
		break;
		case "BlueBird_3":
			dialog.text = "Дык здесь и видят, у Бермуд. По хождению в наших рифах 'Синей Птице' равных нет, не раз в погоне за ней галеоны да фрегаты бились - и прямиком на дно!";
			link.l1 = "Вот значит как... Ну что же, будем искать. Слушай, а хозяин магазина местный, он что за человек?";
			link.l1.go = "BlueBird_4";
		break;
		case "BlueBird_4":
			dialog.text = "Авторитетный весьма, уважаемый среди торговцев.";
			link.l1 = "Понятно... А есть человек, который с ним не в очень хороших отношениях?";
			link.l1.go = "BlueBird_5";
		break;
		case "BlueBird_5":
			dialog.text = "Откровенных врагов у него нет. Но вот бывший хозяин магазина, думаю, зуб на него имеет, хе-хе...";
			link.l1 = "Не будешь ли ты любезен, милейший, назвать мне его имя.";
			link.l1.go = "BlueBird_6";
		break;
		case "BlueBird_6":
			dialog.text = "Его зовут Джон Нортон. На Бермудах ты его уже не найдёшь. Слышал я, что он перебрался в Пуэрто-Принсипе, торгует там себе потихоньку и никуда не высовывается.";
			link.l1 = "Большое тебе спасибо, приятель. Ты - хороший трактирщик.";
			link.l1.go = "BlueBird_7";
		break;
		case "BlueBird_7":
			dialog.text = "Спасибо за комплимент, " + GetAddress_Form(NPChar) + ". Удачи вам!";
			link.l1 = "И тебе всего хорошего, приятель.";
			link.l1.go = "exit";
			AddQuestRecord("Xebeca_BlueBird", "3");
			pchar.questTemp.BlueBird = "toPuertoPrincipe";
		break;

		//голл. линейка, квест №8
 		case "Step_H8_1":
    		dialog.text = "Нет, не слышал.";
    		link.l1 = "Вроде история известная, мне на Кюрасао её рассказали.";
    		link.l1.go = "Step_H8_2";
 		break;
 		case "Step_H8_2":
    		dialog.text = "Ничего об этом не знаю, у меня в таверне такого не было.";
    		link.l1 = "Спасибо за помощь.";
    		link.l1.go = "exit";
 		break;
		//Изабелла, ищем Атиллу
 		case "Step_Romantic_1":
    		dialog.text = "Знаю, он дом купил здесь, в городе. Так что там и должен быть.";
    		link.l1 = "Спасибо, приятель...";
    		link.l1.go = "exit";
 		break;
		//Пиратка, квест №3, поиски Лоу
		case "PL_Q3_1":
			dialog.text = "Появлялся. Все ходил, чего-то вынюхивал. То к одному подсядет, то к другому... Кажется, он интересовался нашей верфью.";
			link.l1 = "Благодарю.";
			link.l1.go = "exit";
			AddQuestRecord("Pir_Line_3_KillLoy", "9");
			pchar.questTemp.piratesLine = "KillLoy_toAlexus";
		break;

		case "PL_Q3_2":
			dialog.text = "Так вы уже про него спрашивали.";
			link.l1 = "Может, ты чего вспомнил?";
			link.l1.go = "PL_Q3_3";
		break;
		case "PL_Q3_3":
			dialog.text = "Да, чего тут вспоминать, он все про верфь выспрашивал, к людям разным подсаживался. Вон, кстати, Капитан Гудли сел за столик, он с Лоу разговаривал.";
			link.l1 = "Капитан Гудли?! Спасибо, уже кое-что.";
			link.l1.go = "exit";
			pchar.questTemp.piratesLine = "KillLoy_toGoogly";
						//садим капитана Гудли в таверну
			sld = GetCharacter(NPC_GenerateCharacter("CapGoodly", "Goodley", "man", "man", 20, PIRATE, -1, true));
			sld.name = "Капитан";
			sld.lastname = "Гудли";
			sld.rank = 20;
			sld.city = "Pirates";
			sld.dialog.filename   = "Quest\PiratesLine_dialog.c";
			sld.dialog.currentnode   = "CapGoodly";
			sld.greeting = "Gr_EvilPirate";
			LAi_SetSitType(sld);
			LAi_group_MoveCharacter(sld, "PIRATE_CITIZENS");
            FreeSitLocator("Pirates_tavern", "sit2");
         	ChangeCharacterAddressGroup(sld, "Pirates_tavern", "sit", "sit2");
		break;
		//Пиратка, квест №5, ОЗГ
		case "StepPL5Berm_1":
			dialog.text = "Да вот, только что и забегал. Спросил, где живёт Орри Брюс, и ушёл. ";
			link.l1 = "И где же этот Орри Брюс обитает?";
			link.l1.go = "StepPL5Berm_2";
		break;
		case "StepPL5Berm_2":
			dialog.text = "Так, в двухэтажном доме с каменной кладкой он живёт. Как выйдешь из таверны, налево первый дом. Там и обитает...";
			link.l1 = "Спасибо, ты мне здорово помог!";
			link.l1.go = "exit";
			AddQuestRecord("Pir_Line_5_Hunter", "10");
			pchar.questTemp.piratesLine = "PL5Hunter_toOrryHouse";
		break;


	}
	UnloadSegment(NPChar.FileDialog2);  // если где-то выход внутри switch  по return не забыть сделать анлод
}

