# cpp-advanced-vector
Аналог std::vector с размещением в сырой памяти. Может быть включен в другой проект. Работа конструктора контейнера построена на выделении сырой памяти.Шаблоны позволяют использовать контейнер для любых конструируемых типов. Методы класса минимизируют копирование значений - в основном идет перемещение значений или непосредстевнная их инициализация внутри контейнера. За счет этого класс эффективен с т.з. использования памяти и скорости своей работы.


# Системные требования
C++17
