Response player_turn(const bool color)
{
    // return 1 if quit
    
    // Собираем все начальные позиции возможных ходов для подсветки
    vector<pair<POS_T, POS_T>> cells;
    for (auto turn : logic.turns)
    {
        cells.emplace_back(turn.x, turn.y);
    }
    // Подсвечиваем клетки, с которых можно начать ход
    board.highlight_cells(cells);
    
    move_pos pos = {-1, -1, -1, -1};
    POS_T x = -1, y = -1;
    
    // Фаза 1: Выбор фигуры для хода и ее целевой позиции
    // trying to make first move
    while (true)
    {
        // Ожидаем выбора клетки от пользователя
        auto resp = hand.get_cell();
        // Если получен не CELL ответ (QUIT, BACK, etc.) - возвращаем его
        if (get<0>(resp) != Response::CELL)
            return get<0>(resp);
            
        // Преобразуем ответ в координаты клетки
        pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

        bool is_correct = false;
        // Проверяем корректность выбора клетки
        for (auto turn : logic.turns)
        {
            // Если выбрана начальная позиция существующего хода
            if (turn.x == cell.first && turn.y == cell.second)
            {
                is_correct = true;
                break;
            }
            // Если выбрана конечная позиция для уже выбранной фигуры
            if (turn == move_pos{x, y, cell.first, cell.second})
            {
                pos = turn;
                break;
            }
        }
        // Если нашли полное соответствие хода - выходим из цикла
        if (pos.x != -1)
            break;
            
        // Если выбрана некорректная клетка
        if (!is_correct)
        {
            // Сбрасываем текущий выбор и обновляем подсветку
            if (x != -1)
            {
                board.clear_active();
                board.clear_highlight();
                board.highlight_cells(cells);
            }
            x = -1;
            y = -1;
            continue;
        }
        
        // Сохраняем выбранную начальную позицию
        x = cell.first;
        y = cell.second;
        
        // Обновляем визуальное представление
        board.clear_highlight();
        board.set_active(x, y); // Подсвечиваем выбранную фигуру
        
        // Собираем все возможные целевые позиции для выбранной фигуры
        vector<pair<POS_T, POS_T>> cells2;
        for (auto turn : logic.turns)
        {
            if (turn.x == x && turn.y == y)
            {
                cells2.emplace_back(turn.x2, turn.y2);
            }
        }
        // Подсвечиваем возможные целевые клетки
        board.highlight_cells(cells2);
    }
    
    // Очищаем визуальные эффекты после выбора хода
    board.clear_highlight();
    board.clear_active();
    
    // Выполняем основной ход
    board.move_piece(pos, pos.xb != -1);
    
    // Если ход без взятия - завершаем ход
    if (pos.xb == -1)
        return Response::OK;
        
    // Фаза 2: Обработка серии взятий (для шашек)
    // continue beating while can
    beat_series = 1;
    while (true)
    {
        // Ищем возможные продолжения взятий из текущей позиции
        logic.find_turns(pos.x2, pos.y2);
        // Если нет дальнейших взятий - выходим из цикла
        if (!logic.have_beats)
            break;

        // Подсвечиваем возможные продолжения взятий
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x2, turn.y2);
        }
        board.highlight_cells(cells);
        board.set_active(pos.x2, pos.y2);
        
        // Фаза 2.1: Выбор продолжения серии взятий
        // trying to make move
        while (true)
        {
            // Ожидаем выбора клетки для продолжения хода
            auto resp = hand.get_cell();
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp);
                
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

            bool is_correct = false;
            // Проверяем корректность выбранного продолжения
            for (auto turn : logic.turns)
            {
                if (turn.x2 == cell.first && turn.y2 == cell.second)
                {
                    is_correct = true;
                    pos = turn; // Обновляем текущую позицию для следующего взятия
                    break;
                }
            }
            if (!is_correct)
                continue;

            // Очищаем визуальные эффекты и выполняем взятие
            board.clear_highlight();
            board.clear_active();
            beat_series += 1; // Увеличиваем счетчик серии взятий
            board.move_piece(pos, beat_series);
            break;
        }
    }

    return Response::OK;
}
