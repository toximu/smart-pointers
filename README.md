# Smart Pointers

Потокобезопасная реализация умных указателей `unique_ptr` и `shared_ptr` на C++20. Управление ресурсами и памятью реализовано со строгим соблюдением правила пяти (Rule-of-5).

## Сборка и запуск тестов

```bash
cmake -B build
cmake --build build
./build/tests