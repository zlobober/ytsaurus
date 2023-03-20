---
vcsPath: ydb/docs/ru/core/yql/reference/yql-core/syntax/_includes/reduce.md
sourcePath: ydb/docs/ru/core/yql/reference/yql-core/syntax/_includes/reduce.md
---
# REDUCE

Выполняет группировку входа по указанным ключевым столбцам, затем передает текущие ключи и ленивый итератор соответствующих им значений из остальных колонок на обработку в указанную UDF. По аналогии с `PROCESS` UDF может вернуть произвольное количество строк результата на каждый вызов, а также возвращать альтернативы (`Variant`) для создания множества выходов. В терминах MapReduce  очень близок к Reduce.

Указываемые следом ключевые слова:

* `PRESORT` <span style="color: gray;">(опционально)</span> — указание порядка внутри каждой группы, синтаксис аналогичен [ORDER BY](../select.md#orderby);
* `ON` <span style="color: gray;">(обязательно)</span> — указание ключевых столбцов;
* `USING` или `USING ALL` <span style="color: gray;">(обязательно)</span> — вызов UDF, о правилах подробнее ниже.

Правила передачи аргументов UDF:

* Если аргументом UDF указан `TableRows()`, то UDF обязана принимать один аргумент — ленивый итератор по строкам, с точки зрения типов — `Stream<Struct<...>>`. В этом случае выходным типом функции может быть только `Stream<OutputType>` или `List<OutputType>`. Гарантируется, что данные во входном итераторе сгруппированы по ключу и, при необходимости, отсортированы согласно `PRESORT` секции. С `TableRows()` можно использовать только `USING ALL`.
* При использовании `USING`:
    * UDF обязана принимать два аргумента: в первый передается текущий ключ, а во второй — ленивый итератор с соответствующими этому ключу значениями.
* При использовании `USING ALL`:
    * UDF обязана принимать один аргумент — ленивый итератор кортежей (`Tuple`), где первый элемент кортежа это ключ, а второй — ленивый итератор с соответствующими этому ключу значениями.
* Ключ для передачи в UDF формируется по следующему правилу. Если ключевая колонка одна, то её значение формирует ключ как есть, если несколько (список колонок указывается по аналогии с `GROUP BY` через запятую), то ключ будет формироваться как кортеж (`Tuple`) со значениями из перечисленных колонок в указанном порядке.
* При вызове `REDUCE` в запросе в круглых скобках после имени UDF указываются только выражение, значения которого будут переданы в качестве элементов итератора (второй аргумент UDF для `USING` или второй элемент кортежа для `USING ALL`).

Результат формируется аналогичным [PROCESS](../process.md) образом. Также доступно ключевое слово `TableRow()` для получения строки целиком в виде структуры.

В `REDUCE` можно передать несколько входов (под входом здесь подразумевается таблица, [диапазон таблиц](../select.md#range), подзапрос, [именованное выражение](../expressions.md#named-nodes)), разделенных запятой. Все входы обязаны иметь указанные в `ON` ключевые колонки совпадающего типа. В функцию из `USING` в этом случае можно передать только специальное именованное выражение `TableRow()`. Второй аргумент (или второй элемент кортежа для `USING ALL`) будет содержать ленивый итератор альтернатив с заполненным элементом, соответствущим номеру входа текущей записи.

После `USING` в `REDUCE` можно опционально указать `ASSUME ORDER BY` со списком столбцов. Результат такого `REDUCE` будет считаться сортированным, но без выполнения фактической сортировки. Проверка сортированности осуществляется на этапе исполнения запроса. Поддерживается задание порядка сортировки с помощью ключевых слов `ASC` (по возрастанию) и `DESC` (по убыванию). Выражения в `ASSUME ORDER BY` не поддерживается.

**Примеры:**
``` yql
REDUCE my_table
ON key, subkey
USING MyUdf::MyReducer(TableRow());
```

``` yql
REDUCE my_table
ON key, subkey
USING ALL MyUdf::MyStreamReducer(TableRow()); -- MyUdf::MyStreamReducer принимает на вход ленивый список кортежей (ключ, список записей для ключа)
```

``` yql
REDUCE my_table
PRESORT LENGTH(subkey) DESC
ON key
USING MyUdf::MyReducer(
    AsTuple(subkey, value)
);
```

``` yql
REDUCE my_table
ON key
USING ALL MyUdf::MyFlatStreamReducer(TableRows()); -- MyUdf::MyFlatStreamReducer принимает на вход единый ленивый список записей
```

``` yql
-- Функция возвращает альтернативы
$udf = Python::MyReducer(Callable<(String, Stream<Struct<...>>) -> Variant<Struct<...>, Struct<...>>>,
    $udfScript
);

-- На выходе из REDUCE получаем кортеж списков
$i, $j = (REDUCE my_table ON key USING $udf(TableRow()));

SELECT * FROM $i;
SELECT * FROM $j;
```


``` yql
$script = @@
def MyReducer(key, values):
    state = None, 0
    for name, last_visit_time in values:
        if state[1] < last_visit_time:
            state = name, last_visit_time
    return {
        'region':key,
        'last_visitor':state[0],
    }
@@;

$udf = Python::MyReducer(Callable<(
    Int64?,
    Stream<Tuple<String?, Uint64?>>
) -> Struct<
    region:Int64?,
    last_visitor:String?
>>,
    $script
);

REDUCE hahn.`home/yql/tutorial/users`
ON region USING $udf((name, last_visit_time));
```

``` yql
-- Функция принимает на вход ключ и итератор альтернатив
$udf = Python::MyReducer(Callable<(String, Stream<Variant<Struct<...>,Struct<...>>>) -> Struct<...>>,
    $udfScript
);

REDUCE my_table1, my_table2 ON key USING $udf(TableRow());
```