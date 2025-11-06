#pragma once
#include <stdlib.h>

// Тип для координат на игровом поле. 
// Использование int8_t оптимизирует память для полей стандартных размеров (до 128x128)
typedef int8_t POS_T;

// Структура, описывающая ход в игре (шашки/шахматы)
struct move_pos
{
    POS_T x, y;             // Координаты начальной позиции фигуры (from)
    POS_T x2, y2;           // Координаты конечной позиции фигуры (to)
    POS_T xb = -1, yb = -1; // Координаты побитой фигуры (beaten). 
                            // Значения -1 указывают, что ход без взятия

    // Конструктор для обычного хода (без взятия)
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    
    // Конструктор для хода со взятием фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Оператор сравнения для проверки идентичности ходов
    // Используется для поиска в контейнерах и проверки валидности хода
    bool operator==(const move_pos &other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    
    // Отрицание оператора равенства для удобства
    bool operator!=(const move_pos &other) const
    {
        return !(*this == other);
    }
};
