#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9; // Бесконечность для алгоритма минимакс

class Logic
{
  public:
    // Конструктор, инициализирующий логику игры с доской и конфигурацией
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        // Инициализация генератора случайных чисел (с случайным seed или фиксированным)
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType"); // Режим оценки позиции
        optimization = (*config)("Bot", "Optimization");   // Уровень оптимизации
    }

    // Публичные поля класса
    vector<move_pos> turns;  // Список возможных ходов для текущей позиции
    bool have_beats;         // Флаг наличия взятий среди возможных ходов
    int Max_depth;           // Максимальная глубина поиска для алгоритма минимакс

  private:
    // Приватные поля класса
    default_random_engine rand_eng;  // Генератор случайных чисел для перемешивания ходов
    string scoring_mode;              // Режим оценки позиции ("NumberAndPotential" и др.)
    string optimization;              // Уровень оптимизации алгоритма ("O0", "O1" и т.д.)
    vector<move_pos> next_move;       // Вектор лучших ходов для каждого состояния
    vector<int> next_best_state;      // Вектор переходов между состояниями для построения цепочки ходов
    Board *board;                     // Указатель на игровую доску
    Config *config;                   // Указатель на конфигурацию игры

public:
    // === ПЕРЕГРУЖЕННЫЕ ФУНКЦИИ find_turns() ===

    /**
     * Находит все возможные ходы для указанного цвета на текущей доске
     * @param color цвет игрока (0 - белые, 1 - черные)
     */
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    /**
     * Находит все возможные ходы для конкретной фигуры на текущей доске
     * @param x координата X фигуры
     * @param y координата Y фигуры
     */
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    /**
     * Находит все возможные ходы для указанного цвета на произвольной доске
     * Собирает ходы со всех фигур указанного цвета, приоритет отдает взятиям
     * @param color цвет игрока
     * @param mtx матрица состояния доски
     */
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        
        // Проход по всем клеткам доски
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                // Проверка, что фигура принадлежит нужному игроку
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    // Находим ходы для конкретной фигуры
                    find_turns(i, j, mtx);
                    
                    // Если нашли взятия и до этого их не было - очищаем результат
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    
                    // Добавляем ходы если: есть взятия и мы их уже собираем ИЛИ нет взятий
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        // Перемешиваем ходы для разнообразия игры бота
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    /**
     * Находит все возможные ходы для конкретной фигуры на произвольной доске
     * Сначала проверяет взятия, затем обычные ходы
     * @param x координата X фигуры
     * @param y координата Y фигуры  
     * @param mtx матрица состояния доски
     */
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y];
        
        // Проверка ВЗЯТИЙ (приоритет)
        switch (type)
        {
        case 1: // Белая шашка
        case 2: // Черная шашка
            // Проверка взятий для обычных шашек (ход на 2 клетки по диагонали)
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 2)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2; // Координаты побитой фигуры
                    
                    // Условия корректного взятия:
                    // - Конечная клетка свободна
                    // - Между начальной и конечной есть фигура противника
                    // - Эта фигура принадлежит противнику
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default: // Дамки (3 - белая, 4 - черная)
            // Проверка взятий для дамок (ход на любое расстояние по диагонали)
            for (POS_T i = -1; i <= 1; i += 2)  // Направления по X
            {
                for (POS_T j = -1; j <= 1; j += 2)  // Направления по Y
                {
                    POS_T xb = -1, yb = -1; // Координаты побитой фигуры
                    
                    // Движение по диагонали пока не выйдем за пределы доски
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2]) // Если нашли фигуру
                        {
                            // Если это своя фигура ИЛИ уже нашли фигуру противника - прерываем
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            // Запоминаем координаты фигуры противника
                            xb = i2;
                            yb = j2;
                        }
                        // Если нашли фигуру противника и можем перепрыгнуть
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        
        // Если нашли взятия - возвращаем только их
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        
        // Проверка ОБЫЧНЫХ ХОДОВ (если нет взятий)
        switch (type)
        {
        case 1: // Белая шашка (ходит только вперед - уменьшение X)
        case 2: // Черная шашка (ходит только назад - увеличение X)
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1); // Направление движения
                for (POS_T j = y - 1; j <= y + 1; j += 2) // Диагональные направления
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                        continue;
                    turns.emplace_back(x, y, i, j);
                }
                break;
            }
        default: // Дамки
            // Дамка может ходить на любое количество клеток по диагонали
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    // Движение по диагонали пока не встретим препятствие
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

    /**
     * Выполняет ход на переданной матрице и возвращает новое состояние
     * @param mtx исходное состояние доски
     * @param turn ход для выполнения
     * @return новое состояние доски после хода
     */
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        // Если ход включает взятие - удаляем побитую фигуру
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;
            
        // Проверка превращения в дамку
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
            
        // Перемещение фигуры
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;
        return mtx;
    }

    /**
     * Вычисляет оценку текущей позиции для алгоритма минимакс
     * @param mtx состояние доски для оценки
     * @param first_bot_color цвет бота, для которого считается оценка
     * @return числовая оценка позиции (чем больше - тем лучше для бота)
     */
    double calc_score(const vector<vector<POS_T>> &mtx, const bool first_bot_color) const
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0;
        
        // Подсчет количества фигур каждого типа
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);  // Белые шашки
                wq += (mtx[i][j] == 3); // Белые дамки
                b += (mtx[i][j] == 2);  // Черные шашки  
                bq += (mtx[i][j] == 4); // Черные дамки
                
                // Дополнительная оценка потенциала для обычных шашек
                if (scoring_mode == "NumberAndPotential")
                {
                    // Белые шашки получают бонус за приближение к дамочному полю
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    // Черные шашки получают бонус за приближение к дамочному полю  
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        
        // Если бот играет белыми - меняем местами оценки
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        
        // Проверка терминальных состояний
        if (w + wq == 0) // Противник проиграл
            return INF;
        if (b + bq == 0) // Бот проиграл
            return 0;
            
        // Коэффициент ценности дамки относительно шашки
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5; // В этом режиме дамки ценятся выше
        }
        
        // Формула оценки: (фигуры бота) / (фигуры противника)
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

    // УДАЛЕННЫЕ РЕАЛИЗАЦИИ ФУНКЦИЙ (по условию задачи):
    // vector<move_pos> find_best_turns(const bool color)
    // double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state, double alpha = -1)
    // double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1, double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
};
