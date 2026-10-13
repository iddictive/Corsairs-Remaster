// Habitue_dialog.c: call the entry only from sit_3's ordinary case 0.
// Custom nodes share stock sit_2 (paid next drink) and exit_sit (stand up).
// This dialogue owns no quest, journal, NPC, money or saved-state mutation.
bool GreatRecipe_ProcessTavernRumor(string node, aref Link)
{
    if (node != "GreatRecipe_TavernRumor" && node != "GreatRecipe_TavernRumor_Flying" &&
        node != "GreatRecipe_TavernRumor_Underwater" && node != "GreatRecipe_TavernRumor_EmptyCrew" &&
        node != "GreatRecipe_TavernRumor_Island") return false;

    switch (node)
    {
        case "GreatRecipe_TavernRumor":
            switch (rand(3))
            {
                case 0:
                    dialog.text = "А вот один шкипер клянётся: корабль Кровавой Бороды над самой водой летает. Ни волну не режет, ни следа не оставляет! Паруса красные, а под килем молнии скачут. Пролетел над его шлюпкой — у бедняги шляпу с головы сдуло. Шляпа-то новая была, вот он и запомнил.";
                    link.l1 = "Шляпу ветер унёс, а корабль потом к рассказу прибавился?";
                    link.l1.go = "GreatRecipe_TavernRumor_Flying";
                break;

                case 1:
                    dialog.text = "Ты бы послушал нашего плотника! Говорит, у Кровавой Бороды паруса под водой ходят, а на мачтах рыба сушится. Корабль по дну несётся, словно по ветру. Наверх торчит одна рыжая борода: капитан ею дорогу щупает, чтоб в берег не врезаться. Плотник за это кулаками божился!";
                    link.l1 = "Под водой паруса? Ты его с утопленным кораблём не спутал?";
                    link.l1.go = "GreatRecipe_TavernRumor_Underwater";
                break;

                case 2:
                    dialog.text = "На судне Кровавой Бороды, говорят, ни единой живой души. На реях пустые куртки висят, сами снасти тянут. А ночью из кубрика хохот: в кости играют. Один рыбак подкрался послушать — они ставили его зубы! Все до последнего пересчитали, хотя рот он держал закрытым.";
                    link.l1 = "Пустые куртки в кости играют? Кто тебе такое рассказал?";
                    link.l1.go = "GreatRecipe_TavernRumor_EmptyCrew";
                break;

                case 3:
                    dialog.text = "Слышал про остров, который с якоря снимается? Вчера там пальмы, нынче — одни барашки. Один боцман хотел пристать, а остров развернулся к нему кормой! Пальмы оказались мачтами, утёс — бортом. И рыжий черт с утёса орал, чтоб за стоянку платили. Кровавая Борода, кто ж ещё!";
                    link.l1 = "Остров с кормой? У боцмана в кружке море ещё не качалось?";
                    link.l1.go = "GreatRecipe_TavernRumor_Island";
                break;
            }
            link.l2 = "Ладно, берегу голову для других чудес. Мне пора.";
            link.l2.go = "exit_sit";
            return true;
        break;

        case "GreatRecipe_TavernRumor_Flying":
            dialog.text = "Может, и прибавился. Сначала шкипер говорил — у Бермуд это было, потом уже у Ямайки. Про шляпу зато ни разу не соврал: всякий вечер новую требует. Я-то самого корабля не видел. Да и кто видел, тот почему-то всё больше в таверне сидит.";
        break;

        case "GreatRecipe_TavernRumor_Underwater":
            dialog.text = "Вот и я ему так сказал! А он: тонуть всякий дурак умеет, ты по дну под парусом пройди. Про Бермуды ещё бормотал... Или это бочар про Бермуды говорил? Они оба с одной кружки пили. Какой порт, какое судно — не добился я толку.";
        break;

        case "GreatRecipe_TavernRumor_EmptyCrew":
            dialog.text = "Матрос с бермудской шхуны. У него самого половины зубов нет, так что, может, проиграл, ха! Только другой уверял, будто у Бороды команда живее всех живых: пляшут на палубе, ром бочками пьют. Кто из них врёт? Откуда мне знать, я с ними в море не ходил.";
        break;

        case "GreatRecipe_TavernRumor_Island":
            dialog.text = "Ещё как качалось! Я спросил, где тот остров, — он на потолок показал. Потом помянул Бермуды и какого-то Лаки: дескать, тому и без наших баек забот хватает. Кто такой, не знаю. А на другой день боцман уже клялся, что остров сожрал его якорь. Я бы ему шлюпку не доверил.";
        break;

    }

    if (makeint(pchar.money) >= 2)
    {
        link.l1 = "С тебя байки, с меня ром. Наливай!";
        link.l1.go = "sit_2";
        link.l2 = "Ну и морские чудеса... Ладно, мне пора.";
        link.l2.go = "exit_sit";
    }
    else
    {
        link.l1 = "Ну и морские чудеса... Ладно, мне пора.";
        link.l1.go = "exit_sit";
    }
    return true;
}
