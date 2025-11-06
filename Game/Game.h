// to start checkers
int play()
{
    // Замер времени начала игры для профилирования
    auto start = chrono::steady_clock::now();
    
    // Обработка режима повторной игры - реинициализация состояния
    if (is_replay)
    {
        logic = Logic(&board, &config);
        config.reload();
        board.redraw();
    }
    else
    {
        // Первоначальная отрисовка игрового поля
        board.start_draw();
    }
    is_replay = false;

    int turn_num = -1;
    bool is_quit = false;
    // Получение максимального количества ходов из конфигурации
    const int Max_turns = config("Game", "MaxNumTurns");
    
    // Главный игровой цикл - выполняется пока не достигнут лимит ходов
    while (++turn_num < Max_turns)
    {
        beat_series = 0;
        // Поиск возможных ходов для текущего игрока
        logic.find_turns(turn_num % 2);
        
        // Если ходов нет - игра завершается
        if (logic.turns.empty())
            break;
            
        // Настройка уровня ИИ для бота на основе конфигурации
        logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
        
        // Определение типа игрока (человек/бот) и выполнение хода
        if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
        {
            // Ход человека
            auto resp = player_turn(turn_num % 2);
            if (resp == Response::QUIT)
            {
                is_quit = true;
                break;
            }
            else if (resp == Response::REPLAY)
            {
                is_replay = true;
                break;
            }
            else if (resp == Response::BACK)
            {
                // Обработка отмены хода с учетом особенностей правил
                if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                    !beat_series && board.history_mtx.size() > 2)
                {
                    board.rollback();
                    --turn_num;
                }
                if (!beat_series)
                    --turn_num;

                board.rollback();
                --turn_num;
                beat_series = 0;
            }
        }
        else
            // Ход бота
            bot_turn(turn_num % 2);
    }
    
    // Замер и запись времени игры в лог
    auto end = chrono::steady_clock::now();
    ofstream fout(project_path + "log.txt", ios_base::app);
    fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
    fout.close();

    // Рекурсивный перезапуск при запросе повторной игры
    if (is_replay)
        return play();
    if (is_quit)
        return 0;
        
    // Определение результата игры
    int res = 2;
    if (turn_num == Max_turns)
    {
        res = 0;  // Ничья - достигнут лимит ходов
    }
    else if (turn_num % 2)
    {
        res = 1;  // Победа черных
    }
    
    // Отображение финального экрана и обработка пользовательского ввода
    board.show_final(res);
    auto resp = hand.wait();
    if (resp == Response::REPLAY)
    {
        is_replay = true;
        return play();
    }
    return res;
}
