# Работа с Excel

Web интерфейс {{product-name}} позволяет загружать небольшие Microsoft Excel таблицы в статические таблицы {{product-name}}, а также выгружать данные статических таблиц со строгой схемой в виде таблиц Microsoft Excel.

## Заливка в новую таблицу

Загрузка с одновременным созданием {{product-name}} таблицы доступна из меню **Create object** на странице директории.

![](../../../../images/excel_upload_create.png){ .center }

В меню загрузки важно установить правильные значения переключателей **Column names** и **Types**.

Включенный **Column names** соответствует наличию имён колонок в первой строке Excel таблицы. Если в таблице нет заголовка, в качестве имён колонок {{product-name}} таблицы будут использоваться имена колонок из Excel: A, B, C и так далее.

Включенный **Types** соответствует наличию строки с типами. Строка с типами должна идти за строкой с именами колонок при её наличии. Типы задаются в формате `type` (смотрите [соответствие типов](../../../user-guide/storage/excel.md#type-mapping)).

{% note warning "Внимание" %}

Если типы не заданы, для всех колонок будет использоваться `any`.

{% endnote %}

![](../../../../images/excel_upload.png){ .center }

## Заливка в существующую таблицу

Загрузка в существующую {{product-name}} таблицу доступна по кнопке **Upload** на странице таблицы.

![](../../../../images/excel_upload_existing.png){ .center }

{% note warning "Внимание" %}

Колонки из схемы {{product-name}} таблицы должны обязательно присутствовать в Excel таблице.
Дополнительные колонки Excel таблицы будут проигнорированы.

{% endnote %}

В меню загрузки с помощью переключателя **Append** можно выбрать режим загрузки. Включенное значение соответствует дописыванию строк в конец {{product-name}} таблицы, выключенное — перезаписыванию таблицы.

![](../../../../images/excel_upload_append.png){ .center }

## Ограничения загрузки

* Загружается только первый лист Excel таблицы;
* максимальное число строк — 1048576;
* максимальное число столбцов - 16384;
* максимальный размер входного файла — 50 МБ.

## Скачивание

Интерфейс позволяет указать интересующее подмножество строк и колонок.

![](../../../../images/excel_download.png){ .center }

В первой строке результата будут названия колонок, во второй — их типы.

### Ограничения

* Статические таблицы со строгой схемой;
* максимальное число строк — `1048574`;
* максимальное число колонок — `16384`;
* максимальный размер выходного файла — 50 МБ;
* максимальная длина строки в ячейке — `32767`; строки длиннее `32767` обрезаются.

### Соответствие типов {#type-mapping}

В Microsoft Excel поддерживается [4 типа данных](http://johnatten.com/2011/10/03/microsoft-excel-basics-part-ii-data-types-in-excel/): `Logical`, `Number`, `Text`, `Error`.

|**Описание типа данных {{product-name}}**|**Представление в `type`**|**Представление в `type_v3`**|**Представление в Excel**|
|----------------|--------------------|-----------------------|---------------------|
|целое число в диапазоне `[-2^63, 2^63-1]`|`int64`|`int64`|`Number*`|
|целое число в диапазоне `[-2^31, 2^31-1]`|`int32`|`int32`|`Number`|
|целое число в диапазоне `[-2^15, 2^15-1]`|`int16`|`int16`|`Number`|
|целое число в диапазоне `[-2^7, 2^7-1]`|`int8`|`int8`|`Number`|
|целое число в диапазоне `[0, 2^64-1]`|`uint64`|`uint64`|`Number*`|
|целое число в диапазоне `[0, 2^32-1]`|`uint32`|`uint32`|`Number`|
|целое число в диапазоне `[0, 2^16-1]`|`uint16`|`uint16`|`Number`|
|целое число в диапазоне `[0, 2^8-1]`|`uint8`|`uint8`|`Number`|
|вещественное 4-байтное число|`float`|`float`|`Number*`|
|вещественное 8-байтное число|`double`|`double`|`Number*`|
|стандартный булев тип `true/false`|`boolean`|`bool` (отличие от `type`)|`Logical`|
|произвольная последовательность байтов|`string`|`string`|`Text*`|
|корректная UTF8 последовательность|`utf8`|`utf8`|`Text*`|
|целое число в диапазоне `[0, 49673 - 1]`,<br> выражает число дней, прошедших с unix-эпохи;<br> выражаемый диапазон дат: `[1970-01-01, 2105-12-31]`|`date`|`date`|`Number**`|
|целое число в диапазоне `[0, 49673 * 86400 - 1]`,<br> выражает число секунд, прошедших с unix-эпохи;<br> выражаемый диапазон времён:<br>`[1970-01-01T00:00:00Z, 2105-12-31T23:59:59Z]`|`datetime`|`datetime`|`Number**`|
|целое число в диапазоне `[0, 49673 * 86400 * 10^6 - 1]`,<br> выражает число микросекунд, прошедших с unix-эпохи;<br> выражаемый диапазон времён:<br>`[1970-01-01T00:00:00Z, 2105-12-31T23:59:59.999999Z]`|`timestamp`|`timestamp`|`Number***`|
|целое число в диапазоне<br>`[-49673 * 86400 * 10^6 + 1, 49673 * 86400 * 10^6 - 1]`,<br> выражает число микросекунд между двумя таймстемпами|`interval`|`interval`|`Number*`|
|произвольная YSON-структура,<br> физически представляется последовательностью байт,<br> не может иметь атрибут `required=%true`|`any`|`yson` (отличие от `type`)|`Text**`|

Ниже приведены пояснения по типам данных в Excel.

#### `Number*` { #Number* }

`Number` — это Double-Precision Floating Point value.

В числе может быть только [15 цифр](https://stackoverflow.com/questions/38522197/converting-the-text-data-to-a-int64-format-in-excel),
при попытке вставить `99999999999999999` (`10^17-1`) в ячейке появится `99999999999999900`.

Числа, не удовлетворяющие этому ограничению, экспортируются как строки.

#### `Number**` { #Number** }

Значение помещается в `Number`. `date` и `datetime` экспортируется как `Number` со специальными стилями отображения,
и в excel выглядят следующим образом:
* `date` — `2020-12-05`
* `datetime` — `2000-12-10 10:22:17`

#### `Number***` { #Number*** }

Значение может не поместиться в `Number`.
`Timestamp` с миллисекундным разрешением экспортируется в виде `Number` со специальным стилем отображения (`1969-12-30 00:00:00`), с меньшим — в виде строки формата: `2006-01-02T15:04:05.999999Z`.

#### `Text*` { #Text* }

`Text` — это строковый тип. Максимальная длина строки в ячейке — `32768`.

В {{product-name}} длина строки может быть больше: до `128 * 10^6`. Строки длиннее этого ограничения обрезаются.

#### `Text**` { #Text** }

Значения сериализуются в [YSON](../../user-guide/storage/yson.md).
Длинные строки, как и в `Text*`, обрезаются.

### Отсутствующие значения

Для `optional` типов в случае отсутствия значения в ячейку будет записана пустая строка.