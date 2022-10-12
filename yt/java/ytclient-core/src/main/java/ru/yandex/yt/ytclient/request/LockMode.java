package ru.yandex.yt.ytclient.request;

import ru.yandex.lang.NonNullApi;
import ru.yandex.lang.NonNullFields;

@NonNullFields
@NonNullApi
public enum LockMode {
    Snapshot("snapshot", 1),
    Shared("shared", 2),
    Exclusive("exclusive", 3);

    private final int protoValue;
    private final String wireName;

    LockMode(String wireName, int protoValue) {
        this.wireName = wireName;
        this.protoValue = protoValue;
    }

    public int getProtoValue() {
        return protoValue;
    }

    public String getWireName() {
        return wireName;
    }

}