# Поднятие сервисов: дин. таблицы, CHYT, SPYT

Количество инстансов tab-нод конфигурируется в спеке кластера.
Настройки числа целлов/памяти - это дин конфиг.
Default bundle создаётся автоматом, в первом варианте все живут в нём (иначе нужен bundle_balancer, теги, всё это)?

CHYT - клика запускается через CLI. Можно бинарь CHYT зашить в докер-образ, иначе придётся его как-то в кипарис заливать.

SPYT - только в варианте с porto ЛИБО тоже зашивать вместе с зависимостями в наш докер-образ.
Надо обсуждать ещё?